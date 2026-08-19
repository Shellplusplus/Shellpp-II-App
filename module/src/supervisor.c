#include "shellpp_ii_module.h"
#include "shellpp_native_app.h"

/* The firmware module loader does not provide compiler unwind personalities.
 * This Supervisor does not unwind, but Clang emits the index reference. */
__attribute__((used, naked)) void __aeabi_unwind_cpp_pr0(void) { __asm__("bx lr"); }

typedef int (*register_driver_t)(const char *, void *, unsigned int, void *);
typedef int (*unregister_driver_t)(const char *);
#define REGISTER_DRIVER ((register_driver_t)0x0C1A0D51)
#define UNREGISTER_DRIVER ((unregister_driver_t)0x0C1A611D)
#define SHELLPP_DEVICE "/dev/shellpp"
#define SHELLPP_MAGIC 0x53505331u
#define SHELLPP_STATUS_ABI 2u
#define SHELLPP_BUILD_MARKER 0x53494932u
#define SHELLPP_CMD_RESTORE 0x5351000au
#define SHELLPP_CMD_INSTALL 0x53510002u
#define SHELLPP_CMD_UNINSTALL 0x53510003u
#define SHELLPP_CMD_NOTIFY_LOADED 0x53510004u
#define RESULT_COMPLETED 5u
#define RESULT_FAILED 15u

/* Canopus writes only this four-callback prefix before register_driver. */
struct file_operations_prefix { void *open; void *close; void *read; void *write; void *reserved[8]; };
static struct file_operations_prefix g_fops;
/* Keep the standard writable initialized-data section expected by modlib. */
__attribute__((used, section(".data"))) static volatile unsigned int g_data_anchor = 1;
static unsigned char g_status[384];
static volatile unsigned int g_command, g_stage, g_state, g_registered;
static volatile int g_error;

static int control_open(void *file) { (void)file; return 0; }
static int control_close(void *file) { (void)file; return 0; }
static int control_read(void *file, void *buffer, unsigned int count) {
    unsigned int i;
    struct shellpp_native_status native_status;
    (void)file;
    if (!buffer || count < sizeof(g_status)) return -22;
    for (i = 0; i < sizeof(g_status); ++i) g_status[i] = 0;
    *(unsigned int *)(g_status + 0) = SHELLPP_MAGIC;
    *(unsigned int *)(g_status + 4) = SHELLPP_STATUS_ABI;
    *(unsigned int *)(g_status + 20) = g_command;
    *(unsigned int *)(g_status + 24) = g_state;
    *(int *)(g_status + 32) = g_error;
    /* Match Canopus's stable snapshot pair at words 10 and 11. */
    *(unsigned int *)(g_status + 36) = g_stage;
    *(unsigned int *)(g_status + 40) = g_stage;
    shellpp_native_get_status(&native_status);
    *(unsigned int *)(g_status + 44) = SHELLPP_BUILD_MARKER;
    *(unsigned int *)(g_status + 48) = g_registered;
    *(unsigned int *)(g_status + 52) = native_status.app_id;
    *(unsigned int *)(g_status + 56) = native_status.registered;
    *(unsigned int *)(g_status + 60) = native_status.published;
    *(unsigned int *)(g_status + 64) = native_status.loaded_notified;
    *(int *)(g_status + 68) = native_status.install_result;
    *(int *)(g_status + 72) = native_status.launcher_result;
    *(int *)(g_status + 76) = native_status.notification_result;
    for (i = 0; i < sizeof(g_status); ++i) ((unsigned char *)buffer)[i] = g_status[i];
    return sizeof(g_status);
}
static int control_write(void *file, const void *buffer, unsigned int count) {
    const unsigned int *command = (const unsigned int *)buffer;
    int rc = 0;
    (void)file;
    if (!buffer || count < 16 || command[0] != SHELLPP_MAGIC) return -22;
    g_command = command[1]; g_stage = command[2]; g_error = 0;
    if (g_command == SHELLPP_CMD_NOTIFY_LOADED) {
        rc = shellpp_native_notify_loaded();
    } else if (g_command == SHELLPP_CMD_RESTORE ||
        (g_command == SHELLPP_CMD_INSTALL && g_stage == 0)) {
        rc = 0;
    } else if (g_command == SHELLPP_CMD_INSTALL &&
               (g_stage == 1 || g_stage == 2)) {
        rc = shellpp_native_install_stage(g_stage);
    } else if (g_command == SHELLPP_CMD_UNINSTALL) {
        rc = shellpp_native_uninstall();
    } else {
        rc = -22;
    }
    g_error = rc;
    g_state = rc == 0 ? RESULT_COMPLETED : RESULT_FAILED;
    /* The Canopus device protocol reports command failure through its status
     * snapshot. Preserve the completed write so Lua can read that status. */
    return 16;
}
static void shellpp_supervisor_ctor(void) __attribute__((constructor));
static void shellpp_supervisor_ctor(void) {
    unsigned int i; int rc;
    for (i = 0; i < sizeof(g_fops); ++i) ((unsigned char *)&g_fops)[i] = 0;
    g_fops.open = (void *)control_open; g_fops.close = (void *)control_close;
    g_fops.read = (void *)control_read; g_fops.write = (void *)control_write;
    rc = REGISTER_DRIVER(SHELLPP_DEVICE, &g_fops, 0666, 0);
    g_error = rc; g_registered = rc == 0; g_state = rc == 0 ? RESULT_COMPLETED : RESULT_FAILED;
}
static int shellpp_supervisor_uninit(void *arg) {
    (void)arg;
    if (!shellpp_native_can_unload()) return -16;
    if (g_registered) { (void)UNREGISTER_DRIVER(SHELLPP_DEVICE); g_registered = 0; }
    return 0;
}
int module_initialize(struct shellpp_ii_mod_info *modinfo) {
    modinfo->uninitializer = shellpp_supervisor_uninit; modinfo->arg = 0;
    modinfo->exports = 0; modinfo->nexports = 0; return 0;
}
