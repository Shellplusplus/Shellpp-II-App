#include "shellpp_native_app.h"
#include "shellpp_native_ui.h"

/*
 * Xiaomi Band 10 Pro, firmware 3.101.036.
 *
 * Registration mirrors the verified Canopus Supervisor sequence for this
 * firmware: app lookup -> app/page registration -> app lookup -> Launcher
 * publication -> notification.  The 0x0c addresses are the module runtime
 * mapping used by Canopus; the 0x2c-prefixed firmware dump uses a different
 * image-analysis base and must not be called by a loaded module.
 */
typedef void *(*app_lookup_t)(uint16_t app_id);
typedef int (*app_install_t)(void *app, void *const *pages, uint32_t page_count);
typedef int (*launcher_add_t)(uint16_t app_id);
typedef int (*notification_submit_t)(const void *notification);
#define APP_LOOKUP ((app_lookup_t)0x0ca5107du)
#define APP_INSTALL ((app_install_t)0x0ca51a55u)
#define LAUNCHER_ADD ((launcher_add_t)0x0c4f2c4du)
#define NOTIFICATION_SUBMIT ((notification_submit_t)0x0ca81fb9u)

#define APP_DESCRIPTOR_SIZE 0x40u
#define PAGE_DESCRIPTOR_SIZE 0x74u
#define SHELLPP_PAGE_COUNT 8u
#define SHELLPP_APP_ID 0x00cdu
#define ERR_APP_MISSING (-100)
#define ERR_APP_CONFLICT (-101)
#define ERR_APP_NOT_REGISTERED (-102)
#define ERR_BAD_STAGE (-103)
#define ERR_UNINSTALL_REQUIRES_REBOOT (-95)

static const char g_package_name[] = "com.shellpp.ii";
static const char g_display_name[] = "Shell++ II";
static const char g_page_names[SHELLPP_PAGE_COUNT][16] = {
    "shellpp-home",
    "shellpp-files",
    "shellpp-viewer",
    "shellpp-cache",
    "shellpp-about",
    "shellpp-display",
    "shellpp-cpu",
    "shellpp-restart",
};
static const char g_launcher_icon[] = "/data/shellpp-ii/shellpp_ii_icon.bin";
static const char g_notification_title[] = "Shell++ II";
static const char g_notification_body[] = "Shell++ 已成功加载";

/* Firmware retains descriptors and callback pointers after app_install(). */
static uint32_t g_app_descriptor[APP_DESCRIPTOR_SIZE / sizeof(uint32_t)];
static uint32_t g_page_descriptors[SHELLPP_PAGE_COUNT]
    [PAGE_DESCRIPTOR_SIZE / sizeof(uint32_t)];
static void *g_pages[SHELLPP_PAGE_COUNT];
static volatile uint16_t g_app_id = SHELLPP_APP_ID;
static volatile uint32_t g_registered;
static volatile uint32_t g_published;
static volatile uint32_t g_loaded_notified;
static volatile int g_install_result;
static volatile int g_launcher_result;
static volatile int g_loaded_notification_result;

/* Canopus's 0x58-byte loaded_notification record, with only strings/icon
 * replaced.  It is consumed synchronously by the firmware entry point. */
struct shellpp_notification {
    uint32_t magic0;
    uint32_t magic1;
    uint32_t reserved0;
    const char *title;
    const char *source;
    const char *body;
    const void *reserved1;
    const char *icon;
    const char *small_icon;
    uint32_t reserved2[11];
    uint32_t flags;
    uint32_t reserved3;
};

/* The actual Canopus supervisor has two notification classes.  Both records
 * carry flags = 1, but the first word selects the delivery class:
 *
 *   0x50555302: module_notification, submitted from Canopus's load-notice
 *                entry point and eligible for the foreground alert path.
 *   0x50555301: loaded_notification, submitted after native app install and
 *                retained as the normal completion notice.
 *
 * Shell++ II deliberately emits only one notice per Run.  Its Lua Supervisor
 * command is the equivalent of Canopus's load-notice entry point, so use the
 * foreground module class here.  This preserves the requested single notice
 * while allowing firmware to select the same alert/haptic policy as Canopus. */
static const struct shellpp_notification g_foreground_notification = {
    0x50555302u, 0x43414e4fu, 0u,
    g_notification_title, g_notification_title, g_notification_body,
    0, g_launcher_icon, g_launcher_icon,
    { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
    /* Both Canopus notification classes store flags = 1 at +0x50.  Flags are
     * therefore not the foreground/haptic selector; the class word above is. */
    1u, 0u,
};

_Static_assert(sizeof(g_app_descriptor) == APP_DESCRIPTOR_SIZE,
    "3.101.036 launcher descriptor size");
_Static_assert(sizeof(g_page_descriptors[0]) == PAGE_DESCRIPTOR_SIZE,
    "3.101.036 page descriptor size");
_Static_assert(sizeof(struct shellpp_notification) == 0x58u,
    "Canopus-compatible notification record size");

static void zero_bytes(void *address, uint32_t length) {
    uint8_t *bytes = (uint8_t *)address;
    uint32_t index;
    for (index = 0; index < length; ++index) bytes[index] = 0;
}

static void write_pointer(void *base, uint32_t offset, const void *value) {
    *(const void **)((uint8_t *)base + offset) = value;
}

static void write_u16(void *base, uint32_t offset, uint16_t value) {
    *(uint16_t *)((uint8_t *)base + offset) = value;
}

static void write_u32(void *base, uint32_t offset, uint32_t value) {
    *(uint32_t *)((uint8_t *)base + offset) = value;
}

static const char *launcher_display_name(void) {
    return g_display_name;
}

static int string_equal(const char *left, const char *right) {
    uint32_t index = 0;
    if (!left || !right) return 0;
    do {
        if (left[index] != right[index]) return 0;
    } while (left[index++] != '\0');
    return 1;
}

/* APP_INSTALL queues registry publication on the firmware worker.  A direct
 * APP_LOOKUP immediately after it can therefore observe the old table and
 * incorrectly produce ERR_APP_MISSING.  Yield with a bounded Thumb-safe
 * delay while the worker commits the native App record. */
static void native_registry_wait(void) {
    volatile uint32_t spin;
    for (spin = 0; spin < 180000u; ++spin) {
        __asm__ volatile("nop");
    }
}

static void *lookup_installed_with_retry(uint16_t app_id) {
    uint32_t attempt;
    void *installed = 0;
    for (attempt = 0; attempt < 8u; ++attempt) {
        installed = APP_LOOKUP(app_id);
        if (installed) return installed;
        native_registry_wait();
    }
    return installed;
}

static int installed_package_matches(void *app) {
    const char *package_name;
    if (!app) return 0;
    /* Canopus checks the native app package pointer at +0x08. */
    package_name = *(const char **)((uint8_t *)app + 0x08u);
    return string_equal(package_name, g_package_name);
}

static int page_index_of(void *page) {
    uint32_t index;
    for (index = 0; index < SHELLPP_PAGE_COUNT; ++index) {
        if (page == (void *)g_page_descriptors[index]) return (int)index;
    }
    return -1;
}

static int page_on_signal(void *page, uint32_t signal, void *payload) {
    (void)page;
    (void)signal;
    (void)payload;
    return 0;
}

static int page_on_create(void *page, void *root, void *start_data) {
    int index = page_index_of(page);
    (void)start_data;
    if (index < 0 || !root) return -1;
    return shellpp_ui_page_create((uint32_t)index, page, root);
}

static int page_on_resume(void *page) {
    int index = page_index_of(page);
    return index < 0 ? -1 : shellpp_ui_page_resume((uint32_t)index, page);
}

static int page_on_pause(void *page) {
    int index = page_index_of(page);
    return index < 0 ? -1 : shellpp_ui_page_pause((uint32_t)index);
}

static int page_on_destroy(void *page) {
    int index = page_index_of(page);
    if (index < 0) return -1;
    return shellpp_ui_page_destroy((uint32_t)index);
}

static void initialize_descriptors(uint16_t app_id) {
    uint32_t index;

    zero_bytes(g_app_descriptor, sizeof(g_app_descriptor));
    zero_bytes(g_page_descriptors, sizeof(g_page_descriptors));
    shellpp_ui_reset();

    /* Canopus and the LyraPlayer native reference both use +0x1c for the
     * launcher metadata/display-name callback. */
    write_pointer(g_app_descriptor, 0x08u, g_package_name);
    write_pointer(g_app_descriptor, 0x0cu, g_launcher_icon);
    write_u16(g_app_descriptor, 0x10u, app_id);
    write_pointer(g_app_descriptor, 0x1cu, (const void *)launcher_display_name);

    for (index = 0; index < SHELLPP_PAGE_COUNT; ++index) {
        void *descriptor = g_page_descriptors[index];

        /* Canopus stores the page label at +0x10 and the full key at +0x14:
         * (app_id << 16) | page_id. */
        write_pointer(descriptor, 0x10u, g_page_names[index]);
        write_u32(descriptor, 0x14u, ((uint32_t)app_id << 16) | index);
        write_pointer(descriptor, 0x34u, (const void *)page_on_signal);
        write_pointer(descriptor, 0x4cu, (const void *)page_on_create);
        write_pointer(descriptor, 0x50u, (const void *)page_on_resume);
        write_pointer(descriptor, 0x58u, (const void *)page_on_pause);
        write_pointer(descriptor, 0x5cu, (const void *)page_on_destroy);
        g_pages[index] = descriptor;
    }
}

static int register_native_app(void) {
    void *installed;

    if (g_registered) {
        installed = APP_LOOKUP(g_app_id);
        return installed_package_matches(installed) ? 0 : ERR_APP_MISSING;
    }

    g_app_id = SHELLPP_APP_ID;
    installed = lookup_installed_with_retry(g_app_id);
    if (installed) {
        if (!installed_package_matches(installed)) return ERR_APP_CONFLICT;
        /* This is the same branch Canopus takes when its descriptor has
         * survived a prior request. The installed native app is valid; stage
         * 2 must still be allowed to publish its Launcher item and notice. */
        g_install_result = 0;
        g_registered = 1u;
        return 0;
    }

    initialize_descriptors(g_app_id);
    g_install_result = APP_INSTALL(g_app_descriptor, g_pages, SHELLPP_PAGE_COUNT);
    installed = lookup_installed_with_retry(g_app_id);
    if (!installed) return ERR_APP_MISSING;
    if (!installed_package_matches(installed)) return ERR_APP_CONFLICT;
    g_registered = 1u;
    return 0;
}

static int publish_launcher(void) {
    void *installed;

    if (!g_registered) return ERR_APP_NOT_REGISTERED;
    installed = lookup_installed_with_retry(g_app_id);
    if (!installed || !installed_package_matches(installed)) return ERR_APP_MISSING;

    if (!g_published) {
        /* Canopus records this opaque result but does not interpret it as
         * errno. The verified native app is the publication precondition. */
        g_launcher_result = LAUNCHER_ADD(g_app_id);
        g_published = 1u;
    }
    return 0;
}

int shellpp_native_install_stage(uint32_t stage) {
    if (stage == 1u) return register_native_app();
    if (stage == 2u) return publish_launcher();
    return ERR_BAD_STAGE;
}

int shellpp_native_notify_loaded(void) {
    if (!g_loaded_notified) {
        /* Exactly one notification per boot/Run: use the foreground record.
         * The firmware may also retain this one record in notification center,
         * but Shell++ must not submit a second duplicate entry. */
        g_loaded_notification_result = NOTIFICATION_SUBMIT(&g_foreground_notification);
        g_loaded_notified = 1u;
    }
    return 0;
}

void shellpp_native_get_status(struct shellpp_native_status *status) {
    if (!status) return;
    status->app_id = g_app_id;
    status->registered = g_registered;
    status->published = g_published;
    status->loaded_notified = g_loaded_notified;
    status->install_result = g_install_result;
    status->launcher_result = g_launcher_result;
    status->notification_result = g_loaded_notification_result;
}

int shellpp_native_can_unload(void) {
    return g_registered == 0u;
}

int shellpp_native_uninstall(void) {
    if (!g_registered) return 0;

    /* App and page registries retain callbacks into module text. Until the
     * reverse unregister ABI is proven, do not report a false success. */
    return ERR_UNINSTALL_REQUIRES_REBOOT;
}
