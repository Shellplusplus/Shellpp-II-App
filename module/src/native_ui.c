#include "shellpp_native_ui.h"
#include "shellpp_native_app.h"
#include "shellpp_native_fs.h"
#include "shellpp_firmware_abi.h"

typedef void *(*lvx_content_create_t)(void *root);
typedef void *(*lvx_page_title_create_t)(void *root, const char *title,
    uint32_t mode, const void *back_callback, void *context);
typedef void *(*lvx_label_create_t)(void *parent);
typedef void (*lvx_label_set_text_t)(void *label, const char *text);
typedef void *(*lv_display_get_layer_top_t)(void *display);
typedef void (*lv_timer_cb_t)(void *timer);
typedef void *(*lv_timer_create_t)(lv_timer_cb_t callback, uint32_t period_ms,
    void *user_data);
typedef void (*lv_timer_delete_t)(void *timer);
typedef void (*lvx_object_set_size_t)(void *object, int32_t width, int32_t height);
typedef void (*lvx_object_align_t)(void *object, uint32_t alignment,
    int32_t x_offset, int32_t y_offset);
typedef void (*lvx_align_to_t)(void *object, void *base, uint32_t alignment,
    int32_t x_offset, int32_t y_offset);
typedef void (*lvx_set_hidden_t)(void *object, uint32_t hidden);
typedef int (*lvx_style_apply_t)(void *object, const void *style,
    uint8_t opacity, uint8_t reserved);
typedef void *(*lvx_list_row_create_t)(void *parent, const char *primary,
    const char *secondary, uint32_t trailing);
typedef void (*lvx_list_row_update_t)(void *row, const void *icon,
    const char *primary, const char *secondary, uint32_t trailing,
    uint8_t selected);
typedef void *(*lvx_list_row_trailing_t)(void *row);
typedef void (*lvx_event_add_t)(void *object, void (*callback)(void *),
    uint32_t event_code, void *user_data);
typedef void *(*lvx_event_get_user_data_t)(void *event);
typedef uint32_t (*lvx_event_get_code_t)(void *event);
typedef void (*activity_navigate_t)(uint32_t key, uint32_t arg1,
    uint32_t arg2, uint32_t arg3);
typedef void (*activity_finish_t)(void *descriptor);
typedef int (*restart_spawn_t)(uint32_t *pid, const char *path, void *fa,
    void *attr, char *const *argv, char *const *envp);
typedef int (*restart_fa_init_t)(void *fa);
typedef int (*restart_fa_addopen_t)(void *fa, int fd, const char *path,
    int oflags, uint32_t mode);
typedef int (*restart_fa_destroy_t)(void *fa);
typedef int (*restart_attr_init_t)(void *attr);
typedef int (*restart_attr_destroy_t)(void *attr);
typedef int (*restart_waitpid_t)(uint32_t pid, int *status, int options);

#define LVX_CONTENT_CREATE ((lvx_content_create_t)SHELLPP_ABI_LVX_CONTENT_CREATE_ADDR)
#define LVX_PAGE_TITLE_CREATE ((lvx_page_title_create_t)SHELLPP_ABI_LVX_PAGE_TITLE_CREATE_ADDR)
#define LVX_LABEL_CREATE ((lvx_label_create_t)SHELLPP_ABI_LVX_LABEL_CREATE_ADDR)
#define LVX_LABEL_SET_TEXT ((lvx_label_set_text_t)SHELLPP_ABI_LVX_LABEL_SET_TEXT_ADDR)
#define LV_DISPLAY_GET_LAYER_TOP ((lv_display_get_layer_top_t)SHELLPP_ABI_LV_DISPLAY_GET_LAYER_TOP_ADDR)
#define LV_TIMER_CREATE ((lv_timer_create_t)SHELLPP_ABI_LV_TIMER_CREATE_ADDR)
#define LV_TIMER_DELETE ((lv_timer_delete_t)SHELLPP_ABI_LV_TIMER_DELETE_ADDR)
#define LVX_OBJECT_SET_SIZE ((lvx_object_set_size_t)SHELLPP_ABI_LVX_OBJECT_SET_SIZE_ADDR)
#define LVX_OBJECT_ALIGN ((lvx_object_align_t)SHELLPP_ABI_LVX_OBJECT_ALIGN_ADDR)
#define LVX_ALIGN_TO ((lvx_align_to_t)SHELLPP_ABI_LVX_ALIGN_TO_ADDR)
#define LVX_SET_HIDDEN ((lvx_set_hidden_t)SHELLPP_ABI_LVX_SET_HIDDEN_ADDR)
#define LVX_STYLE_APPLY ((lvx_style_apply_t)SHELLPP_ABI_LVX_STYLE_APPLY_ADDR)
#define LVX_LIST_ROW_CREATE ((lvx_list_row_create_t)SHELLPP_ABI_LVX_LIST_ROW_CREATE_ADDR)
#define LVX_LIST_ROW_UPDATE ((lvx_list_row_update_t)SHELLPP_ABI_LVX_LIST_ROW_UPDATE_ADDR)
#define LVX_LIST_ROW_TRAILING ((lvx_list_row_trailing_t)SHELLPP_ABI_LVX_LIST_ROW_TRAILING_ADDR)
#define LVX_EVENT_ADD ((lvx_event_add_t)SHELLPP_ABI_LVX_EVENT_ADD_ADDR)
#define LVX_EVENT_GET_USER_DATA ((lvx_event_get_user_data_t)SHELLPP_ABI_LVX_EVENT_GET_USER_DATA_ADDR)
#define LVX_EVENT_GET_CODE ((lvx_event_get_code_t)SHELLPP_ABI_LVX_EVENT_GET_CODE_ADDR)
#define ACTIVITY_NAVIGATE ((activity_navigate_t)SHELLPP_ABI_ACTIVITY_NAVIGATE_ADDR)
#define ACTIVITY_FINISH ((activity_finish_t)SHELLPP_ABI_ACTIVITY_FINISH_ADDR)
#define RESTART_SPAWN ((restart_spawn_t)SHELLPP_ABI_POSIX_SPAWN_ADDR)
#define RESTART_FA_INIT ((restart_fa_init_t)SHELLPP_ABI_FILE_ACTIONS_INIT_ADDR)
#define RESTART_FA_ADDOPEN ((restart_fa_addopen_t)SHELLPP_ABI_FILE_ACTIONS_ADDOPEN_ADDR)
#define RESTART_FA_DESTROY ((restart_fa_destroy_t)SHELLPP_ABI_FILE_ACTIONS_DESTROY_ADDR)
#define RESTART_ATTR_INIT ((restart_attr_init_t)SHELLPP_ABI_SPAWNATTR_INIT_ADDR)
#define RESTART_ATTR_DESTROY ((restart_attr_destroy_t)SHELLPP_ABI_SPAWNATTR_DESTROY_ADDR)
#define RESTART_WAITPID ((restart_waitpid_t)SHELLPP_ABI_WAITPID_ADDR)
#define RESTART_SOFT ((void (*)(void))SHELLPP_ABI_SOFT_RESTART_ADDR)
#define STYLE_MISANS_DEMIBOLD_32 ((const void *)SHELLPP_ABI_STYLE_MISANS_DEMIBOLD_32_ADDR)

#define SHELLPP_APP_ID 0x00cdu
#define PAGE_HOME 0u
#define PAGE_FILES 1u
#define PAGE_VIEWER 2u
#define PAGE_CACHE 3u
#define PAGE_ABOUT 4u
#define PAGE_DISPLAY 5u
#define PAGE_CPU 6u
#define PAGE_CPU_BENCH 7u
#define PAGE_MEMORY 8u
#define PAGE_RESTART 9u
#define PAGE_VIBRATION 10u
#define PAGE_COUNT 11u
#define MONITOR_CPU 0u
#define MONITOR_MEMORY 1u
#define MONITOR_STATE_MEMORY_PAGE 0x01u
#define MONITOR_STATE_MEMORY_ENABLED 0x02u
#define MONITOR_STATE_MEMORY_FLOAT 0x04u
#define UI_MAX_ROWS 28u
#define CONTENT_WIDTH 336
#define CONTENT_HEIGHT 424
#define CONTENT_TOP_OFFSET 56
#define ALIGN_TOP_MID SHELLPP_ABI_ALIGN_TOP_MID
#define ALIGN_TOP_LEFT SHELLPP_ABI_ALIGN_TOP_LEFT
#define ALIGN_OUT_BOTTOM_MID SHELLPP_ABI_ALIGN_OUT_BOTTOM_MID
#define CPU_FLOAT_LAYER_X 25
#define CPU_FLOAT_LAYER_Y 20
#define CPU_FLOAT_LABEL_X -20
#define CPU_FLOAT_LABEL_Y 0
#define CPU_FLOAT_WIDTH 150
#define CPU_FLOAT_HEIGHT 52
#define MEMORY_FLOAT_OFFSET_Y 20
#define VIEW_LABEL_TOP 0
#define VIEW_LABEL_HEIGHT 210
#define VIEW_ROW_TOP 216
#define EVENT_CLICKED SHELLPP_ABI_EVENT_CLICKED
#define TRAILING_NONE SHELLPP_ABI_TRAILING_NONE
#define ROW_GAP 4
#define LABEL_SLICE 384u
#define HEX_RAW_OFFSET 8192u
#define HEX_SCREEN_LINES 10u
#define FS_TYPE_REGULAR 8u
#define APP_MAX_ITEMS 256u
#define APP_SELECTION_BYTES (APP_MAX_ITEMS / 8u)
#define APP_PAGE_SIZE 16u
#define APP_NAME_CAP 80u
#define APP_PACKAGE_CAP 96u
#define APP_TEXT_CAP 7808u
#define APP_OBJECT_CAP 3996u
#define APP_JSON_KEY_CAP 128u
#define APP_MODE_MENU 0u
#define APP_MODE_LIST 1u
#define APP_OPERATION_HIDE 1u
#define APP_OPERATION_SHOW 2u
#define APP_OPERATION_DELETE 3u
#define APP_SOURCE_VISIBLE 0u
#define APP_SOURCE_HIDDEN 1u

enum browser_mode {
    BROWSER_LIST = 0,
    BROWSER_DETAIL = 1,
    BROWSER_TEXT = 2,
    BROWSER_HEX = 3,
};

enum ui_action {
    ACTION_NONE = 0,
    ACTION_NAVIGATE = 1,
    ACTION_BROWSER_ENTRY = 2,
    ACTION_BROWSER_PARENT = 3,
    ACTION_BROWSER_PREVIOUS = 4,
    ACTION_BROWSER_NEXT = 5,
    ACTION_BROWSER_PASTE = 6,
    ACTION_OPEN_TEXT = 7,
    ACTION_OPEN_HEX = 8,
    ACTION_CLIP_COPY = 10,
    ACTION_CLIP_MOVE = 11,
    ACTION_DELETE = 12,
    ACTION_TEXT_PREVIOUS = 13,
    ACTION_TEXT_NEXT = 14,
    ACTION_HEX_PREVIOUS = 15,
    ACTION_HEX_NEXT = 16,
    ACTION_CACHE_REFRESH = 33,
    ACTION_CACHE_LOGS = 34,
    ACTION_CACHE_CLEAR = 35,
    ACTION_BROWSER_REFRESH = 36,
    ACTION_BROWSER_BACK = 37,
    ACTION_FILE_GROUP_OPEN = 38,
    ACTION_DISPLAY_OPEN = 39,
    ACTION_CPU_MONITOR = 40,
    ACTION_CPU_FLOAT = 41,
    ACTION_CPU_REFRESH = 42,
    ACTION_CPU_PAGE_OPEN = 43,
    ACTION_MEMORY_PAGE_OPEN = 44,
    ACTION_MEMORY_MONITOR = 45,
    ACTION_MEMORY_FLOAT = 46,
    ACTION_MEMORY_REFRESH = 47,
    ACTION_APPS_OPEN = 48,
    ACTION_APP_REFRESH = 49,
    ACTION_APP_SELECT_ALL = 50,
    ACTION_APP_TOGGLE = 51,
    ACTION_APP_HIDE = 52,
    ACTION_APP_SHOW = 53,
    ACTION_APP_DELETE = 54,
    ACTION_APP_HIDE_ALL = 55,
    ACTION_APP_SHOW_ALL = 56,
    ACTION_APP_PREVIOUS = 57,
    ACTION_APP_NEXT = 58,
    ACTION_APP_REBOOT = 59,
    ACTION_RESTART_HARD = 60,
    ACTION_RESTART_SOFT = 61,
    ACTION_BROWSER_DETAIL_BACK = 62,
    ACTION_CPU_BENCH = 63,
    ACTION_CPU_BENCH_PAGE_OPEN = 64,
    ACTION_VIBRATION_PAGE_OPEN = 65,
    ACTION_VIBRATION_TEST = 66,
};

struct ui_binding {
    uint8_t action;
    uint8_t argument;
    uint8_t enabled;
    uint8_t reserved;
};

struct ui_page {
    void *root;
    void *content;
    void *title;
    void *descriptor;
    void *label;
    void *rows[UI_MAX_ROWS];
    uint16_t generation;
    uint8_t active;
    uint8_t interactive;
};

struct row_spec {
    const char *primary;
    const char *secondary;
    uint8_t action;
    uint8_t argument;
    uint8_t enabled;
    uint8_t trailing;
    uint8_t checked;
};

struct app_ui_state {
    uint32_t workspace_used;
    uint16_t total;
    uint16_t page;
    uint16_t selected_count;
    uint8_t selected[APP_SELECTION_BYTES];
    uint8_t delete_armed;
    uint8_t reboot_armed;
    uint8_t loaded;
    uint8_t truncated;
};

struct app_item_meta {
    uint16_t name_offset;
    uint16_t package_offset;
    uint8_t hidden;
    uint8_t locked;
    uint8_t valid;
    uint8_t source_index;
};

static struct ui_page g_ui[PAGE_COUNT];
/* Only the foreground page needs live action bindings. Sharing this table
 * keeps the extra benchmark page within the 036 firmware BSS limit. */
static struct ui_binding g_bindings[UI_MAX_ROWS];
static uint16_t g_bindings_generation;
static uint8_t g_bindings_page;
static union {
    struct shellpp_fs_page directory;
    struct app_item_meta apps[APP_MAX_ITEMS];
} g_list;
#define g_directory_page (g_list.directory)
#define g_app_items (g_list.apps)
static struct shellpp_fs_cursor g_after_cursor;
static struct shellpp_fs_cursor g_navigation_cursor;
static struct shellpp_cache_report g_cache_report;
/* Application-manager metadata is only needed while the file browser is not
 * using its edit workspace.  Overlay it so the module stays inside the
 * firmware's fixed 24 KiB .bss budget. */
struct app_workspace {
    struct app_ui_state state;
    char text[APP_TEXT_CAP];
    uint8_t object[APP_OBJECT_CAP];
};

union shellpp_workspace {
    uint8_t bytes[SHELLPP_FS_EDIT_LIMIT + 1u];
    struct app_workspace app;
    uint32_t alignment;
};
static union shellpp_workspace g_workspace_storage;
#define g_workspace (g_workspace_storage.bytes)
#define g_app (g_workspace_storage.app.state)
#define g_app_text (g_workspace_storage.app.text)
#define g_app_object (g_workspace_storage.app.object)
#define g_app_truncated (g_app.truncated)

_Static_assert(sizeof(struct app_workspace) <= SHELLPP_FS_EDIT_LIMIT + 1u,
    "application workspace exceeds file editor workspace");
static char g_current_path[SHELLPP_FS_PATH_CAP];
static char g_selected_path[SHELLPP_FS_PATH_CAP];
static char g_clipboard_path[SHELLPP_FS_PATH_CAP];
static char g_path_buffer[SHELLPP_FS_PATH_CAP];
static char g_status[128];
static char g_size_text[40];
static char g_entry_secondary[SHELLPP_FS_DIR_PAGE_ENTRIES][32];
static char g_cache_secondary[SHELLPP_FS_CACHE_ROOTS][40];
static char g_clipboard_secondary[96];
static char g_cache_total_text[24];
static char g_cache_freed_text[24];
static char g_memory_text[48];
static char g_cpu_bench_rows[6][24];
/* The resident Shell++ Lua service owns this application data directory. */

static uint32_t g_workspace_length;
static uint32_t g_selected_size;
static uint32_t g_text_offset;
static uint32_t g_hex_file_offset;
static uint32_t g_hex_page_length;
static uint32_t g_hex_line_offset;
static uint32_t g_cut_index;
static uint8_t g_cut_byte;
static uint8_t g_cut_active;
static uint8_t g_browser_owner;
static uint8_t g_browser_mode;
static uint8_t g_browser_read_only;
static uint8_t g_selected_type;
static uint8_t g_selected_is_link;
static uint8_t g_selected_size_known;
static uint8_t g_clipboard_mode;
static uint8_t g_delete_armed;
static uint8_t g_cache_include_logs;
static uint8_t g_cache_clear_armed;
static uint8_t g_busy;
static uint8_t g_viewer_rebuild_pending;
/* This must remain outside the shared editor/application workspace. */
static uint8_t g_app_mode;
static uint32_t g_cache_last_freed;
static uint8_t g_monitor_state;
static uint8_t g_cpu_monitor_enabled;
static uint8_t g_cpu_float_enabled;
static void *g_cpu_timer;
static void *g_cpu_float_label;
static char g_cpu_text[24];
static void *g_memory_float_label;
static uint8_t g_restart_hard_armed;
static uint8_t g_restart_soft_armed;

static const char g_empty[] = "";
static const char g_page_titles[PAGE_COUNT][32] = {
    "Shell++ II",
    "文件与应用管理",
    "文件查看",
    "缓存清理",
    "关于 Shell++ II",
    "显示",
    "占用显示",
    "CPU跑分",
    "内存占用",
    "重启",
    "震动测试",
};

static void clear_bytes(void *address, uint32_t length) {
    uint8_t *bytes = (uint8_t *)address;
    uint32_t index;
    for (index = 0; index < length; ++index) bytes[index] = 0;
}

static uint32_t text_length(const char *text, uint32_t limit) {
    uint32_t length = 0;
    if (!text) return limit;
    while (length < limit && text[length]) ++length;
    return length;
}

static int copy_text(char *target, uint32_t capacity, const char *source) {
    uint32_t length;
    uint32_t index;
    if (!target || !capacity || !source) return -1;
    length = text_length(source, capacity);
    if (length >= capacity) return -1;
    for (index = 0; index < length; ++index) target[index] = source[index];
    target[length] = '\0';
    return 0;
}

static void set_status(const char *text) {
    if (copy_text(g_status, sizeof(g_status), text ? text : g_empty) < 0)
        g_status[0] = '\0';
}

static char *append_text(char *cursor, char *end, const char *text) {
    if (!text) return cursor;
    while (*text && cursor + 1 < end) *cursor++ = *text++;
    *cursor = '\0';
    return cursor;
}

static char *append_u32(char *cursor, char *end, uint32_t value) {
    static const uint32_t powers[] = {
        1000000000u, 100000000u, 10000000u, 1000000u, 100000u,
        10000u, 1000u, 100u, 10u, 1u
    };
    uint32_t index;
    uint8_t started = 0u;
    for (index = 0; index < sizeof(powers) / sizeof(powers[0]); ++index) {
        uint8_t digit = 0u;
        while (value >= powers[index]) { value -= powers[index]; ++digit; }
        if (digit || started || index + 1u == sizeof(powers) / sizeof(powers[0])) {
            if (cursor + 1 < end) *cursor++ = (char)('0' + digit);
            started = 1u;
        }
    }
    *cursor = '\0';
    return cursor;
}

static void format_size(char *buffer, uint32_t capacity, uint32_t size,
        uint8_t known) {
    char *cursor = buffer;
    char *end = buffer + capacity;
    if (!capacity) return;
    *cursor = '\0';
    if (!known) { (void)append_text(cursor, end, "大小未知"); return; }
    cursor = append_u32(cursor, end, size);
    (void)append_text(cursor, end, " B");
}

static void format_cache_size(char *buffer, uint32_t capacity, uint32_t bytes) {
    const uint32_t megabyte = 1024u * 1024u;
    char *cursor = buffer;
    char *end = buffer + capacity;
    uint32_t remainder;
    uint32_t fraction;
    if (!capacity) return;
    *cursor = '\0';
    cursor = append_u32(cursor, end, bytes / megabyte);
    remainder = bytes % megabyte;
    fraction = (remainder * 1000u) / megabyte;
    if (cursor + 1 < end) *cursor++ = '.';
    if (cursor + 1 < end) *cursor++ = (char)('0' + (fraction / 100u));
    if (cursor + 1 < end) *cursor++ = (char)('0' + ((fraction / 10u) % 10u));
    if (cursor + 1 < end) *cursor++ = (char)('0' + (fraction % 10u));
    (void)append_text(cursor, end, "MB");
}

static void restore_cut(void) {
    if (g_cut_active) {
        g_workspace[g_cut_index] = g_cut_byte;
        g_cut_active = 0u;
    }
}

static uint32_t utf8_floor(uint32_t position, uint32_t length) {
    if (position > length) position = length;
    while (position > 0u && position < length &&
            (g_workspace[position] & 0xc0u) == 0x80u) --position;
    return position;
}

static void prepare_cut(uint32_t start, uint32_t length, uint32_t slice) {
    uint32_t end = start + slice;
    restore_cut();
    if (end > length) end = length;
    end = utf8_floor(end, length);
    g_cut_index = end;
    g_cut_byte = g_workspace[end];
    g_workspace[end] = 0u;
    g_cut_active = 1u;
}

static uint32_t event_cookie(uint16_t generation, uint8_t page, uint8_t slot) {
    return ((uint32_t)generation << 16) | ((uint32_t)page << 8) | slot;
}

static void apply_misans(void *object) {
    if (object)
        (void)LVX_STYLE_APPLY(object, STYLE_MISANS_DEMIBOLD_32, 255u, 0u);
}

static int memory_sample(void);
static int set_cpu_monitor(uint8_t enabled);
static int set_cpu_float(uint8_t enabled);
static int set_memory_monitor(uint8_t enabled);
static int set_memory_float(uint8_t enabled);

static int cpu_sample(void) {
    int result = shellpp_fs_read_cpu(g_cpu_text, sizeof(g_cpu_text), 0);
    if (g_cpu_float_label && g_cpu_float_enabled)
        LVX_LABEL_SET_TEXT(g_cpu_float_label, g_cpu_text);
    if (g_ui[PAGE_CPU].active && g_ui[PAGE_CPU].interactive &&
            g_ui[PAGE_CPU].rows[2]) {
        LVX_LIST_ROW_UPDATE(g_ui[PAGE_CPU].rows[2], 0, "当前占用",
            g_cpu_text, TRAILING_NONE, 0u);
    }
    return result;
}

static void format_cpu_bench_result(
        const struct shellpp_cpu_bench_result *result) {
    char *cursor;
    char *end;
    cursor = g_cpu_bench_rows[0]; end = cursor + sizeof(g_cpu_bench_rows[0]);
    cursor = append_text(cursor, end, "综合：");
    cursor = append_u32(cursor, end, result->overall_score);
    (void)append_text(cursor, end, " 分");
    cursor = g_cpu_bench_rows[1]; end = cursor + sizeof(g_cpu_bench_rows[1]);
    cursor = append_text(cursor, end, "整数：");
    cursor = append_u32(cursor, end, result->integer_score);
    (void)append_text(cursor, end, " 分");
    cursor = g_cpu_bench_rows[2]; end = cursor + sizeof(g_cpu_bench_rows[2]);
    cursor = append_text(cursor, end, "浮点：");
    cursor = append_u32(cursor, end, result->float_score);
    (void)append_text(cursor, end, " 分");
    cursor = g_cpu_bench_rows[3]; end = cursor + sizeof(g_cpu_bench_rows[3]);
    cursor = append_text(cursor, end, "内存：");
    cursor = append_u32(cursor, end, result->memory_score);
    (void)append_text(cursor, end, " 分");
    cursor = g_cpu_bench_rows[4]; end = cursor + sizeof(g_cpu_bench_rows[4]);
    cursor = append_text(cursor, end, "混合：");
    cursor = append_u32(cursor, end, result->mixed_score);
    (void)append_text(cursor, end, " 分");
    cursor = g_cpu_bench_rows[5]; end = cursor + sizeof(g_cpu_bench_rows[5]);
    cursor = append_text(cursor, end, "耗时：");
    cursor = append_u32(cursor, end, result->elapsed_ms);
    (void)append_text(cursor, end, " ms");
}

static int cpu_bench_run(void) {
    uint8_t cpu_enabled = g_cpu_monitor_enabled;
    uint8_t memory_enabled = (g_monitor_state & MONITOR_STATE_MEMORY_ENABLED) != 0u;
    uint8_t memory_float = (g_monitor_state & MONITOR_STATE_MEMORY_FLOAT) != 0u;
    struct shellpp_cpu_bench_result bench_result;
    int status;

    if (cpu_enabled) (void)set_cpu_monitor(0u);
    if (memory_enabled) (void)set_memory_monitor(0u);
    if (memory_float) (void)set_memory_float(0u);
    status = shellpp_cpu_bench_run(&bench_result);
    if (status == 0) {
        format_cpu_bench_result(&bench_result);
        copy_text(g_status, sizeof(g_status),
            "CPU跑分完成");
    } else {
        copy_text(g_cpu_bench_rows[0], sizeof(g_cpu_bench_rows[0]),
            "综合：不可用");
        copy_text(g_cpu_bench_rows[1], sizeof(g_cpu_bench_rows[1]),
            "整数：不可用");
        copy_text(g_cpu_bench_rows[2], sizeof(g_cpu_bench_rows[2]),
            "浮点：不可用");
        copy_text(g_cpu_bench_rows[3], sizeof(g_cpu_bench_rows[3]),
            "内存：不可用");
        copy_text(g_cpu_bench_rows[4], sizeof(g_cpu_bench_rows[4]),
            "混合：不可用");
        copy_text(g_cpu_bench_rows[5], sizeof(g_cpu_bench_rows[5]),
            "耗时：不可用");
        copy_text(g_status, sizeof(g_status),
            "CPU跑分失败");
    }
    if (memory_float) (void)set_memory_float(1u);
    if (memory_enabled) (void)set_memory_monitor(1u);
    if (cpu_enabled) (void)set_cpu_monitor(1u);
    return status;
}

static void cpu_timer_callback(void *timer) {
    (void)timer;
    if (g_cpu_monitor_enabled) (void)cpu_sample();
    if (g_monitor_state & (MONITOR_STATE_MEMORY_ENABLED |
            MONITOR_STATE_MEMORY_FLOAT)) (void)memory_sample();
}

static void stop_cpu_overlay(void) {
    g_cpu_monitor_enabled = 0u;
    if (g_cpu_timer && !(g_monitor_state & MONITOR_STATE_MEMORY_ENABLED)) {
        LV_TIMER_DELETE(g_cpu_timer);
        g_cpu_timer = 0;
    }
    g_cpu_float_enabled = 0u;
    if (g_cpu_float_label) LVX_SET_HIDDEN(g_cpu_float_label, 1u);
}

static int memory_sample(void) {
    /* LVX retains label text pointers. Keep the sampled value in module
     * storage instead of passing a timer callback's stack buffer. */
    int result = shellpp_fs_read_memory(g_memory_text,
        sizeof(g_memory_text), 0);
    if (g_memory_float_label &&
            (g_monitor_state & MONITOR_STATE_MEMORY_FLOAT))
        LVX_LABEL_SET_TEXT(g_memory_float_label, g_memory_text);
    if (g_ui[PAGE_MEMORY].active && g_ui[PAGE_MEMORY].interactive &&
            g_ui[PAGE_MEMORY].rows[2]) {
        LVX_LIST_ROW_UPDATE(g_ui[PAGE_MEMORY].rows[2], 0, "当前占用",
            g_memory_text, TRAILING_NONE, 0u);
    }
    return result;
}

static void stop_memory_overlay(void) {
    g_monitor_state &= (uint8_t)~(MONITOR_STATE_MEMORY_ENABLED |
        MONITOR_STATE_MEMORY_FLOAT);
    if (g_cpu_timer && !g_cpu_monitor_enabled && !(g_monitor_state &
            (MONITOR_STATE_MEMORY_ENABLED | MONITOR_STATE_MEMORY_FLOAT))) {
        LV_TIMER_DELETE(g_cpu_timer);
        g_cpu_timer = 0;
    }
    if (g_memory_float_label) LVX_SET_HIDDEN(g_memory_float_label, 1u);
}

static int ensure_cpu_float(void) {
    void *top;
    if (g_cpu_float_label) return 0;
    top = LV_DISPLAY_GET_LAYER_TOP(0);
    if (!top) return -1;
    g_cpu_float_label = LVX_LABEL_CREATE(top);
    if (!g_cpu_float_label) return -1;
    /* Lua creates a transparent 150x52 top-layer container at (10, 0),
     * then positions the Band 10 Pro label at (-20, 0) inside it. The
     * verified native API has no raw object constructor, so apply the same
     * effective top-layer coordinates directly to the label. */
    LVX_OBJECT_SET_SIZE(g_cpu_float_label, CPU_FLOAT_WIDTH, CPU_FLOAT_HEIGHT);
    LVX_OBJECT_ALIGN(g_cpu_float_label, ALIGN_TOP_LEFT,
        CPU_FLOAT_LAYER_X + CPU_FLOAT_LABEL_X,
        CPU_FLOAT_LAYER_Y + CPU_FLOAT_LABEL_Y);
    LVX_LABEL_SET_TEXT(g_cpu_float_label, g_cpu_text);
    /* CPU data is ASCII-only. Keep the system default label font here so the
     * persistent overlay stays smaller than the MiSans 32 page typography. */
    LVX_SET_HIDDEN(g_cpu_float_label, g_cpu_float_enabled ? 0u : 1u);
    return 0;
}

static int set_cpu_float(uint8_t enabled) {
    if (enabled) {
        if (ensure_cpu_float() < 0) return -1;
        g_cpu_float_enabled = 1u;
        LVX_LABEL_SET_TEXT(g_cpu_float_label, g_cpu_text);
        LVX_SET_HIDDEN(g_cpu_float_label, 0u);
    } else {
        g_cpu_float_enabled = 0u;
        if (g_cpu_float_label) {
            LVX_LABEL_SET_TEXT(g_cpu_float_label, g_empty);
            LVX_SET_HIDDEN(g_cpu_float_label, 1u);
        }
    }
    return 0;
}

static int set_cpu_monitor(uint8_t enabled) {
    if (enabled) {
        if (g_cpu_monitor_enabled) return 0;
        g_cpu_monitor_enabled = 1u;
        if (!g_cpu_timer)
            g_cpu_timer = LV_TIMER_CREATE(cpu_timer_callback, 500u, 0);
        if (!g_cpu_timer) {
            g_cpu_monitor_enabled = 0u;
            return -1;
        }
        (void)cpu_sample();
    } else {
        g_cpu_monitor_enabled = 0u;
        if (g_cpu_timer && !(g_monitor_state &
                MONITOR_STATE_MEMORY_ENABLED)) {
            LV_TIMER_DELETE(g_cpu_timer);
            g_cpu_timer = 0;
        }
    }
    return 0;
}

static int ensure_memory_float(void) {
    void *top;
    if (g_memory_float_label) return 0;
    top = LV_DISPLAY_GET_LAYER_TOP(0);
    if (!top) return -1;
    g_memory_float_label = LVX_LABEL_CREATE(top);
    if (!g_memory_float_label) return -1;
    LVX_OBJECT_SET_SIZE(g_memory_float_label, CPU_FLOAT_WIDTH, CPU_FLOAT_HEIGHT);
    LVX_OBJECT_ALIGN(g_memory_float_label, ALIGN_TOP_LEFT,
        CPU_FLOAT_LAYER_X + CPU_FLOAT_LABEL_X,
        CPU_FLOAT_LAYER_Y + CPU_FLOAT_LABEL_Y + MEMORY_FLOAT_OFFSET_Y);
    LVX_LABEL_SET_TEXT(g_memory_float_label, "MEM:0%");
    LVX_SET_HIDDEN(g_memory_float_label,
        (g_monitor_state & MONITOR_STATE_MEMORY_FLOAT) ? 0u : 1u);
    return 0;
}

static int set_memory_float(uint8_t enabled) {
    if (enabled) {
        if (ensure_memory_float() < 0) return -1;
        g_monitor_state |= MONITOR_STATE_MEMORY_FLOAT;
        if (!g_cpu_timer)
            g_cpu_timer = LV_TIMER_CREATE(cpu_timer_callback, 500u, 0);
        if (!g_cpu_timer) {
            g_monitor_state &= (uint8_t)~MONITOR_STATE_MEMORY_FLOAT;
            return -1;
        }
        (void)memory_sample();
        LVX_SET_HIDDEN(g_memory_float_label, 0u);
    } else {
        g_monitor_state &= (uint8_t)~MONITOR_STATE_MEMORY_FLOAT;
        if (g_memory_float_label) {
            LVX_LABEL_SET_TEXT(g_memory_float_label, g_empty);
            LVX_SET_HIDDEN(g_memory_float_label, 1u);
        }
        if (g_cpu_timer && !g_cpu_monitor_enabled && !(g_monitor_state &
                MONITOR_STATE_MEMORY_ENABLED)) {
            LV_TIMER_DELETE(g_cpu_timer);
            g_cpu_timer = 0;
        }
    }
    return 0;
}

static int set_memory_monitor(uint8_t enabled) {
    if (enabled) {
        if (g_monitor_state & MONITOR_STATE_MEMORY_ENABLED) return 0;
        g_monitor_state |= MONITOR_STATE_MEMORY_ENABLED;
        if (!g_cpu_timer)
            g_cpu_timer = LV_TIMER_CREATE(cpu_timer_callback, 500u, 0);
        if (!g_cpu_timer) {
            g_monitor_state &= (uint8_t)~MONITOR_STATE_MEMORY_ENABLED;
            return -1;
        }
        (void)memory_sample();
    } else {
        g_monitor_state &= (uint8_t)~MONITOR_STATE_MEMORY_ENABLED;
        if (g_cpu_timer && !g_cpu_monitor_enabled && !(g_monitor_state &
                MONITOR_STATE_MEMORY_FLOAT)) {
            LV_TIMER_DELETE(g_cpu_timer);
            g_cpu_timer = 0;
        }
    }
    return 0;
}

static void set_row_hidden(void *row, uint32_t hidden) {
    void *trailing;
    if (!row) return;
    LVX_SET_HIDDEN(row, hidden);
    trailing = LVX_LIST_ROW_TRAILING(row);
    if (trailing) LVX_SET_HIDDEN(trailing, hidden);
}

static void set_row_enabled(void *row, uint8_t enabled) {
    void *trailing;
    if (!row) return;
    LVX_SET_HIDDEN(row, 0u);
    trailing = LVX_LIST_ROW_TRAILING(row);
    if (trailing) LVX_SET_HIDDEN(trailing, enabled ? 0u : 1u);
}

static void row_event(void *event);
static void title_back_event(void *event);
static void render_page(uint32_t page_index);
static int handle_back(uint32_t page_index);

/* A list-row container retains its scroll/layout state on this firmware.
 * Build a fresh hidden-safe viewport when the browser changes views so
 * nested directories cannot leave rows from the previous directory behind. */
static void rebuild_viewer_content(void) {
    struct ui_page *ui = &g_ui[PAGE_VIEWER];
    void *content;
    if (!ui->active || !ui->root) return;
    content = LVX_CONTENT_CREATE(ui->root);
    if (!content) {
        set_status("视图刷新失败");
        return;
    }
    LVX_OBJECT_SET_SIZE(content, CONTENT_WIDTH, CONTENT_HEIGHT);
    LVX_OBJECT_ALIGN(content, ALIGN_TOP_MID, 0, CONTENT_TOP_OFFSET);
    if (ui->content) LVX_SET_HIDDEN(ui->content, 1u);
    ui->content = content;
    ui->label = 0;
    clear_bytes(ui->rows, sizeof(ui->rows));
    if (g_bindings_page == PAGE_VIEWER) g_bindings_page = 0xffu;
}

static int ensure_row(struct ui_page *ui, uint32_t page_index, uint32_t slot,
        uint8_t trailing) {
    void *row;
    if (ui->rows[slot]) return 0;
    row = LVX_LIST_ROW_CREATE(ui->content, g_empty, g_empty, trailing);
    if (!row) return -1;
    ui->rows[slot] = row;
    apply_misans(row);
    LVX_EVENT_ADD(row, row_event, EVENT_CLICKED,
        (void *)(uintptr_t)event_cookie(ui->generation, (uint8_t)page_index,
            (uint8_t)slot));
    LVX_OBJECT_ALIGN(row, ALIGN_TOP_MID, 0, 0);
    return 0;
}

static void apply_specs(uint32_t page_index, const struct row_spec *specs,
        uint32_t count, const char *label_text, int32_t label_height,
        int32_t row_start) {
    struct ui_page *ui = &g_ui[page_index];
    uint32_t index;
    if (count > UI_MAX_ROWS) count = UI_MAX_ROWS;
    clear_bytes(g_bindings, sizeof(g_bindings));
    g_bindings_page = (uint8_t)page_index;
    g_bindings_generation = ui->generation;
    /* Hide stale rows before painting the new directory. Do not update them
     * with empty text: this firmware still lays out an empty list-row as a
     * visible card, which creates placeholder cards below the real entries. */
    for (index = 0; index < UI_MAX_ROWS; ++index) {
        if (ui->rows[index]) {
            set_row_hidden(ui->rows[index], 1u);
        }
    }
    if (ui->label) LVX_SET_HIDDEN(ui->label, 1u);
    if (label_text) {
        if (!ui->label) {
            ui->label = LVX_LABEL_CREATE(ui->content);
            if (!ui->label) return;
            apply_misans(ui->label);
        }
        LVX_LABEL_SET_TEXT(ui->label, label_text);
        LVX_OBJECT_SET_SIZE(ui->label, CONTENT_WIDTH - 8, label_height);
        LVX_OBJECT_ALIGN(ui->label, ALIGN_TOP_MID, 0, 0);
        LVX_SET_HIDDEN(ui->label, 0u);
    }
    for (index = 0; index < count; ++index) {
        if (ensure_row(ui, page_index, index, specs[index].trailing) < 0) return;
        LVX_LIST_ROW_UPDATE(ui->rows[index], 0, specs[index].primary,
            specs[index].secondary ? specs[index].secondary : g_empty,
            TRAILING_NONE, specs[index].checked);
        apply_misans(ui->rows[index]);
        if (index == 0u) {
            LVX_OBJECT_ALIGN(ui->rows[index], ALIGN_TOP_MID, 0, row_start);
        } else {
            LVX_ALIGN_TO(ui->rows[index], ui->rows[index - 1u],
                ALIGN_OUT_BOTTOM_MID, 0, ROW_GAP);
        }
        g_bindings[index].action = specs[index].action;
        g_bindings[index].argument = specs[index].argument;
        g_bindings[index].enabled = specs[index].enabled;
        set_row_enabled(ui->rows[index], specs[index].enabled);
    }
}

/* Keep viewer content below the action rows and outside list-row layout. */
static void apply_view_label(uint32_t page_index, const char *text) {
    struct ui_page *ui = &g_ui[page_index];
    if (!ui->label) ui->label = LVX_LABEL_CREATE(ui->content);
    if (!ui->label) return;
    LVX_LABEL_SET_TEXT(ui->label, text ? text : g_empty);
    LVX_OBJECT_SET_SIZE(ui->label, CONTENT_WIDTH - 8, VIEW_LABEL_HEIGHT);
    LVX_OBJECT_ALIGN(ui->label, ALIGN_TOP_MID, 0, VIEW_LABEL_TOP);
    LVX_SET_HIDDEN(ui->label, 0u);
}

static void format_view_page(uint32_t current, uint32_t total) {
    char *cursor = g_size_text;
    char *end = g_size_text + sizeof(g_size_text);
    *cursor = '\0';
    cursor = append_u32(cursor, end, current ? current : 1u);
    if (cursor + 3 < end) {
        *cursor++ = ' ';
        *cursor++ = '/';
        *cursor++ = ' ';
        *cursor = '\0';
    }
    (void)append_u32(cursor, end, total ? total : 1u);
}

static void add_spec(struct row_spec *specs, uint32_t *count,
        const char *primary, const char *secondary, uint8_t action,
        uint8_t argument, uint8_t enabled) {
    if (*count >= UI_MAX_ROWS) return;
    specs[*count].primary = primary;
    specs[*count].secondary = secondary ? secondary : g_empty;
    specs[*count].action = action;
    specs[*count].argument = argument;
    specs[*count].enabled = enabled;
    /* Match the About page: cards remain clickable, with no selection circle,
     * switch, or forward affordance on the right. */
    specs[*count].trailing = TRAILING_NONE;
    specs[*count].checked = 0u;
    ++*count;
}

static const char *fs_error_text(int result) {
    switch (result) {
        case SHELLPP_FS_ERR_PATH: return "路径无效";
        case SHELLPP_FS_ERR_OPEN: return "打开失败";
        case SHELLPP_FS_ERR_READ: return "读取失败";
        case SHELLPP_FS_ERR_WRITE: return "写入失败";
        case SHELLPP_FS_ERR_CLOSE: return "关闭失败";
        case SHELLPP_FS_ERR_SEEK: return "定位失败";
        case SHELLPP_FS_ERR_TOO_LARGE: return "文件过大";
        case SHELLPP_FS_ERR_RENAME: return "重命名失败";
        case SHELLPP_FS_ERR_DELETE: return "删除失败";
        case SHELLPP_FS_ERR_DIRECTORY: return "目录读取失败";
        case SHELLPP_FS_ERR_NOT_EDITABLE: return "文件不可编辑";
        case SHELLPP_FS_ERR_SAME_PATH: return "源与目标相同";
        case SHELLPP_FS_ERR_TRUNCATED: return "复制长度不一致";
        case SHELLPP_FS_ERR_UNSAFE_TYPE: return "拒绝不安全类型";
        case SHELLPP_FS_ERR_EXISTS: return "目标已存在，未覆盖";
        default: return "操作失败";
    }
}

static void set_operation_status(const char *operation, int result) {
    char *cursor = g_status;
    char *end = g_status + sizeof(g_status);
    *cursor = '\0';
    cursor = append_text(cursor, end, operation);
    cursor = append_text(cursor, end, result == SHELLPP_FS_OK ? "完成" : "：");
    if (result != SHELLPP_FS_OK) (void)append_text(cursor, end, fs_error_text(result));
}

static const char g_app_visible_path[] = "/data/apps.json";
static const char g_app_hidden_path[] = "/data/apps.json_hide";

static int app_writer_bytes(struct shellpp_fs_atomic_writer *writer,
        const uint8_t *value, uint32_t length) {
    if (!writer || (!value && length)) return SHELLPP_FS_ERR_ARGUMENT;
    return shellpp_fs_atomic_write(writer, value, length);
}

static int app_writer_char(struct shellpp_fs_atomic_writer *writer,
        uint8_t value) {
    return app_writer_bytes(writer, &value, 1u);
}

static int app_writer_text(struct shellpp_fs_atomic_writer *writer,
        const char *value) {
    uint32_t length;
    if (!writer || !value) return SHELLPP_FS_ERR_ARGUMENT;
    length = text_length(value, APP_TEXT_CAP);
    if (length >= APP_TEXT_CAP) return SHELLPP_FS_ERR_ARGUMENT;
    return app_writer_bytes(writer, (const uint8_t *)value, length);
}

static int app_writer_json_string(struct shellpp_fs_atomic_writer *writer,
        const char *value) {
    uint32_t index;
    int result;
    if (!value) return SHELLPP_FS_ERR_ARGUMENT;
    result = app_writer_char(writer, '"');
    if (result != SHELLPP_FS_OK) return result;
    for (index = 0u; index < APP_TEXT_CAP && value[index]; ++index) {
        uint8_t byte = (uint8_t)value[index];
        if (byte < 0x20u) return SHELLPP_FS_ERR_ARGUMENT;
        if (byte == '"' || byte == '\\') {
            result = app_writer_char(writer, '\\');
            if (result != SHELLPP_FS_OK) return result;
        }
        result = app_writer_char(writer, byte);
        if (result != SHELLPP_FS_OK) return result;
    }
    if (index >= APP_TEXT_CAP) return SHELLPP_FS_ERR_ARGUMENT;
    return app_writer_char(writer, '"');
}

struct app_json_reader {
    struct shellpp_fs_reader reader;
    uint8_t block[96];
    uint32_t cursor;
    uint32_t length;
    const uint8_t *memory;
    uint32_t memory_cursor;
    uint32_t memory_length;
    uint32_t position;
    uint32_t limit;
    uint8_t pushed;
    uint8_t has_pushed;
    uint8_t last;
    uint8_t has_last;
};

static uint8_t app_text_equal(const char *left, const char *right) {
    uint32_t index = 0u;
    if (!left || !right) return 0u;
    do {
        if (left[index] != right[index]) return 0u;
    } while (left[index++] != '\0');
    return 1u;
}

static int app_json_open(struct app_json_reader *json, const char *path) {
    if (!json) return SHELLPP_FS_ERR_ARGUMENT;
    clear_bytes(json, sizeof(*json));
    json->reader.fd = -1;
    json->limit = 0xffffffffu;
    return shellpp_fs_reader_open(path, &json->reader);
}

static void app_json_open_memory(struct app_json_reader *json,
        const uint8_t *memory, uint32_t length) {
    clear_bytes(json, sizeof(*json));
    json->reader.fd = -1;
    json->memory = memory;
    json->memory_length = length;
    json->limit = length;
}

static void app_json_close(struct app_json_reader *json) {
    if (json) shellpp_fs_reader_close(&json->reader);
}

/* Return one byte, zero for EOF, and -1 for an I/O error. */
static int app_json_get(struct app_json_reader *json, uint8_t *value) {
    int result;
    uint32_t read_count = 0u;
    if (!json || !value) return -1;
    if (json->position >= json->limit) return 0;
    if (json->has_pushed) {
        *value = json->pushed;
        json->has_pushed = 0u;
        json->last = *value;
        json->has_last = 1u;
        ++json->position;
        return 1;
    }
    if (json->memory) {
        if (json->memory_cursor >= json->memory_length) return 0;
        *value = json->memory[json->memory_cursor++];
        json->last = *value;
        json->has_last = 1u;
        ++json->position;
        return 1;
    }
    if (json->cursor >= json->length) {
        result = shellpp_fs_reader_read(&json->reader, json->block,
            sizeof(json->block), &read_count);
        if (result != SHELLPP_FS_OK) return -1;
        json->cursor = 0u;
        json->length = read_count;
        if (!read_count) return 0;
    }
    *value = json->block[json->cursor++];
    json->last = *value;
    json->has_last = 1u;
    ++json->position;
    return 1;
}

static int app_json_unget(struct app_json_reader *json) {
    if (!json || !json->has_last || json->has_pushed || !json->position)
        return -1;
    json->pushed = json->last;
    json->has_pushed = 1u;
    --json->position;
    return 0;
}

static int app_json_next_nonspace(struct app_json_reader *json,
        uint8_t *value) {
    int result;
    do {
        result = app_json_get(json, value);
        if (result <= 0) return result;
    } while (*value == ' ' || *value == '\t' || *value == '\r' ||
        *value == '\n');
    return 1;
}

static int app_json_peek_nonspace(struct app_json_reader *json,
        uint8_t *value) {
    int result = app_json_next_nonspace(json, value);
    if (result <= 0) return result;
    return app_json_unget(json) < 0 ? -1 : 1;
}

static int app_json_expect(struct app_json_reader *json, uint8_t expected) {
    uint8_t value;
    int result = app_json_next_nonspace(json, &value);
    return result == 1 && value == expected ? 0 : -1;
}

static void app_json_put_char(char *output, uint32_t capacity,
        uint32_t *length, uint8_t *truncated, uint8_t value) {
    if (output && capacity && *length + 1u < capacity)
        output[(*length)++] = (char)value;
    else
        *truncated = 1u;
}

static int app_json_read_string_open(struct app_json_reader *json,
        char *output, uint32_t capacity) {
    uint8_t value;
    uint8_t truncated = 0u;
    uint32_t length = 0u;
    int result;
    if (output && capacity) output[0] = '\0';
    for (;;) {
        result = app_json_get(json, &value);
        if (result != 1) return -1;
        if (value == '"') break;
        if (value != '\\') {
            if (value < 0x20u) return -1;
            app_json_put_char(output, capacity, &length, &truncated, value);
            continue;
        }
        result = app_json_get(json, &value);
        if (result != 1) return -1;
        if (value == '"' || value == '\\' || value == '/') {
            app_json_put_char(output, capacity, &length, &truncated, value);
        } else if (value == 'b' || value == 'f' || value == 'n' ||
                value == 'r' || value == 't') {
            app_json_put_char(output, capacity, &length, &truncated, ' ');
        } else if (value == 'u') {
            uint32_t codepoint = 0u;
            uint32_t index;
            for (index = 0u; index < 4u; ++index) {
                result = app_json_get(json, &value);
                if (result != 1) return -1;
                codepoint <<= 4;
                if (value >= '0' && value <= '9')
                    codepoint |= (uint32_t)(value - '0');
                else if (value >= 'a' && value <= 'f')
                    codepoint |= (uint32_t)(value - 'a' + 10u);
                else if (value >= 'A' && value <= 'F')
                    codepoint |= (uint32_t)(value - 'A' + 10u);
                else
                    return -1;
            }
            app_json_put_char(output, capacity, &length, &truncated,
                codepoint >= 0x20u && codepoint < 0x7fu ?
                    (uint8_t)codepoint : (uint8_t)'?');
        } else {
            return -1;
        }
    }
    if (output && capacity) output[length] = '\0';
    return truncated ? 1 : 0;
}

static int app_json_read_string_value(struct app_json_reader *json,
        char *output, uint32_t capacity) {
    uint8_t value;
    int result = app_json_next_nonspace(json, &value);
    if (result != 1 || value != '"') return -1;
    return app_json_read_string_open(json, output, capacity);
}

static int app_json_parse_bool(struct app_json_reader *json,
        uint8_t *output) {
    static const uint8_t true_tail[] = { 'r', 'u', 'e' };
    static const uint8_t false_tail[] = { 'a', 'l', 's', 'e' };
    const uint8_t *tail;
    uint32_t count;
    uint32_t index;
    uint8_t value;
    int result = app_json_next_nonspace(json, &value);
    if (result != 1) return -1;
    if (value == 't') {
        *output = 1u;
        tail = true_tail;
        count = sizeof(true_tail);
    } else if (value == 'f') {
        *output = 0u;
        tail = false_tail;
        count = sizeof(false_tail);
    } else {
        return -1;
    }
    for (index = 0u; index < count; ++index) {
        result = app_json_get(json, &value);
        if (result != 1 || value != tail[index]) return -1;
    }
    return 0;
}

/* Copy a complete JSON value while preserving all bytes inside the value.
 * The caller owns object/array separators, so this leaves ',' and the parent
 * closing delimiter unread.  A null writer performs the same bounded parse
 * without emitting data. */
static int app_json_copy_value(struct app_json_reader *json,
        struct shellpp_fs_atomic_writer *writer) {
    uint8_t value;
    uint8_t in_string = 0u;
    uint8_t escaped = 0u;
    uint32_t depth = 0u;
    int result = app_json_next_nonspace(json, &value);
    if (result != 1) return -1;
    if (writer && app_writer_char(writer, value) != SHELLPP_FS_OK) return -1;
    if (value == '"') {
        in_string = 1u;
    } else if (value == '{' || value == '[') {
        depth = 1u;
    } else {
        for (;;) {
            result = app_json_get(json, &value);
            if (result == 0) return 0;
            if (result < 0) return -1;
            if (value == ',' || value == '}' || value == ']')
                return app_json_unget(json) < 0 ? -1 : 0;
            if (writer && app_writer_char(writer, value) != SHELLPP_FS_OK)
                return -1;
        }
    }
    for (;;) {
        result = app_json_get(json, &value);
        if (result != 1) return -1;
        if (writer && app_writer_char(writer, value) != SHELLPP_FS_OK) return -1;
        if (in_string) {
            if (escaped) {
                escaped = 0u;
            } else if (value == '\\') {
                escaped = 1u;
            } else if (value == '"') {
                if (!depth) return 0;
                in_string = 0u;
            } else if (value < 0x20u) {
                return -1;
            }
            continue;
        }
        if (value == '"') {
            in_string = 1u;
        } else if (value == '{' || value == '[') {
            ++depth;
        } else if (value == '}' || value == ']') {
            if (!depth) return -1;
            --depth;
            if (!depth) return 0;
        }
    }
}

static int app_json_skip_value(struct app_json_reader *json) {
    return app_json_copy_value(json, 0);
}

/* Parse object and array separators in one place. Keeping this parser
 * streaming means a long app list never needs a second large native buffer. */
static int app_json_object_next_key(struct app_json_reader *json,
        uint8_t *first, char *key, uint32_t key_capacity) {
    uint8_t value;
    int result;
    if (!json || !first || !key || !key_capacity) return -1;
    result = app_json_peek_nonspace(json, &value);
    if (result != 1) return -1;
    if (value == '}') return app_json_expect(json, '}') < 0 ? -1 : 0;
    if (!*first && app_json_expect(json, ',') < 0) return -1;
    if (app_json_expect(json, '"') < 0 ||
            app_json_read_string_open(json, key, key_capacity) != 0 ||
            app_json_expect(json, ':') < 0)
        return -1;
    *first = 0u;
    return 1;
}

static int app_json_array_next(struct app_json_reader *json, uint8_t *first) {
    uint8_t value;
    int result;
    if (!json || !first) return -1;
    result = app_json_peek_nonspace(json, &value);
    if (result != 1) return -1;
    if (value == ']') return app_json_expect(json, ']') < 0 ? -1 : 0;
    if (!*first && app_json_expect(json, ',') < 0) return -1;
    *first = 0u;
    return 1;
}

static const char *app_item_name(uint32_t index) {
    if (index >= g_app.total || !g_app_items[index].valid ||
            g_app_items[index].name_offset >= g_app.workspace_used)
        return g_empty;
    return g_app_text + g_app_items[index].name_offset;
}

static const char *app_item_package(uint32_t index) {
    if (index >= g_app.total || !g_app_items[index].valid ||
            g_app_items[index].package_offset >= g_app.workspace_used)
        return g_empty;
    return g_app_text + g_app_items[index].package_offset;
}

static uint8_t app_package_protected(const char *package_name) {
    /* The old Xiaomi Vela Shell++ is intentionally manageable. Keep only
     * this resident native app protected from removing its own callbacks. */
    return app_text_equal(package_name, "com.shellpp.ii");
}

static uint8_t app_item_locked(uint32_t index) {
    if (index >= g_app.total || !g_app_items[index].valid) return 1u;
    return g_app_items[index].locked ||
        app_package_protected(app_item_package(index));
}

static uint8_t app_item_selected(uint32_t index) {
    if (index >= g_app.total) return 0u;
    return (g_app.selected[index >> 3] & (uint8_t)(1u << (index & 7u))) != 0u;
}

static void app_set_selected(uint32_t index, uint8_t selected) {
    uint8_t mask;
    if (index >= g_app.total || app_item_locked(index)) return;
    mask = (uint8_t)(1u << (index & 7u));
    if (selected) {
        if (!(g_app.selected[index >> 3] & mask)) {
            g_app.selected[index >> 3] |= mask;
            ++g_app.selected_count;
        }
    } else if (g_app.selected[index >> 3] & mask) {
        g_app.selected[index >> 3] &= (uint8_t)~mask;
        if (g_app.selected_count) --g_app.selected_count;
    }
}

static void app_clear_selection(void) {
    clear_bytes(g_app.selected, sizeof(g_app.selected));
    g_app.selected_count = 0u;
    g_app.delete_armed = 0u;
}

static uint16_t app_selectable_count(void) {
    uint32_t index;
    uint16_t count = 0u;
    for (index = 0u; index < g_app.total; ++index)
        if (!app_item_locked(index)) ++count;
    return count;
}

static void app_clear_items(void) {
    clear_bytes(g_app_items, sizeof(g_list.apps));
    clear_bytes(g_app_text, APP_TEXT_CAP);
    g_app.workspace_used = 0u;
    g_app.total = 0u;
    g_app.page = 0u;
    g_app.loaded = 0u;
    g_app_truncated = 0u;
    app_clear_selection();
}

static void app_copy_display_name(char *output, const char *source) {
    uint32_t index = 0u;
    if (!output) return;
    if (!source) {
        output[0] = '\0';
        return;
    }
    while (index + 1u < APP_NAME_CAP && source[index]) {
        output[index] = source[index];
        ++index;
    }
    output[index] = '\0';
}

static void app_add_item(const char *package_name, const char *name,
        uint8_t hidden, uint8_t locked, uint8_t source_index) {
    uint32_t package_length;
    uint32_t name_length;
    uint32_t index;
    struct app_item_meta *item;
    if (!package_name || !package_name[0]) return;
    package_length = text_length(package_name, APP_PACKAGE_CAP);
    if (package_length >= APP_PACKAGE_CAP) {
        g_app_truncated = 1u;
        return;
    }
    for (index = 0u; index < g_app.total; ++index) {
        if (g_app_items[index].valid &&
                app_text_equal(app_item_package(index), package_name))
            return;
    }
    if (!name || !name[0]) name = package_name;
    name_length = text_length(name, APP_NAME_CAP);
    if (name_length >= APP_NAME_CAP) {
        g_app_truncated = 1u;
        return;
    }
    if (g_app.total >= APP_MAX_ITEMS ||
            g_app.workspace_used + name_length + package_length + 2u >
                APP_TEXT_CAP) {
        g_app_truncated = 1u;
        return;
    }
    item = &g_app_items[g_app.total];
    item->name_offset = (uint16_t)g_app.workspace_used;
    for (index = 0u; index < name_length; ++index)
        g_app_text[g_app.workspace_used + index] = name[index];
    g_app.workspace_used += name_length;
    g_app_text[g_app.workspace_used++] = '\0';
    item->package_offset = (uint16_t)g_app.workspace_used;
    for (index = 0u; index < package_length; ++index)
        g_app_text[g_app.workspace_used + index] = package_name[index];
    g_app.workspace_used += package_length;
    g_app_text[g_app.workspace_used++] = '\0';
    item->hidden = hidden ? 1u : 0u;
    /* Historical registry data can mark the previous Shell++ as locked.
     * That policy belongs to the old synchronizer and must not carry into
     * this native manager. Other firmware-locked entries stay protected. */
    item->locked = app_text_equal(package_name, "com.shell.liangyi") ? 0u :
        ((locked || app_package_protected(package_name)) ? 1u : 0u);
    item->valid = 1u;
    item->source_index = source_index;
    ++g_app.total;
}

static int app_capture_put(uint8_t *output, uint32_t capacity,
        uint32_t *length, uint8_t value) {
    if (!output || !length || *length >= capacity)
        return SHELLPP_FS_ERR_TOO_LARGE;
    output[(*length)++] = value;
    return SHELLPP_FS_OK;
}

/* Capture one registry entry before deciding whether it is selected.  The
 * object workspace is bounded deliberately: an oversized entry aborts a
 * mutating operation instead of risking a lossy registry rewrite. */
static int app_json_capture_value(struct app_json_reader *json,
        uint8_t *output, uint32_t capacity, uint32_t *length) {
    uint8_t value;
    uint8_t in_string = 0u;
    uint8_t escaped = 0u;
    uint32_t depth = 0u;
    int result;
    if (!output || !length) return SHELLPP_FS_ERR_ARGUMENT;
    *length = 0u;
    result = app_json_next_nonspace(json, &value);
    if (result != 1 || app_capture_put(output, capacity, length, value) !=
            SHELLPP_FS_OK) return SHELLPP_FS_ERR_ARGUMENT;
    if (value == '"') {
        in_string = 1u;
    } else if (value == '{' || value == '[') {
        depth = 1u;
    } else {
        for (;;) {
            result = app_json_get(json, &value);
            if (result == 0) return SHELLPP_FS_OK;
            if (result < 0) return SHELLPP_FS_ERR_READ;
            if (value == ',' || value == '}' || value == ']')
                return app_json_unget(json) < 0 ? SHELLPP_FS_ERR_ARGUMENT :
                    SHELLPP_FS_OK;
            if (app_capture_put(output, capacity, length, value) !=
                    SHELLPP_FS_OK) return SHELLPP_FS_ERR_TOO_LARGE;
        }
    }
    for (;;) {
        result = app_json_get(json, &value);
        if (result != 1) return SHELLPP_FS_ERR_ARGUMENT;
        if (app_capture_put(output, capacity, length, value) !=
                SHELLPP_FS_OK) return SHELLPP_FS_ERR_TOO_LARGE;
        if (in_string) {
            if (escaped) {
                escaped = 0u;
            } else if (value == '\\') {
                escaped = 1u;
            } else if (value == '"') {
                if (!depth) return SHELLPP_FS_OK;
                in_string = 0u;
            } else if (value < 0x20u) {
                return SHELLPP_FS_ERR_ARGUMENT;
            }
            continue;
        }
        if (value == '"') {
            in_string = 1u;
        } else if (value == '{' || value == '[') {
            ++depth;
        } else if (value == '}' || value == ']') {
            if (!depth) return SHELLPP_FS_ERR_ARGUMENT;
            --depth;
            if (!depth) return SHELLPP_FS_OK;
        }
    }
}

/* Reads only list metadata.  The captured JSON itself remains unchanged and
 * is copied back verbatim unless the user explicitly moves or removes it. */
static int app_parse_captured_item(const uint8_t *data, uint32_t length,
        char *package_name, char *display_name, uint8_t *locked) {
    struct app_json_reader json;
    char key[APP_JSON_KEY_CAP];
    char candidate[APP_NAME_CAP];
    uint8_t first = 1u;
    uint8_t priority = 0xffu;
    uint8_t byte;
    int result;
    if (!data || !length || !package_name || !display_name || !locked)
        return SHELLPP_FS_ERR_ARGUMENT;
    package_name[0] = '\0';
    display_name[0] = '\0';
    *locked = 0u;
    app_json_open_memory(&json, data, length);
    if (app_json_expect(&json, '{') < 0) return SHELLPP_FS_ERR_ARGUMENT;
    for (;;) {
        uint8_t candidate_priority;
        result = app_json_object_next_key(&json, &first, key, sizeof(key));
        if (result < 0) return SHELLPP_FS_ERR_ARGUMENT;
        if (!result) break;
        if (app_text_equal(key, "package")) {
            result = app_json_read_string_value(&json, package_name,
                APP_PACKAGE_CAP);
            if (result != 0 || !package_name[0])
                return SHELLPP_FS_ERR_ARGUMENT;
        } else if (app_text_equal(key, "name") ||
                app_text_equal(key, "appName") ||
                app_text_equal(key, "label") ||
                app_text_equal(key, "title")) {
            candidate_priority = app_text_equal(key, "name") ? 0u :
                (app_text_equal(key, "appName") ? 1u :
                (app_text_equal(key, "label") ? 2u : 3u));
            result = app_json_read_string_value(&json, candidate,
                sizeof(candidate));
            if (result < 0) return SHELLPP_FS_ERR_ARGUMENT;
            if (result == 0 && candidate[0] && candidate_priority < priority) {
                app_copy_display_name(display_name, candidate);
                priority = candidate_priority;
            }
        } else if (app_text_equal(key, "locked")) {
            if (app_json_parse_bool(&json, locked) < 0)
                return SHELLPP_FS_ERR_ARGUMENT;
        } else if (app_json_skip_value(&json) < 0) {
            return SHELLPP_FS_ERR_ARGUMENT;
        }
    }
    if (!package_name[0]) return SHELLPP_FS_ERR_ARGUMENT;
    if (!display_name[0]) app_copy_display_name(display_name, package_name);
    result = app_json_next_nonspace(&json, &byte);
    return result == 0 ? SHELLPP_FS_OK : SHELLPP_FS_ERR_ARGUMENT;
}

static int app_load_registry_array(struct app_json_reader *json,
        uint8_t source_index) {
    uint8_t first = 1u;
    int result;
    if (app_json_expect(json, '[') < 0) return SHELLPP_FS_ERR_ARGUMENT;
    for (;;) {
        char package_name[APP_PACKAGE_CAP];
        char display_name[APP_NAME_CAP];
        uint8_t locked;
        uint32_t object_length;
        result = app_json_array_next(json, &first);
        if (result < 0) return SHELLPP_FS_ERR_ARGUMENT;
        if (!result) return SHELLPP_FS_OK;
        result = app_json_capture_value(json, g_app_object, APP_OBJECT_CAP,
            &object_length);
        if (result != SHELLPP_FS_OK) return result;
        if (!object_length || g_app_object[0] != '{' ||
                app_parse_captured_item(g_app_object, object_length,
                    package_name, display_name, &locked) != SHELLPP_FS_OK) {
            /* Keep malformed or non-app records untouched on disk, but do not
             * expose them to bulk operations through an incomplete list. */
            g_app_truncated = 1u;
            continue;
        }
        app_add_item(package_name, display_name,
            source_index == APP_SOURCE_HIDDEN, locked, source_index);
    }
}

static int app_registry_file_state(const char *path, uint8_t *exists) {
    uint8_t type;
    int result;
    if (!exists) return SHELLPP_FS_ERR_ARGUMENT;
    result = shellpp_fs_path_type(path, exists, &type);
    if (result != SHELLPP_FS_OK || !*exists) return result;
    return type == FS_TYPE_REGULAR ? SHELLPP_FS_OK : SHELLPP_FS_ERR_UNSAFE_TYPE;
}

static int app_load_registry_items(const char *path, uint8_t source_index,
        uint8_t optional) {
    struct app_json_reader json;
    char key[APP_JSON_KEY_CAP];
    uint8_t first = 1u;
    uint8_t found_items = 0u;
    uint8_t exists;
    int result;
    result = app_registry_file_state(path, &exists);
    if (result != SHELLPP_FS_OK) return result;
    if (!exists) return optional ? SHELLPP_FS_OK : SHELLPP_FS_ERR_OPEN;
    result = app_json_open(&json, path);
    if (result != SHELLPP_FS_OK) return result;
    if (app_json_expect(&json, '{') < 0) result = SHELLPP_FS_ERR_ARGUMENT;
    else {
        result = SHELLPP_FS_OK;
        for (;;) {
            int next = app_json_object_next_key(&json, &first, key,
                sizeof(key));
            if (next < 0) { result = SHELLPP_FS_ERR_ARGUMENT; break; }
            if (!next) break;
            if (app_text_equal(key, "InstalledApps")) {
                if (found_items) { result = SHELLPP_FS_ERR_ARGUMENT; break; }
                found_items = 1u;
                result = app_load_registry_array(&json, source_index);
            } else if (app_json_skip_value(&json) < 0) {
                result = SHELLPP_FS_ERR_ARGUMENT;
            }
            if (result != SHELLPP_FS_OK) break;
        }
    }
    app_json_close(&json);
    if (result != SHELLPP_FS_OK || !found_items)
        return result != SHELLPP_FS_OK ? result : SHELLPP_FS_ERR_ARGUMENT;
    return SHELLPP_FS_OK;
}

static int app_reload_direct(void) {
    int result;
    app_clear_items();
    result = app_load_registry_items(g_app_visible_path, APP_SOURCE_VISIBLE,
        0u);
    if (result == SHELLPP_FS_OK)
        result = app_load_registry_items(g_app_hidden_path, APP_SOURCE_HIDDEN,
            1u);
    if (result != SHELLPP_FS_OK) {
        app_clear_items();
        set_operation_status("应用注册表读取", result);
        return result;
    }
    g_app.loaded = 1u;
    if (g_app_truncated)
        set_status("应用列表已读取，部分条目不可管理");
    else
        set_status("应用列表已读取");
    return SHELLPP_FS_OK;
}

static uint8_t app_item_is_target(uint32_t index, uint8_t operation,
        uint8_t all) {
    const struct app_item_meta *item;
    if (index >= g_app.total || !(item = &g_app_items[index])->valid ||
            app_item_locked(index)) return 0u;
    if (operation == APP_OPERATION_HIDE &&
            item->source_index != APP_SOURCE_VISIBLE) return 0u;
    if (operation == APP_OPERATION_SHOW &&
            item->source_index != APP_SOURCE_HIDDEN) return 0u;
    return all || app_item_selected(index);
}

static uint8_t app_package_is_target(const char *package_name,
        uint8_t source_index, uint8_t operation, uint8_t all) {
    uint32_t index;
    for (index = 0u; index < g_app.total; ++index) {
        if (g_app_items[index].source_index == source_index &&
                app_item_is_target(index, operation, all) &&
                app_text_equal(app_item_package(index), package_name))
            return 1u;
    }
    return 0u;
}

static uint16_t app_target_count(uint8_t operation, uint8_t all) {
    uint32_t index;
    uint16_t count = 0u;
    for (index = 0u; index < g_app.total; ++index)
        if (app_item_is_target(index, operation, all)) ++count;
    return count;
}

static int app_write_moved_object(struct shellpp_fs_atomic_writer *writer,
        const uint8_t *data, uint32_t length, uint8_t make_hidden) {
    struct app_json_reader json;
    char key[APP_JSON_KEY_CAP];
    uint8_t first = 1u;
    uint8_t output_first = 1u;
    uint8_t byte;
    int result;
    if (!writer || !data || !length) return SHELLPP_FS_ERR_ARGUMENT;
    app_json_open_memory(&json, data, length);
    if (app_json_expect(&json, '{') < 0 ||
            app_writer_char(writer, '{') != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_ARGUMENT;
    for (;;) {
        int next = app_json_object_next_key(&json, &first, key, sizeof(key));
        if (next < 0) return SHELLPP_FS_ERR_ARGUMENT;
        if (!next) break;
        if (app_text_equal(key, "hideFlag")) {
            if (app_json_skip_value(&json) < 0) return SHELLPP_FS_ERR_ARGUMENT;
            continue;
        }
        if (!output_first && app_writer_char(writer, ',') != SHELLPP_FS_OK)
            return SHELLPP_FS_ERR_WRITE;
        output_first = 0u;
        result = app_writer_json_string(writer, key);
        if (result == SHELLPP_FS_OK) result = app_writer_char(writer, ':');
        if (result != SHELLPP_FS_OK || app_json_copy_value(&json, writer) < 0)
            return SHELLPP_FS_ERR_WRITE;
    }
    if (make_hidden) {
        if (!output_first && app_writer_char(writer, ',') != SHELLPP_FS_OK)
            return SHELLPP_FS_ERR_WRITE;
        result = app_writer_text(writer, "\"hideFlag\":true");
        if (result != SHELLPP_FS_OK) return result;
    }
    if (app_writer_char(writer, '}') != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_WRITE;
    result = app_json_next_nonspace(&json, &byte);
    return result == 0 ? SHELLPP_FS_OK : SHELLPP_FS_ERR_ARGUMENT;
}

/* Copies one InstalledApps array without the closing bracket.  The caller can
 * then append moved entries before closing it, which makes target-first moves
 * recoverable if the source rewrite fails. */
static int app_write_registry_array_entries(struct app_json_reader *json,
        struct shellpp_fs_atomic_writer *writer, uint8_t source_index,
        uint8_t operation, uint8_t all, uint8_t remove_selected,
        uint8_t *output_empty, uint16_t *changed) {
    uint8_t first = 1u;
    int result;
    if (!writer || !output_empty || !changed || app_json_expect(json, '[') < 0)
        return SHELLPP_FS_ERR_ARGUMENT;
    for (;;) {
        int next = app_json_array_next(json, &first);
        if (next < 0) return SHELLPP_FS_ERR_ARGUMENT;
        if (!next) return SHELLPP_FS_OK;
        if (remove_selected) {
            char package_name[APP_PACKAGE_CAP];
            char display_name[APP_NAME_CAP];
            uint8_t locked;
            uint32_t object_length;
            uint8_t remove = 0u;
            result = app_json_capture_value(json, g_app_object, APP_OBJECT_CAP,
                &object_length);
            if (result != SHELLPP_FS_OK) return result;
            if (object_length && g_app_object[0] == '{' &&
                    app_parse_captured_item(g_app_object, object_length,
                        package_name, display_name, &locked) == SHELLPP_FS_OK)
                remove = app_package_is_target(package_name, source_index,
                    operation, all);
            if (remove) {
                ++*changed;
                continue;
            }
            if (!*output_empty && app_writer_char(writer, ',') != SHELLPP_FS_OK)
                return SHELLPP_FS_ERR_WRITE;
            *output_empty = 0u;
            if (app_writer_bytes(writer, g_app_object, object_length) !=
                    SHELLPP_FS_OK) return SHELLPP_FS_ERR_WRITE;
        } else {
            if (!*output_empty && app_writer_char(writer, ',') != SHELLPP_FS_OK)
                return SHELLPP_FS_ERR_WRITE;
            *output_empty = 0u;
            if (app_json_copy_value(json, writer) < 0)
                return SHELLPP_FS_ERR_WRITE;
        }
    }
}

static int app_append_selected_from_registry(
        struct shellpp_fs_atomic_writer *writer, const char *path,
        uint8_t source_index, uint8_t operation, uint8_t all,
        uint8_t make_hidden, uint8_t *output_empty, uint16_t *changed) {
    struct app_json_reader json;
    char key[APP_JSON_KEY_CAP];
    uint8_t root_first = 1u;
    uint8_t found_items = 0u;
    uint8_t exists;
    int result;
    result = app_registry_file_state(path, &exists);
    if (result != SHELLPP_FS_OK || !exists)
        return result != SHELLPP_FS_OK ? result : SHELLPP_FS_ERR_OPEN;
    result = app_json_open(&json, path);
    if (result != SHELLPP_FS_OK) return result;
    if (app_json_expect(&json, '{') < 0) result = SHELLPP_FS_ERR_ARGUMENT;
    else {
        result = SHELLPP_FS_OK;
        for (;;) {
            int next = app_json_object_next_key(&json, &root_first, key,
                sizeof(key));
            if (next < 0) { result = SHELLPP_FS_ERR_ARGUMENT; break; }
            if (!next) break;
            if (app_text_equal(key, "InstalledApps")) {
                uint8_t array_first = 1u;
                if (found_items || app_json_expect(&json, '[') < 0) {
                    result = SHELLPP_FS_ERR_ARGUMENT;
                    break;
                }
                found_items = 1u;
                for (;;) {
                    char package_name[APP_PACKAGE_CAP];
                    char display_name[APP_NAME_CAP];
                    uint8_t locked;
                    uint32_t object_length;
                    int array_next = app_json_array_next(&json, &array_first);
                    if (array_next < 0) { result = SHELLPP_FS_ERR_ARGUMENT; break; }
                    if (!array_next) break;
                    result = app_json_capture_value(&json, g_app_object,
                        APP_OBJECT_CAP, &object_length);
                    if (result != SHELLPP_FS_OK) break;
                    if (!object_length || g_app_object[0] != '{' ||
                            app_parse_captured_item(g_app_object, object_length,
                                package_name, display_name, &locked) !=
                                    SHELLPP_FS_OK ||
                            !app_package_is_target(package_name, source_index,
                                operation, all))
                        continue;
                    if (!*output_empty && app_writer_char(writer, ',') !=
                            SHELLPP_FS_OK) { result = SHELLPP_FS_ERR_WRITE; break; }
                    *output_empty = 0u;
                    result = app_write_moved_object(writer, g_app_object,
                        object_length, make_hidden);
                    if (result != SHELLPP_FS_OK) break;
                    ++*changed;
                }
            } else if (app_json_skip_value(&json) < 0) {
                result = SHELLPP_FS_ERR_ARGUMENT;
            }
            if (result != SHELLPP_FS_OK) break;
        }
    }
    app_json_close(&json);
    if (result != SHELLPP_FS_OK || !found_items)
        return result != SHELLPP_FS_OK ? result : SHELLPP_FS_ERR_ARGUMENT;
    return SHELLPP_FS_OK;
}

static int app_rewrite_registry(const char *path, uint8_t source_index,
        uint8_t operation, uint8_t all, uint8_t remove_selected,
        const char *append_path, uint8_t append_source, uint8_t make_hidden,
        uint8_t allow_missing, uint16_t *changed) {
    struct app_json_reader json;
    struct shellpp_fs_atomic_writer writer;
    char key[APP_JSON_KEY_CAP];
    uint8_t input_first = 1u;
    uint8_t output_first = 1u;
    uint8_t found_items = 0u;
    uint8_t exists;
    int result;
    if (!changed) return SHELLPP_FS_ERR_ARGUMENT;
    *changed = 0u;
    result = app_registry_file_state(path, &exists);
    if (result != SHELLPP_FS_OK) return result;
    if (!exists) {
        uint8_t array_empty = 1u;
        if (!allow_missing) return SHELLPP_FS_ERR_OPEN;
        if (!append_path) return SHELLPP_FS_OK;
        writer.fd = -1;
        result = shellpp_fs_atomic_begin(path, &writer);
        if (result != SHELLPP_FS_OK) return result;
        result = app_writer_text(&writer, "{\"InstalledApps\":[");
        if (result == SHELLPP_FS_OK)
            result = app_append_selected_from_registry(&writer, append_path,
                append_source, operation, all, make_hidden, &array_empty,
                changed);
        if (result == SHELLPP_FS_OK && !*changed)
            result = SHELLPP_FS_ERR_ARGUMENT;
        if (result == SHELLPP_FS_OK) result = app_writer_text(&writer, "]}");
        if (result == SHELLPP_FS_OK)
            result = shellpp_fs_atomic_commit(path, &writer);
        else
            shellpp_fs_atomic_abort(path, &writer);
        return result;
    }
    result = app_json_open(&json, path);
    if (result != SHELLPP_FS_OK) return result;
    writer.fd = -1;
    result = shellpp_fs_atomic_begin(path, &writer);
    if (result != SHELLPP_FS_OK) { app_json_close(&json); return result; }
    if (app_json_expect(&json, '{') < 0 ||
            app_writer_char(&writer, '{') != SHELLPP_FS_OK) {
        result = SHELLPP_FS_ERR_ARGUMENT;
    } else {
        result = SHELLPP_FS_OK;
        for (;;) {
            int next = app_json_object_next_key(&json, &input_first, key,
                sizeof(key));
            if (next < 0) { result = SHELLPP_FS_ERR_ARGUMENT; break; }
            if (!next) break;
            if (found_items && app_text_equal(key, "InstalledApps")) {
                result = SHELLPP_FS_ERR_ARGUMENT;
                break;
            }
            if (!output_first && app_writer_char(&writer, ',') != SHELLPP_FS_OK) {
                result = SHELLPP_FS_ERR_WRITE;
                break;
            }
            output_first = 0u;
            result = app_writer_json_string(&writer, key);
            if (result == SHELLPP_FS_OK) result = app_writer_char(&writer, ':');
            if (result != SHELLPP_FS_OK) break;
            if (app_text_equal(key, "InstalledApps")) {
                uint8_t array_empty = 1u;
                found_items = 1u;
                result = app_writer_char(&writer, '[');
                if (result == SHELLPP_FS_OK)
                    result = app_write_registry_array_entries(&json, &writer,
                        source_index, operation, all, remove_selected,
                        &array_empty, changed);
                if (result == SHELLPP_FS_OK && append_path)
                    result = app_append_selected_from_registry(&writer,
                        append_path, append_source, operation, all,
                        make_hidden, &array_empty, changed);
                if (result == SHELLPP_FS_OK)
                    result = app_writer_char(&writer, ']');
            } else if (app_json_copy_value(&json, &writer) < 0) {
                result = SHELLPP_FS_ERR_WRITE;
            }
            if (result != SHELLPP_FS_OK) break;
        }
        if (result == SHELLPP_FS_OK && !found_items)
            result = SHELLPP_FS_ERR_ARGUMENT;
        if (result == SHELLPP_FS_OK && app_writer_char(&writer, '}') !=
                SHELLPP_FS_OK) result = SHELLPP_FS_ERR_WRITE;
    }
    app_json_close(&json);
    if (result == SHELLPP_FS_OK && append_path && !*changed)
        result = SHELLPP_FS_ERR_ARGUMENT;
    if (result == SHELLPP_FS_OK)
        result = shellpp_fs_atomic_commit(path, &writer);
    else
        shellpp_fs_atomic_abort(path, &writer);
    return result;
}

static int app_delete_selected_directories(void) {
    uint32_t index;
    int overall = SHELLPP_FS_OK;
    for (index = 0u; index < g_app.total; ++index) {
        int result;
        if (!app_item_is_target(index, APP_OPERATION_DELETE, 0u)) continue;
        result = shellpp_fs_delete_app_package(app_item_package(index));
        if (result != SHELLPP_FS_OK && overall == SHELLPP_FS_OK)
            overall = result;
    }
    return overall;
}

static void app_set_count_status(const char *verb, uint16_t count,
        const char *suffix) {
    char *cursor = g_status;
    char *end = g_status + sizeof(g_status);
    *cursor = '\0';
    cursor = append_text(cursor, end, verb);
    cursor = append_u32(cursor, end, count);
    (void)append_text(cursor, end, suffix);
}

static void app_refresh_after_mutation(const char *verb, uint16_t count,
        const char *suffix) {
    if (app_reload_direct() == SHELLPP_FS_OK)
        app_set_count_status(verb, count, suffix);
    else
        set_status("注册表已修改，列表刷新失败");
}

static void app_apply_direct(uint8_t operation, uint8_t all) {
    uint16_t requested;
    uint16_t target_changed = 0u;
    uint16_t source_changed = 0u;
    int result;
    if (!g_app.loaded) {
        set_status("应用列表尚未读取");
        return;
    }
    if (all && g_app_truncated) {
        set_status("列表不完整，无法批量操作");
        return;
    }
    requested = app_target_count(operation, all);
    if (!requested) {
        set_status("没有可操作的应用");
        return;
    }
    if (operation == APP_OPERATION_HIDE) {
        result = app_rewrite_registry(g_app_hidden_path, APP_SOURCE_HIDDEN,
            operation, all, 0u, g_app_visible_path, APP_SOURCE_VISIBLE, 1u,
            1u, &target_changed);
        if (result != SHELLPP_FS_OK) {
            set_operation_status("隐藏应用", result);
            return;
        }
        result = app_rewrite_registry(g_app_visible_path, APP_SOURCE_VISIBLE,
            operation, all, 1u, 0, 0u, 0u, 0u, &source_changed);
        if (result != SHELLPP_FS_OK) {
            (void)app_reload_direct();
            set_status("隐藏列表已写入，原列表更新失败");
            return;
        }
        app_refresh_after_mutation("已隐藏 ", source_changed,
            " 个，重启后生效");
        return;
    }
    if (operation == APP_OPERATION_SHOW) {
        result = app_rewrite_registry(g_app_visible_path, APP_SOURCE_VISIBLE,
            operation, all, 0u, g_app_hidden_path, APP_SOURCE_HIDDEN, 0u,
            0u, &target_changed);
        if (result != SHELLPP_FS_OK) {
            set_operation_status("显示应用", result);
            return;
        }
        result = app_rewrite_registry(g_app_hidden_path, APP_SOURCE_HIDDEN,
            operation, all, 1u, 0, 0u, 0u, 1u, &source_changed);
        if (result != SHELLPP_FS_OK) {
            (void)app_reload_direct();
            set_status("显示列表已写入，隐藏列表更新失败");
            return;
        }
        app_refresh_after_mutation("已显示 ", source_changed,
            " 个，重启后生效");
        return;
    }
    result = app_rewrite_registry(g_app_visible_path, APP_SOURCE_VISIBLE,
        APP_OPERATION_DELETE, all, 1u, 0, 0u, 0u, 0u, &target_changed);
    if (result != SHELLPP_FS_OK) {
        set_operation_status("卸载应用", result);
        return;
    }
    result = app_rewrite_registry(g_app_hidden_path, APP_SOURCE_HIDDEN,
        APP_OPERATION_DELETE, all, 1u, 0, 0u, 0u, 1u, &source_changed);
    if (result != SHELLPP_FS_OK) {
        (void)app_reload_direct();
        set_status("显示注册表已更新，隐藏注册表更新失败");
        return;
    }
    result = app_delete_selected_directories();
    if (result == SHELLPP_FS_OK)
        app_refresh_after_mutation("已卸载 ",
            (uint16_t)(target_changed + source_changed), " 个，建议重启");
    else {
        (void)app_reload_direct();
        set_status("注册表已移除，部分应用数据目录未删除");
    }
}

static void app_enter_list_mode(void) {
    /* File editing shares this workspace. It is reclaimed only when the
     * Files page enters the native application manager explicitly. */
    clear_bytes(&g_workspace_storage.app, sizeof(g_workspace_storage.app));
    clear_bytes(g_app_items, sizeof(g_list.apps));
    g_app_mode = APP_MODE_LIST;
    g_browser_owner = 0xffu;
    set_status("正在读取固件应用注册表");
    (void)app_reload_direct();
}

static void app_leave_list_mode(void) {
    app_clear_items();
    g_app_mode = APP_MODE_MENU;
    set_status("返回文件与应用管理");
}

static void app_toggle_selection(uint8_t page_offset) {
    uint32_t index = (uint32_t)g_app.page * APP_PAGE_SIZE + page_offset;
    if (index >= g_app.total || app_item_locked(index)) return;
    app_set_selected(index, app_item_selected(index) ? 0u : 1u);
    g_app.delete_armed = 0u;
    g_app.reboot_armed = 0u;
    app_set_count_status("已选择 ", g_app.selected_count, " 个应用");
}

static void app_toggle_all_selection(void) {
    uint32_t index;
    uint16_t selectable = app_selectable_count();
    uint8_t select = g_app.selected_count != selectable;
    if (!selectable) return;
    for (index = 0u; index < g_app.total; ++index)
        app_set_selected(index, select);
    g_app.delete_armed = 0u;
    g_app.reboot_armed = 0u;
    app_set_count_status(select ? "已全选 ": "已取消选择 ",
        g_app.selected_count, " 个应用");
}

/* Cortex-M33 defines SYSRESETREQ through SCB->AIRCR. This requests a system
 * reset without depending on an unverified firmware function address. Keep
 * XiaomiVela's priority grouping intact while writing the required key. */
static void app_soft_reboot_now(void) __attribute__((noreturn));
static void app_soft_reboot_now(void) {
    RESTART_SOFT();
    for (;;) __asm__ volatile("wfi");
}

static void app_hard_reboot_now(void) __attribute__((noreturn));
static void app_hard_reboot_now(void) {
    uint8_t fa[64];
    uint8_t attr[64];
    uint32_t pid = 0u;
    int status = 0;
    char *argv[4];
    argv[0] = "nsh";
    argv[1] = "-c";
    argv[2] = "reboot";
    argv[3] = 0;
    (void)RESTART_FA_INIT(fa);
    (void)RESTART_ATTR_INIT(attr);
    (void)RESTART_FA_ADDOPEN(fa, 0, "/dev/null", 0, 0u);
    (void)RESTART_FA_ADDOPEN(fa, 1, "/dev/null", (1 << 1), 0u);
    (void)RESTART_FA_ADDOPEN(fa, 2, "/dev/null", (1 << 1), 0u);
    (void)RESTART_SPAWN(&pid, "/bin/nsh", fa, attr, argv, 0);
    (void)RESTART_WAITPID(pid, &status, 0);
    RESTART_FA_DESTROY(fa);
    RESTART_ATTR_DESTROY(attr);
    for (;;) __asm__ volatile("wfi");
}

static void app_request_reboot(void) {
    if (!g_app.reboot_armed) {
        g_app.reboot_armed = 1u;
        set_status("再次点击重启系统以确认");
        return;
    }
    app_soft_reboot_now();
}

static void restart_request(uint8_t soft) {
    uint8_t *armed = soft ? &g_restart_soft_armed : &g_restart_hard_armed;
    if (!*armed) {
        *armed = 1u;
        set_status("再次点击对应按钮以确认");
        return;
    }
    *armed = 0u;
    if (soft) app_soft_reboot_now();
    app_hard_reboot_now();
}

static void app_previous_page(void) {
    if (g_app.page) --g_app.page;
}

static void app_next_page(void) {
    uint16_t page_count = (uint16_t)((g_app.total + APP_PAGE_SIZE - 1u) /
        APP_PAGE_SIZE);
    if (g_app.page + 1u < page_count) ++g_app.page;
}

static void app_delete_selection(void) {
    if (!g_app.selected_count) {
        set_status("请先选择要卸载的应用");
        return;
    }
    if (!g_app.delete_armed) {
        g_app.delete_armed = 1u;
        set_status("再次点击卸载以确认");
        return;
    }
    app_apply_direct(APP_OPERATION_DELETE, 0u);
}

static int load_directory(const char *path,
        const struct shellpp_fs_cursor *after) {
    int result = shellpp_fs_list_page(path, after, &g_directory_page);
    if (result != SHELLPP_FS_OK) {
        set_operation_status("目录读取", result);
        return result;
    }
    if (copy_text(g_current_path, sizeof(g_current_path), path) < 0) {
        set_status("路径过长");
        return SHELLPP_FS_ERR_PATH;
    }
    clear_bytes(&g_after_cursor, sizeof(g_after_cursor));
    if (after) g_after_cursor = *after;
    g_browser_mode = BROWSER_LIST;
    g_delete_armed = 0u;
    set_status("目录已加载");
    if (g_ui[PAGE_VIEWER].active && g_browser_owner == PAGE_VIEWER)
        g_viewer_rebuild_pending = 1u;
    return SHELLPP_FS_OK;
}

static int refresh_directory(void) {
    return load_directory(g_current_path,
        g_after_cursor.valid ? &g_after_cursor : 0);
}

static void start_browser(uint32_t page_index) {
    restore_cut();
    g_browser_owner = (uint8_t)page_index;
    g_browser_read_only = 0u;
    g_browser_mode = BROWSER_LIST;
    g_delete_armed = 0u;
    clear_bytes(&g_after_cursor, sizeof(g_after_cursor));
    (void)copy_text(g_current_path, sizeof(g_current_path), "/");
    if (load_directory("/", 0) != SHELLPP_FS_OK)
        set_status("根目录读取失败");
}

static int select_entry(uint32_t index) {
    const struct shellpp_fs_entry *entry;
    int result;
    if (index >= g_directory_page.count) return SHELLPP_FS_ERR_ARGUMENT;
    entry = &g_directory_page.entries[index];
    result = shellpp_fs_join(g_current_path, entry->name, g_path_buffer,
        sizeof(g_path_buffer));
    if (result != SHELLPP_FS_OK) { set_operation_status("打开", result); return result; }
    if (entry->is_dir) {
        clear_bytes(&g_navigation_cursor, sizeof(g_navigation_cursor));
        return load_directory(g_path_buffer, 0);
    }
    if (copy_text(g_selected_path, sizeof(g_selected_path), g_path_buffer) < 0)
        return SHELLPP_FS_ERR_PATH;
    g_selected_type = entry->type;
    g_selected_is_link = entry->is_link;
    g_selected_size_known = entry->size_known;
    g_selected_size = entry->size;
    if (!g_selected_is_link && !g_selected_size_known &&
            shellpp_fs_file_size(g_selected_path, &g_selected_size, 0) ==
                SHELLPP_FS_OK)
        g_selected_size_known = 1u;
    g_browser_mode = BROWSER_DETAIL;
    g_delete_armed = 0u;
    set_status("文件已选择");
    g_viewer_rebuild_pending = 1u;
    return SHELLPP_FS_OK;
}

static int open_text_view(void) {
    int result;
    restore_cut();
    if (g_selected_is_link) { set_status("符号链接不跟随"); return SHELLPP_FS_ERR_UNSAFE_TYPE; }
    if (!g_selected_size_known || g_selected_size > SHELLPP_FS_VIEW_LIMIT) {
        set_status("文件过大或大小未知");
        return SHELLPP_FS_ERR_TOO_LARGE;
    }
    g_hex_file_offset = 0u;
    result = shellpp_fs_read_at(g_selected_path, g_hex_file_offset,
        g_workspace, SHELLPP_FS_TEXT_LIMIT, &g_workspace_length);
    if (result != SHELLPP_FS_OK) { set_operation_status("文本读取", result); return result; }
    for (uint32_t index = 0u; index < g_workspace_length; ++index)
        if (g_workspace[index] < 32u && g_workspace[index] != 9u &&
                g_workspace[index] != 10u && g_workspace[index] != 13u)
            g_workspace[index] = '.';
    g_workspace[g_workspace_length] = 0u;
    g_text_offset = 0u;
    g_browser_mode = BROWSER_TEXT;
    set_status("文本读取完成");
    return SHELLPP_FS_OK;
}

static int load_text_page(uint32_t offset) {
    int result;
    uint32_t index;
    restore_cut();
    if (offset > g_selected_size) return SHELLPP_FS_ERR_SEEK;
    result = shellpp_fs_read_at(g_selected_path, offset, g_workspace,
        SHELLPP_FS_TEXT_LIMIT, &g_workspace_length);
    if (result != SHELLPP_FS_OK) {
        set_operation_status("文本读取", result);
        return result;
    }
    for (index = 0u; index < g_workspace_length; ++index)
        if (g_workspace[index] < 32u && g_workspace[index] != 9u &&
                g_workspace[index] != 10u && g_workspace[index] != 13u)
            g_workspace[index] = '.';
    g_workspace[g_workspace_length] = 0u;
    g_hex_file_offset = offset;
    g_text_offset = 0u;
    return SHELLPP_FS_OK;
}

static char *append_hex_byte(char *cursor, char *end, uint8_t value) {
    static const char digits[] = "0123456789ABCDEF";
    if (cursor + 2 < end) {
        *cursor++ = digits[value >> 4];
        *cursor++ = digits[value & 0x0fu];
    }
    *cursor = '\0';
    return cursor;
}

static char *append_hex_word(char *cursor, char *end, uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    int32_t shift;
    for (shift = 28; shift >= 0; shift -= 4)
        if (cursor + 1 < end) *cursor++ = digits[(value >> shift) & 0x0fu];
    *cursor = '\0';
    return cursor;
}

static void format_hex_screen(void) {
    char *cursor = (char *)g_workspace;
    char *end = (char *)g_workspace + HEX_RAW_OFFSET;
    const uint8_t *raw = g_workspace + HEX_RAW_OFFSET;
    uint32_t line;
    *cursor = '\0';
    for (line = 0; line < HEX_SCREEN_LINES; ++line) {
        uint32_t offset = g_hex_line_offset + line * 8u;
        uint32_t column;
        if (offset >= g_hex_page_length) break;
        cursor = append_hex_word(cursor, end, g_hex_file_offset + offset);
        cursor = append_text(cursor, end, "  " );
        for (column = 0; column < 8u && offset + column < g_hex_page_length;
                ++column) {
            cursor = append_hex_byte(cursor, end, raw[offset + column]);
            cursor = append_text(cursor, end, " " );
        }
        cursor = append_text(cursor, end, "\n" );
    }
    g_workspace_length = (uint32_t)(cursor - (char *)g_workspace);
}

static int load_hex_page(uint32_t offset) {
    int result;
    restore_cut();
    if (g_selected_is_link) { set_status("符号链接不跟随"); return SHELLPP_FS_ERR_UNSAFE_TYPE; }
    if (!g_selected_size_known || g_selected_size > SHELLPP_FS_VIEW_LIMIT) {
        set_status("文件过大或大小未知");
        return SHELLPP_FS_ERR_TOO_LARGE;
    }
    if (offset > g_selected_size) return SHELLPP_FS_ERR_SEEK;
    result = shellpp_fs_read_at(g_selected_path, offset,
        g_workspace + HEX_RAW_OFFSET, SHELLPP_FS_HEX_PAGE_SIZE,
        &g_hex_page_length);
    if (result != SHELLPP_FS_OK) { set_operation_status("Hex 读取", result); return result; }
    g_hex_file_offset = offset;
    g_hex_line_offset = 0u;
    format_hex_screen();
    g_browser_mode = BROWSER_HEX;
    set_status("Hex 读取完成");
    return SHELLPP_FS_OK;
}

static uint8_t path_is_root(const char *path) {
    return path && path[0] == '/' && path[1] == '\0';
}

static void format_entry_secondary(uint32_t index) {
    const struct shellpp_fs_entry *entry = &g_directory_page.entries[index];
    char *cursor = g_entry_secondary[index];
    char *end = cursor + sizeof(g_entry_secondary[index]);
    *cursor = '\0';
    if (entry->is_dir) {
        (void)append_text(cursor, end, "目录");
    } else if (entry->is_link) {
        (void)append_text(cursor, end, "符号链接 · 不跟随");
    } else {
        cursor = append_text(cursor, end, entry->type == FS_TYPE_REGULAR ?
            "文件 · " : "未知类型 · ");
        format_size(cursor, (uint32_t)(end - cursor), entry->size,
            entry->size_known);
    }
}

static void format_clipboard_secondary(void) {
    const char *name = shellpp_fs_basename(g_clipboard_path);
    char *cursor = g_clipboard_secondary;
    char *end = cursor + sizeof(g_clipboard_secondary);
    *cursor = '\0';
    cursor = append_text(cursor, end,
        g_clipboard_mode == 2u ? "移动 · " : "复制 · ");
    (void)append_text(cursor, end, name ? name : g_clipboard_path);
}

static void render_home(void) {
    struct row_spec specs[7];
    uint32_t count = 0u;
    add_spec(specs, &count, "文件与应用管理", "文件查看与缓存清理",
        ACTION_FILE_GROUP_OPEN, PAGE_FILES, 1u);
    add_spec(specs, &count, "CPU跑分", "综合性能测试",
        ACTION_CPU_BENCH_PAGE_OPEN, PAGE_CPU_BENCH, 1u);
    add_spec(specs, &count, "震动测试", "官方震动模式与增强档位",
        ACTION_VIBRATION_PAGE_OPEN, PAGE_VIBRATION, 1u);
    add_spec(specs, &count, "显示", "系统性能信息显示",
        ACTION_DISPLAY_OPEN, PAGE_FILES, 1u);
    add_spec(specs, &count, "关于", "关于 Shell++ II",
        ACTION_NAVIGATE, PAGE_ABOUT, 1u);
    add_spec(specs, &count, "重启", "硬重启或软重启系统",
        ACTION_NAVIGATE, PAGE_RESTART, 1u);
    apply_specs(PAGE_HOME, specs, count, 0, 0, 0);
}

static void render_restart(void) {
    struct row_spec specs[1];
    uint32_t count = 0u;
    add_spec(specs, &count,
        g_restart_soft_armed ? "再次确认重载系统" : "重载系统",
        g_empty,
        ACTION_RESTART_SOFT, 0u, 1u);
    apply_specs(PAGE_RESTART, specs, count, 0, 0, 0);
}

static void render_about(void) {
    struct row_spec specs[5];
    uint32_t count = 0u;
    add_spec(specs, &count, "Shell++ II", "Beta1", ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "com.shellpp.ii", "包名", ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "系统固件", SHELLPP_TARGET_FIRMWARE_VERSION,
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "开发人员", "@IKUN_CXKPRO", ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "运行模式", shellpp_native_run_mode(),
        ACTION_NONE, 0u, 0u);
    apply_specs(PAGE_ABOUT, specs, count, 0, 0, 0);
}

static void render_display(void) {
    struct row_spec specs[2];
    uint32_t count = 0u;
    add_spec(specs, &count, "CPU占用显示", "CPU占用检测与悬浮显示",
        ACTION_CPU_PAGE_OPEN, PAGE_CPU, 1u);
    add_spec(specs, &count, "内存占用显示", "内存检测与悬浮显示",
        ACTION_MEMORY_PAGE_OPEN, 0u, 1u);
    apply_specs(PAGE_DISPLAY, specs, count, 0, 0, 0);
}

static void render_cpu(void) {
    struct row_spec specs[3];
    uint32_t count = 0u;
    add_spec(specs, &count, "CPU占用检测",
        g_cpu_monitor_enabled ? "检测：已开启 · 500ms" : "检测：已关闭",
        ACTION_CPU_MONITOR, 0u, 1u);
    add_spec(specs, &count, "CPU悬浮",
        g_cpu_float_enabled ? "悬浮：已开启" : "悬浮：已关闭",
        ACTION_CPU_FLOAT, 0u, 1u);
    add_spec(specs, &count, "当前占用", g_cpu_text,
        ACTION_CPU_REFRESH, 0u, 1u);
    apply_specs(PAGE_CPU, specs, count, 0, 0, 0);
}

static void render_cpu_bench(void) {
    struct row_spec specs[7];
    uint32_t count = 0u;
    add_spec(specs, &count, "开始跑分", g_status,
        ACTION_CPU_BENCH, 0u, 1u);
    add_spec(specs, &count, g_cpu_bench_rows[0], g_empty,
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, g_cpu_bench_rows[1], g_empty,
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, g_cpu_bench_rows[2], g_empty,
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, g_cpu_bench_rows[3], g_empty,
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, g_cpu_bench_rows[4], g_empty,
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, g_cpu_bench_rows[5], g_empty,
        ACTION_NONE, 0u, 0u);
    apply_specs(PAGE_CPU_BENCH, specs, count, 0, 0, 0);
}

static void render_vibration(void) {
    struct row_spec specs[4];
    uint32_t count = 0u;
    add_spec(specs, &count, "短脉冲", "标准震动",
        ACTION_VIBRATION_TEST, 0u, 1u);
    add_spec(specs, &count, "增强脉冲", "较强震感",
        ACTION_VIBRATION_TEST, 1u, 1u);
    add_spec(specs, &count, "停止", "停止当前震动",
        ACTION_VIBRATION_TEST, 2u, 1u);
    add_spec(specs, &count, "状态", g_status,
        ACTION_NONE, 0u, 0u);
    apply_specs(PAGE_VIBRATION, specs, count, 0, 0, 0);
}

static void render_memory(void) {
    struct row_spec specs[3];
    uint32_t count = 0u;
    add_spec(specs, &count, "内存占用检测",
        (g_monitor_state & MONITOR_STATE_MEMORY_ENABLED) ?
            "检测：已开启 · 500ms" : "检测：已关闭",
        ACTION_MEMORY_MONITOR, 0u, 1u);
    add_spec(specs, &count, "内存悬浮",
        (g_monitor_state & MONITOR_STATE_MEMORY_FLOAT) ?
            "悬浮：已开启" : "悬浮：已关闭",
        ACTION_MEMORY_FLOAT, 0u, 1u);
    (void)shellpp_fs_read_memory(g_memory_text,
        sizeof(g_memory_text), 0);
    add_spec(specs, &count, "当前占用", g_memory_text,
        ACTION_MEMORY_REFRESH, 0u, 1u);
    apply_specs(PAGE_MEMORY, specs, count, 0, 0, 0);
}

static void format_app_summary(void) {
    char *cursor = g_memory_text;
    char *end = g_memory_text + sizeof(g_memory_text);
    *cursor = '\0';
    cursor = append_text(cursor, end, "已选 ");
    cursor = append_u32(cursor, end, g_app.selected_count);
    cursor = append_text(cursor, end, " / ");
    (void)append_u32(cursor, end, g_app.total);
}

static void format_app_page_text(void) {
    uint32_t page_count = (g_app.total + APP_PAGE_SIZE - 1u) /
        APP_PAGE_SIZE;
    char *cursor = g_size_text;
    char *end = g_size_text + sizeof(g_size_text);
    *cursor = '\0';
    cursor = append_text(cursor, end, "第 ");
    cursor = append_u32(cursor, end, g_app.page + 1u);
    cursor = append_text(cursor, end, " / ");
    (void)append_u32(cursor, end, page_count ? page_count : 1u);
}

static const char *app_item_state(uint32_t index) {
    if (app_item_locked(index)) return "受保护";
    if (app_item_selected(index))
        return g_app_items[index].hidden ? "已选 · 已隐藏" :
            "已选 · 已显示";
    return g_app_items[index].hidden ? "未选 · 已隐藏" :
        "未选 · 已显示";
}

static void format_app_state(uint32_t index, char *buffer, uint32_t capacity) {
    char *cursor = buffer;
    char *end = buffer + capacity;
    uint32_t size = 0u;
    int result;
    if (!capacity) return;
    cursor = append_text(cursor, end, app_item_state(index));
    cursor = append_text(cursor, end, " · ");
    result = shellpp_fs_app_size(app_item_package(index), &size);
    if (result == SHELLPP_FS_OK)
        format_cache_size(cursor, (uint32_t)(end - cursor), size);
    else
        (void)append_text(cursor, end, "大小未知");
}

static void render_app_list(void) {
    struct row_spec specs[UI_MAX_ROWS];
    uint32_t count = 0u;
    uint32_t index;
    uint32_t start = (uint32_t)g_app.page * APP_PAGE_SIZE;
    uint32_t end = start + APP_PAGE_SIZE;
    uint16_t selectable = app_selectable_count();
    format_app_summary();
    format_app_page_text();
    if (end > g_app.total) end = g_app.total;
    add_spec(specs, &count, "状态", g_status, ACTION_APP_REFRESH, 0u, 1u);
    add_spec(specs, &count,
        g_app.selected_count == selectable && selectable ? "取消全选" :
            "全选",
        g_memory_text, ACTION_APP_SELECT_ALL, 0u,
        g_app.loaded && selectable != 0u);
    add_spec(specs, &count, "隐藏选中", "隐藏已选择的应用",
        ACTION_APP_HIDE, 0u, g_app.selected_count != 0u);
    add_spec(specs, &count, "显示选中", "显示已选择的应用",
        ACTION_APP_SHOW, 0u, g_app.selected_count != 0u);
    add_spec(specs, &count,
        g_app.delete_armed ? "再次确认卸载" : "卸载选中",
        g_app.delete_armed ? "此操作不可撤销" : "需要再次点击确认",
        ACTION_APP_DELETE, 0u,
        g_app.selected_count != 0u);
    add_spec(specs, &count, "隐藏全部", "隐藏所有未受保护的应用",
        ACTION_APP_HIDE_ALL, 0u, g_app.loaded && selectable);
    add_spec(specs, &count, "显示全部", "显示所有已隐藏的应用",
        ACTION_APP_SHOW_ALL, 0u, g_app.loaded && selectable);
    add_spec(specs, &count,
        g_app.reboot_armed ? "再次确认重载" : "重载系统",
        g_app.reboot_armed ? "调用软重启入口，未保存内容会丢失" :
            "应用隐藏或卸载后可在此重载",
        ACTION_APP_REBOOT, 0u, 1u);
    add_spec(specs, &count, "页码", g_size_text, ACTION_NONE, 0u, 0u);
    if (!g_app.loaded) {
        add_spec(specs, &count, "读取失败", "无法读取固件应用注册表",
            ACTION_NONE, 0u, 0u);
    } else if (!g_app.total) {
        add_spec(specs, &count, "暂无应用", "没有可管理的快应用",
            ACTION_NONE, 0u, 0u);
    } else {
        for (index = start; index < end && count < UI_MAX_ROWS; ++index) {
            format_app_state(index, g_entry_secondary[index - start],
                sizeof(g_entry_secondary[index - start]));
            add_spec(specs, &count, app_item_name(index),
                g_entry_secondary[index - start], ACTION_APP_TOGGLE,
                (uint8_t)(index - start), !app_item_locked(index));
            specs[count - 1u].checked = app_item_selected(index);
        }
    }
    add_spec(specs, &count, "上一页", "查看前一页应用",
        ACTION_APP_PREVIOUS, 0u, g_app.page != 0u);
    add_spec(specs, &count, "下一页", "查看后一页应用",
        ACTION_APP_NEXT, 0u,
        (g_app.page + 1u) * APP_PAGE_SIZE < g_app.total);
    apply_specs(PAGE_FILES, specs, count, 0, 0, 0);
}

static void render_file_group(void) {
    struct row_spec specs[3];
    uint32_t count = 0u;
    add_spec(specs, &count, "文件查看", "浏览、编辑、复制、移动与删除文件",
        ACTION_NAVIGATE, PAGE_VIEWER, 1u);
    add_spec(specs, &count, "应用管理", "隐藏、显示或卸载快应用",
        ACTION_APPS_OPEN, 0u, 1u);
    add_spec(specs, &count, "缓存清理",
        "缓存、临时文件、系统日志与离线日志",
        ACTION_NAVIGATE, PAGE_CACHE, 1u);
    apply_specs(PAGE_FILES, specs, count, 0, 0, 0);
}

static void render_browser_list(uint32_t page_index) {
    struct row_spec specs[UI_MAX_ROWS];
    uint32_t count = 0u;
    uint32_t index;
    int parent_result = shellpp_fs_parent(g_current_path, g_path_buffer,
        sizeof(g_path_buffer));
    add_spec(specs, &count, "当前路径", g_current_path,
        ACTION_BROWSER_REFRESH, 0u, 1u);
    add_spec(specs, &count, "状态", g_status, ACTION_NONE, 0u, 0u);
    if (g_clipboard_mode) {
        format_clipboard_secondary();
        add_spec(specs, &count, "粘贴到当前目录", g_clipboard_secondary,
            ACTION_BROWSER_PASTE, 0u, 1u);
    }
    add_spec(specs, &count, "../",
        parent_result == SHELLPP_FS_OK ? g_path_buffer : "/",
        ACTION_BROWSER_PARENT, 0u, !path_is_root(g_current_path));
    if (!g_directory_page.count) {
        add_spec(specs, &count, "目录为空", "没有可显示的文件或目录",
            ACTION_NONE, 0u, 0u);
    } else {
        for (index = 0u; index < g_directory_page.count; ++index) {
            format_entry_secondary(index);
            add_spec(specs, &count, g_directory_page.entries[index].name,
                g_entry_secondary[index], ACTION_BROWSER_ENTRY,
                (uint8_t)index, 1u);
        }
    }
    if (g_after_cursor.valid) {
        add_spec(specs, &count, "上一页", "查看前面的项目",
            ACTION_BROWSER_PREVIOUS, 0u, 1u);
    }
    if (g_directory_page.has_next) {
        add_spec(specs, &count, "下一页", "查看更多项目",
            ACTION_BROWSER_NEXT, 0u, 1u);
    }
    apply_specs(page_index, specs, count, 0, 0, 0);
}

static void render_browser_detail(uint32_t page_index) {
    struct row_spec specs[UI_MAX_ROWS];
    const char *name = shellpp_fs_basename(g_selected_path);
    uint32_t count = 0u;
    uint8_t viewable = !g_selected_is_link && g_selected_size_known &&
        g_selected_size <= SHELLPP_FS_VIEW_LIMIT;
    uint8_t mutable_file = !g_browser_read_only && !g_selected_is_link &&
        g_selected_type == FS_TYPE_REGULAR;
    format_size(g_size_text, sizeof(g_size_text), g_selected_size,
        g_selected_size_known);
    add_spec(specs, &count, "返回目录", "返回当前目录",
        ACTION_BROWSER_BACK, 0u, 1u);
    add_spec(specs, &count, name ? name : "文件", g_size_text,
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "状态", g_status, ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "文本查看",
        viewable ? "最多显示前 4096 B" : "文件过大、未知或不安全",
        ACTION_OPEN_TEXT, 0u, viewable);
    add_spec(specs, &count, "Hex 查看",
        viewable ? "每页读取 2048 B" : "文件过大、未知或不安全",
        ACTION_OPEN_HEX, 0u, viewable);
    {
        add_spec(specs, &count, "复制", "复制到其他目录",
            ACTION_CLIP_COPY, 0u, mutable_file);
        add_spec(specs, &count, "移动", "移动到其他目录",
            ACTION_CLIP_MOVE, 0u, mutable_file);
        add_spec(specs, &count,
            g_delete_armed ? "再次点击确认删除" : "删除",
            g_delete_armed ? "此操作不可撤销" : "需要再次点击确认",
            ACTION_DELETE, 0u,
            mutable_file || g_selected_is_link);
    }
    apply_specs(page_index, specs, count, 0, 0, 0);
}

static void render_text_view(uint32_t page_index) {
    struct row_spec specs[4];
    uint32_t count = 0u;
    uint32_t next_offset;
    uint32_t current_page;
    uint32_t total_pages;
    restore_cut();
    if (g_text_offset > g_workspace_length) g_text_offset = g_workspace_length;
    g_text_offset = utf8_floor(g_text_offset, g_workspace_length);
    next_offset = utf8_floor(g_text_offset + LABEL_SLICE,
        g_workspace_length);
    current_page = (g_hex_file_offset + g_text_offset) / LABEL_SLICE + 1u;
    total_pages = (g_selected_size + LABEL_SLICE - 1u) / LABEL_SLICE;
    format_view_page(current_page, total_pages);
    prepare_cut(g_text_offset, g_workspace_length, LABEL_SLICE);
    add_spec(specs, &count, "上一页", "向前浏览文本",
        ACTION_TEXT_PREVIOUS, 0u, g_text_offset > 0u);
    add_spec(specs, &count, "下一页", "向后浏览文本",
        ACTION_TEXT_NEXT, 0u, next_offset < g_workspace_length);
    add_spec(specs, &count, "页码", g_size_text, ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "返回上一级", "返回当前文件详情",
        ACTION_BROWSER_DETAIL_BACK, 0u, 1u);
    apply_specs(page_index, specs, count, 0, 0, VIEW_ROW_TOP);
    apply_view_label(page_index, (const char *)g_workspace + g_text_offset);
}

static void render_hex_view(uint32_t page_index) {
    struct row_spec specs[4];
    uint32_t count = 0u;
    uint32_t current_page;
    uint32_t total_pages;
    uint8_t has_previous = g_hex_file_offset > 0u || g_hex_line_offset > 0u;
    uint8_t has_next = g_hex_line_offset + HEX_SCREEN_LINES * 8u <
        g_hex_page_length || g_hex_file_offset + g_hex_page_length <
        g_selected_size;
    current_page = (g_hex_file_offset + g_hex_line_offset) /
        (HEX_SCREEN_LINES * 8u) + 1u;
    total_pages = (g_selected_size + HEX_SCREEN_LINES * 8u - 1u) /
        (HEX_SCREEN_LINES * 8u);
    format_view_page(current_page, total_pages);
    add_spec(specs, &count, "上一页", "查看前一段字节",
        ACTION_HEX_PREVIOUS, 0u, has_previous);
    add_spec(specs, &count, "下一页", "查看后一段字节",
        ACTION_HEX_NEXT, 0u, has_next);
    add_spec(specs, &count, "页码", g_size_text, ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "返回上一级", "返回当前文件详情",
        ACTION_BROWSER_DETAIL_BACK, 0u, 1u);
    apply_specs(page_index, specs, count, 0, 0, VIEW_ROW_TOP);
    apply_view_label(page_index, (const char *)g_workspace);
}

static void render_browser(uint32_t page_index) {
    if (g_browser_owner != page_index) {
        start_browser(page_index);
    }
    switch (g_browser_mode) {
        case BROWSER_DETAIL: render_browser_detail(page_index); break;
        case BROWSER_TEXT: render_text_view(page_index); break;
        case BROWSER_HEX: render_hex_view(page_index); break;
        default: render_browser_list(page_index); break;
    }
}

static void refresh_cache_report(void) {
    int result = shellpp_fs_cache_status(g_cache_include_logs,
        &g_cache_report);
    g_cache_clear_armed = 0u;
    set_operation_status("缓存统计", result);
}

static void format_cache_secondary(uint32_t index) {
    const struct shellpp_cache_root_report *root =
        &g_cache_report.roots[index];
    char *cursor = g_cache_secondary[index];
    char *end = cursor + sizeof(g_cache_secondary[index]);
    *cursor = '\0';
    format_cache_size(cursor, (uint32_t)(end - cursor), root->bytes);
    cursor += text_length(cursor, (uint32_t)(end - cursor));
    cursor = append_text(cursor, end, root->exists ? " · 存在 · " :
        " · 不存在 · ");
    (void)append_text(cursor, end, root->path);
}

static void render_cache(void) {
    struct row_spec specs[UI_MAX_ROWS];
    uint32_t count = 0u;
    uint32_t index;
    format_cache_size(g_cache_total_text, sizeof(g_cache_total_text),
        g_cache_report.before_bytes);
    format_cache_size(g_cache_freed_text, sizeof(g_cache_freed_text),
        g_cache_last_freed);
    add_spec(specs, &count, "缓存总量", g_cache_total_text,
        ACTION_CACHE_REFRESH, 0u, 1u);
    add_spec(specs, &count, "包含 Shell++ II 日志",
        g_cache_include_logs ? "已开启 · /data/shellpp-ii/logs" : "已关闭",
        ACTION_CACHE_LOGS, 0u, 1u);
    add_spec(specs, &count,
        g_cache_clear_armed ? "再次点击确认清理" : "清理缓存",
        g_cache_clear_armed ? "仅清理所列目录，根目录会保留" : g_status,
        ACTION_CACHE_CLEAR, 0u, 1u);
    if (g_cache_last_freed) {
        add_spec(specs, &count, "释放空间", g_cache_freed_text,
            ACTION_NONE, 0u, 0u);
    }
    for (index = 0u; index < g_cache_report.root_count; ++index) {
        format_cache_secondary(index);
        add_spec(specs, &count,
            shellpp_fs_basename(g_cache_report.roots[index].path),
            g_cache_secondary[index], ACTION_NONE, 0u, 0u);
    }
    if (!g_cache_report.before_bytes) {
        add_spec(specs, &count, "暂无缓存", "没有可显示的缓存项",
            ACTION_NONE, 0u, 0u);
    }
    apply_specs(PAGE_CACHE, specs, count, 0, 0, 0);
}

static void render_page(uint32_t page_index) {
    if (page_index >= PAGE_COUNT || !g_ui[page_index].active) return;
    if (page_index == PAGE_VIEWER && g_viewer_rebuild_pending) {
        g_viewer_rebuild_pending = 0u;
        rebuild_viewer_content();
    }
    if (page_index == PAGE_HOME) render_home();
    else if (page_index == PAGE_FILES) {
        if (g_app_mode == APP_MODE_LIST) render_app_list();
        else render_file_group();
    }
    else if (page_index == PAGE_VIEWER)
        render_browser(page_index);
    else if (page_index == PAGE_CACHE) render_cache();
    else if (page_index == PAGE_ABOUT) render_about();
    else if (page_index == PAGE_DISPLAY) render_display();
    else if (page_index == PAGE_CPU_BENCH) render_cpu_bench();
    else if (page_index == PAGE_VIBRATION) render_vibration();
    else if (page_index == PAGE_RESTART) render_restart();
    else if (page_index == PAGE_CPU) render_cpu();
    else if (page_index == PAGE_MEMORY) render_memory();
}

static void browser_previous_page(void) {
    int result;
    if (!g_directory_page.count || !g_after_cursor.valid) return;
    result = shellpp_fs_previous_cursor(g_current_path,
        &g_directory_page.first, &g_navigation_cursor);
    if (result != SHELLPP_FS_OK) {
        set_operation_status("上一页", result);
        return;
    }
    (void)load_directory(g_current_path,
        g_navigation_cursor.valid ? &g_navigation_cursor : 0);
}

static void browser_next_page(void) {
    if (!g_directory_page.count || !g_directory_page.has_next) return;
    g_navigation_cursor = g_directory_page.last;
    (void)load_directory(g_current_path, &g_navigation_cursor);
}

static void browser_parent(void) {
    int result;
    if (path_is_root(g_current_path)) return;
    result = shellpp_fs_parent(g_current_path, g_path_buffer,
        sizeof(g_path_buffer));
    if (result == SHELLPP_FS_OK) result = load_directory(g_path_buffer, 0);
    if (result != SHELLPP_FS_OK) set_operation_status("上级目录", result);
}

static void browser_set_clipboard(uint8_t mode) {
    if (g_browser_read_only || g_selected_is_link ||
            g_selected_type != FS_TYPE_REGULAR) {
        set_status("只允许复制或移动普通文件");
        return;
    }
    if (copy_text(g_clipboard_path, sizeof(g_clipboard_path),
            g_selected_path) < 0) {
        set_status("剪贴板路径过长");
        return;
    }
    g_clipboard_mode = mode;
    g_browser_mode = BROWSER_LIST;
    g_delete_armed = 0u;
    set_status(mode == 2u ? "已剪切，进入目标目录后粘贴" :
        "已复制，进入目标目录后粘贴");
}

static void browser_paste(void) {
    const char *name;
    int result;
    uint8_t mode = g_clipboard_mode;
    if (g_browser_read_only || !mode) return;
    name = shellpp_fs_basename(g_clipboard_path);
    result = name ? shellpp_fs_join(g_current_path, name, g_path_buffer,
        sizeof(g_path_buffer)) : SHELLPP_FS_ERR_PATH;
    if (result == SHELLPP_FS_OK) {
        result = mode == 2u ?
            shellpp_fs_move(g_clipboard_path, g_path_buffer, g_workspace,
                sizeof(g_workspace)) :
            shellpp_fs_copy(g_clipboard_path, g_path_buffer, g_workspace,
                sizeof(g_workspace));
    }
    if (result == SHELLPP_FS_OK) {
        g_clipboard_mode = 0u;
        g_clipboard_path[0] = '\0';
        (void)refresh_directory();
    }
    set_operation_status(mode == 2u ? "移动" : "复制", result);
}

static void browser_delete(void) {
    int result;
    if (g_browser_read_only) return;
    if (!g_delete_armed) {
        g_delete_armed = 1u;
        set_status("再次点击删除以确认");
        return;
    }
    g_delete_armed = 0u;
    result = shellpp_fs_delete_file(g_selected_path);
    if (result == SHELLPP_FS_OK) {
        g_browser_mode = BROWSER_LIST;
        (void)refresh_directory();
    }
    set_operation_status("删除", result);
}

static void text_previous(void) {
    uint32_t offset;
    restore_cut();
    if (g_text_offset > LABEL_SLICE) g_text_offset =
        utf8_floor(g_text_offset - LABEL_SLICE, g_workspace_length);
    else if (g_hex_file_offset > 0u) {
        offset = g_hex_file_offset > SHELLPP_FS_TEXT_LIMIT ?
            g_hex_file_offset - SHELLPP_FS_TEXT_LIMIT : 0u;
        if (load_text_page(offset) == SHELLPP_FS_OK &&
                g_workspace_length > LABEL_SLICE)
            g_text_offset = utf8_floor(g_workspace_length - LABEL_SLICE,
                g_workspace_length);
    } else g_text_offset = 0u;
}

static void text_next(void) {
    uint32_t next;
    restore_cut();
    next = utf8_floor(g_text_offset + LABEL_SLICE, g_workspace_length);
    if (next > g_text_offset && next < g_workspace_length)
        g_text_offset = next;
    else if (g_hex_file_offset + g_workspace_length < g_selected_size)
        (void)load_text_page(g_hex_file_offset + g_workspace_length);
}

static void hex_previous(void) {
    if (g_hex_line_offset >= HEX_SCREEN_LINES * 8u) {
        g_hex_line_offset -= HEX_SCREEN_LINES * 8u;
        format_hex_screen();
    } else if (g_hex_file_offset > 0u) {
        uint32_t offset = g_hex_file_offset > SHELLPP_FS_HEX_PAGE_SIZE ?
            g_hex_file_offset - SHELLPP_FS_HEX_PAGE_SIZE : 0u;
        if (load_hex_page(offset) == SHELLPP_FS_OK &&
                g_hex_page_length > HEX_SCREEN_LINES * 8u) {
            g_hex_line_offset = ((g_hex_page_length - 1u) /
                (HEX_SCREEN_LINES * 8u)) * (HEX_SCREEN_LINES * 8u);
            format_hex_screen();
        }
    }
}

static void hex_next(void) {
    if (g_hex_line_offset + HEX_SCREEN_LINES * 8u < g_hex_page_length) {
        g_hex_line_offset += HEX_SCREEN_LINES * 8u;
        format_hex_screen();
    } else if (g_hex_file_offset + g_hex_page_length < g_selected_size) {
        (void)load_hex_page(g_hex_file_offset + g_hex_page_length);
    }
}

static int perform_action(uint32_t page_index, uint8_t action,
        uint8_t argument) {
    if (action == ACTION_FILE_GROUP_OPEN) {
        ACTIVITY_NAVIGATE(((uint32_t)SHELLPP_APP_ID << 16) | PAGE_FILES,
            0u, 0u, 0u);
        return 0;
    }
    if (action == ACTION_DISPLAY_OPEN) {
        ACTIVITY_NAVIGATE(((uint32_t)SHELLPP_APP_ID << 16) | PAGE_DISPLAY,
            0u, 0u, 0u);
        return 0;
    }
    if (action == ACTION_CPU_PAGE_OPEN) {
        g_monitor_state &= (uint8_t)~MONITOR_STATE_MEMORY_PAGE;
        ACTIVITY_NAVIGATE(((uint32_t)SHELLPP_APP_ID << 16) | PAGE_CPU,
            0u, 0u, 0u);
        return 0;
    }
    if (action == ACTION_CPU_BENCH_PAGE_OPEN) {
        ACTIVITY_NAVIGATE(((uint32_t)SHELLPP_APP_ID << 16) | PAGE_CPU_BENCH,
            0u, 0u, 0u);
        return 0;
    }
    if (action == ACTION_VIBRATION_PAGE_OPEN) {
        ACTIVITY_NAVIGATE(((uint32_t)SHELLPP_APP_ID << 16) | PAGE_VIBRATION,
            0u, 0u, 0u);
        return 0;
    }
    if (action == ACTION_MEMORY_PAGE_OPEN) {
        g_monitor_state |= MONITOR_STATE_MEMORY_PAGE;
        ACTIVITY_NAVIGATE(((uint32_t)SHELLPP_APP_ID << 16) | PAGE_MEMORY,
            0u, 0u, 0u);
        return 0;
    }
    if (action == ACTION_NAVIGATE) {
        if (argument < PAGE_COUNT) {
            ACTIVITY_NAVIGATE(((uint32_t)SHELLPP_APP_ID << 16) | argument,
                0u, 0u, 0u);
        }
        return 0;
    }
    if (action == ACTION_APPS_OPEN) {
        if (page_index == PAGE_FILES) app_enter_list_mode();
        return 1;
    }
    if (page_index == PAGE_VIEWER &&
            g_browser_owner != page_index) return 1;
    if (action != ACTION_DELETE) g_delete_armed = 0u;
    if (action != ACTION_CACHE_CLEAR) g_cache_clear_armed = 0u;
    if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST &&
            action != ACTION_APP_REBOOT)
        g_app.reboot_armed = 0u;
    if (page_index == PAGE_RESTART && action != ACTION_RESTART_HARD &&
            action != ACTION_RESTART_SOFT) {
        g_restart_hard_armed = 0u;
        g_restart_soft_armed = 0u;
    }
    switch (action) {
        case ACTION_APP_REFRESH:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                (void)app_reload_direct();
            break;
        case ACTION_APP_SELECT_ALL:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_toggle_all_selection();
            break;
        case ACTION_APP_TOGGLE:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_toggle_selection(argument);
            break;
        case ACTION_APP_HIDE:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_apply_direct(APP_OPERATION_HIDE, 0u);
            break;
        case ACTION_APP_SHOW:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_apply_direct(APP_OPERATION_SHOW, 0u);
            break;
        case ACTION_APP_DELETE:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_delete_selection();
            break;
        case ACTION_APP_HIDE_ALL:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_apply_direct(APP_OPERATION_HIDE, 1u);
            break;
        case ACTION_APP_SHOW_ALL:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_apply_direct(APP_OPERATION_SHOW, 1u);
            break;
        case ACTION_APP_PREVIOUS:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_previous_page();
            break;
        case ACTION_APP_NEXT:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_next_page();
            break;
        case ACTION_APP_REBOOT:
            if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
                app_request_reboot();
            break;
        case ACTION_RESTART_HARD:
            if (page_index == PAGE_RESTART) restart_request(0u);
            break;
        case ACTION_RESTART_SOFT:
            if (page_index == PAGE_RESTART) restart_request(1u);
            break;
        case ACTION_BROWSER_ENTRY: (void)select_entry(argument); break;
        case ACTION_BROWSER_PARENT: browser_parent(); break;
        case ACTION_BROWSER_PREVIOUS: browser_previous_page(); break;
        case ACTION_BROWSER_NEXT: browser_next_page(); break;
        case ACTION_BROWSER_PASTE: browser_paste(); break;
        case ACTION_BROWSER_REFRESH: (void)refresh_directory(); break;
        case ACTION_BROWSER_BACK:
            g_browser_mode = BROWSER_LIST;
            g_delete_armed = 0u;
            set_status("返回目录");
            g_viewer_rebuild_pending = 1u;
            break;
        case ACTION_BROWSER_DETAIL_BACK:
            restore_cut();
            g_browser_mode = BROWSER_DETAIL;
            set_status("返回文件详情");
            break;
        case ACTION_CPU_MONITOR:
            if (set_cpu_monitor(g_cpu_monitor_enabled ? 0u : 1u) < 0)
                set_status("CPU检测启动失败");
            else
                set_status(g_cpu_monitor_enabled ? "CPU检测已开启" :
                    "CPU检测已关闭");
            break;
        case ACTION_CPU_FLOAT:
            if (set_cpu_float(g_cpu_float_enabled ? 0u : 1u) < 0)
                set_status("CPU悬浮创建失败");
            else
                set_status(g_cpu_float_enabled ? "CPU悬浮已开启" :
                    "CPU悬浮已关闭");
            break;
        case ACTION_CPU_REFRESH:
        {
            int result = cpu_sample();
            if (result == SHELLPP_FS_OK)
                set_status("CPU占用已刷新");
            else if (result == SHELLPP_FS_ERR_TRUNCATED)
                set_status("CPU读取内容过长");
            else
                set_status("CPU占用读取失败");
            break;
        }
        case ACTION_CPU_BENCH:
            (void)cpu_bench_run();
            break;
        case ACTION_VIBRATION_TEST:
            if (argument == 2u) {
                set_status(shellpp_vibration_stop() == 0 ?
                    "震动已停止" : "停止震动失败");
            } else {
                set_status(shellpp_vibration_pulse(argument == 1u) == 0 ?
                    (argument == 1u ? "增强脉冲已启动" : "短脉冲已启动") :
                    "震动启动失败");
            }
            break;
        case ACTION_MEMORY_MONITOR:
            if (set_memory_monitor((g_monitor_state &
                    MONITOR_STATE_MEMORY_ENABLED) ? 0u : 1u) < 0)
                set_status("内存检测启动失败");
            else
                set_status((g_monitor_state & MONITOR_STATE_MEMORY_ENABLED) ?
                    "内存检测已开启" :
                    "内存检测已关闭");
            break;
        case ACTION_MEMORY_FLOAT:
            if (set_memory_float((g_monitor_state &
                    MONITOR_STATE_MEMORY_FLOAT) ? 0u : 1u) < 0)
                set_status("内存悬浮创建失败");
            else
                set_status((g_monitor_state & MONITOR_STATE_MEMORY_FLOAT) ?
                    "内存悬浮已开启" :
                    "内存悬浮已关闭");
            break;
        case ACTION_MEMORY_REFRESH:
        {
            int result = memory_sample();
            if (result == SHELLPP_FS_OK)
                set_status("内存占用已刷新");
            else if (result == SHELLPP_FS_ERR_TRUNCATED)
                set_status("内存信息过长");
            else
                set_status("内存占用读取失败");
            break;
        }
        case ACTION_OPEN_TEXT: (void)open_text_view(); break;
        case ACTION_OPEN_HEX: (void)load_hex_page(0u); break;
        case ACTION_CLIP_COPY: browser_set_clipboard(1u); break;
        case ACTION_CLIP_MOVE: browser_set_clipboard(2u); break;
        case ACTION_DELETE: browser_delete(); break;
        case ACTION_TEXT_PREVIOUS: text_previous(); break;
        case ACTION_TEXT_NEXT: text_next(); break;
        case ACTION_HEX_PREVIOUS: hex_previous(); break;
        case ACTION_HEX_NEXT: hex_next(); break;
        case ACTION_CACHE_REFRESH: g_cache_last_freed = 0u;
            refresh_cache_report(); break;
        case ACTION_CACHE_LOGS: g_cache_include_logs = !g_cache_include_logs;
            g_cache_last_freed = 0u; refresh_cache_report(); break;
        case ACTION_CACHE_CLEAR:
            if (!g_cache_clear_armed) {
                g_cache_clear_armed = 1u;
                set_status("再次点击清理缓存以确认");
            } else {
                int result;
                int refresh_result;
                g_cache_clear_armed = 0u;
                result = shellpp_fs_cache_clear(g_cache_include_logs,
                    &g_cache_report);
                g_cache_last_freed = g_cache_report.freed_bytes;
                refresh_result = shellpp_fs_cache_status(g_cache_include_logs,
                    &g_cache_report);
                if (result)
                    set_operation_status("缓存清理", result);
                else if (refresh_result)
                    set_status("缓存清理完成，统计刷新失败");
                else
                    set_status("缓存清理完成");
            }
            break;
        default: break;
    }
    return 1;
}

static int handle_back(uint32_t page_index) {
    if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST) {
        app_leave_list_mode();
        return 1;
    }
    if (page_index == PAGE_VIEWER) {
        if (g_browser_owner != page_index || g_browser_mode == BROWSER_LIST) {
            g_ui[page_index].interactive = 0u;
            ACTIVITY_FINISH(g_ui[page_index].descriptor);
            return 0;
        }
        if (g_browser_mode == BROWSER_DETAIL) {
            g_browser_mode = BROWSER_LIST;
            g_delete_armed = 0u;
            set_status("返回目录");
            g_viewer_rebuild_pending = 1u;
        } else if (g_browser_mode == BROWSER_TEXT ||
                g_browser_mode == BROWSER_HEX) {
            restore_cut();
            g_browser_mode = BROWSER_DETAIL;
            set_status("返回文件详情");
        }
        return 1;
    }
    if (page_index != PAGE_HOME && page_index < PAGE_COUNT) {
        g_ui[page_index].interactive = 0u;
        ACTIVITY_FINISH(g_ui[page_index].descriptor);
    }
    return 0;
}

static void row_event(void *event) {
    uint32_t cookie;
    uint32_t page_index;
    uint32_t slot;
    uint16_t generation;
    struct ui_binding binding;
    int should_render;
    if (!event || LVX_EVENT_GET_CODE(event) != EVENT_CLICKED || g_busy) return;
    cookie = (uint32_t)(uintptr_t)LVX_EVENT_GET_USER_DATA(event);
    generation = (uint16_t)(cookie >> 16);
    page_index = (cookie >> 8) & 0xffu;
    slot = cookie & 0xffu;
    if (page_index >= PAGE_COUNT || slot >= UI_MAX_ROWS) return;
    if (!g_ui[page_index].active || !g_ui[page_index].interactive ||
            g_ui[page_index].generation != generation) return;
    if (g_bindings_page != page_index ||
            g_bindings_generation != generation) return;
    binding = g_bindings[slot];
    if (!binding.enabled || binding.action == ACTION_NONE) return;
    g_busy = 1u;
    should_render = perform_action(page_index, binding.action,
        binding.argument);
    if (should_render && g_ui[page_index].active)
        render_page(page_index);
    g_busy = 0u;
    g_viewer_rebuild_pending = 0u;
}

static void title_back_event(void *event) {
    uint32_t cookie;
    uint32_t page_index;
    uint16_t generation;
    int should_render;
    if (!event || g_busy) return;
    cookie = (uint32_t)(uintptr_t)LVX_EVENT_GET_USER_DATA(event);
    generation = (uint16_t)(cookie >> 16);
    page_index = (cookie >> 8) & 0xffu;
    if (page_index == PAGE_HOME || page_index >= PAGE_COUNT ||
            !g_ui[page_index].active || !g_ui[page_index].interactive ||
            g_ui[page_index].generation != generation) return;
    g_busy = 1u;
    should_render = handle_back(page_index);
    if (should_render && g_ui[page_index].active)
        render_page(page_index);
    g_busy = 0u;
    g_restart_hard_armed = 0u;
    g_restart_soft_armed = 0u;
}

void shellpp_ui_reset(void) {
    /* The overlay belongs to the display top layer, not a page root. It must
     * be stopped and hidden explicitly before the page state is discarded. */
    stop_cpu_overlay();
    stop_memory_overlay();
    if (g_app_mode == APP_MODE_LIST) {
        app_clear_items();
    }
    g_app_mode = APP_MODE_MENU;
    restore_cut();
    clear_bytes(g_ui, sizeof(g_ui));
    clear_bytes(g_bindings, sizeof(g_bindings));
    g_bindings_generation = 0u;
    g_bindings_page = 0xffu;
    clear_bytes(&g_directory_page, sizeof(g_directory_page));
    clear_bytes(&g_after_cursor, sizeof(g_after_cursor));
    clear_bytes(&g_navigation_cursor, sizeof(g_navigation_cursor));
    clear_bytes(&g_cache_report, sizeof(g_cache_report));
    clear_bytes(g_workspace, sizeof(g_workspace));
    clear_bytes(g_current_path, sizeof(g_current_path));
    clear_bytes(g_selected_path, sizeof(g_selected_path));
    clear_bytes(g_clipboard_path, sizeof(g_clipboard_path));
    clear_bytes(g_status, sizeof(g_status));
    g_workspace_length = 0u;
    g_clipboard_mode = 0u;
    g_browser_owner = 0xffu;
    g_browser_mode = BROWSER_LIST;
    g_monitor_state = 0u;
    g_cpu_monitor_enabled = 0u;
    g_cpu_float_enabled = 0u;
    g_cpu_text[0] = 'C';
    g_cpu_text[1] = 'P';
    g_cpu_text[2] = 'U';
    g_cpu_text[3] = ':';
    g_cpu_text[4] = '0';
    g_cpu_text[5] = '%';
    g_cpu_text[6] = '\0';
    copy_text(g_status, sizeof(g_status), "点击开始");
    copy_text(g_cpu_bench_rows[0], sizeof(g_cpu_bench_rows[0]),
        "综合：未运行");
    copy_text(g_cpu_bench_rows[1], sizeof(g_cpu_bench_rows[1]),
        "整数：未运行");
    copy_text(g_cpu_bench_rows[2], sizeof(g_cpu_bench_rows[2]),
        "浮点：未运行");
    copy_text(g_cpu_bench_rows[3], sizeof(g_cpu_bench_rows[3]),
        "内存：未运行");
    copy_text(g_cpu_bench_rows[4], sizeof(g_cpu_bench_rows[4]),
        "混合：未运行");
    copy_text(g_cpu_bench_rows[5], sizeof(g_cpu_bench_rows[5]),
        "耗时：未运行");
    copy_text(g_status, sizeof(g_status),
        "等待选择震动模式");
    g_cache_include_logs = 0u;
    g_cache_last_freed = 0u;
    g_busy = 0u;
}

int shellpp_ui_page_create(uint32_t page_index, void *descriptor, void *root) {
    struct ui_page *ui;
    uint16_t generation;
    uint32_t mode;
    const void *callback;
    void *context;
    if (page_index >= PAGE_COUNT || !descriptor || !root) return -1;
    generation = (uint16_t)(g_ui[page_index].generation + 1u);
    if (!generation) generation = 1u;
    ui = &g_ui[page_index];
    clear_bytes(ui, sizeof(*ui));
    ui->root = root;
    ui->descriptor = descriptor;
    ui->generation = generation;
    ui->active = 1u;
    ui->interactive = 1u;
    ui->content = LVX_CONTENT_CREATE(root);
    if (!ui->content) {
        clear_bytes(ui, sizeof(*ui));
        return -1;
    }
    LVX_OBJECT_SET_SIZE(ui->content, CONTENT_WIDTH, CONTENT_HEIGHT);
    LVX_OBJECT_ALIGN(ui->content, ALIGN_TOP_MID, 0, CONTENT_TOP_OFFSET);
    mode = page_index == PAGE_HOME ? 0u : 1u;
    callback = mode ? (const void *)title_back_event : 0;
    context = (void *)(uintptr_t)event_cookie(generation,
        (uint8_t)page_index, 0xffu);
    ui->title = LVX_PAGE_TITLE_CREATE(root, g_page_titles[page_index], mode,
        callback, context);
    if (!ui->title) {
        clear_bytes(ui, sizeof(*ui));
        return -1;
    }
    apply_misans(ui->title);
    if (page_index == PAGE_VIEWER)
        start_browser(page_index);
    else if (page_index == PAGE_CACHE) {
        g_cache_last_freed = 0u;
        refresh_cache_report();
    }
    else if (page_index == PAGE_MEMORY &&
            (g_monitor_state & MONITOR_STATE_MEMORY_PAGE))
        (void)memory_sample();
    render_page(page_index);
    return 0;
}

int shellpp_ui_page_resume(uint32_t page_index, void *descriptor) {
    if (page_index >= PAGE_COUNT || !descriptor ||
            !g_ui[page_index].active ||
            g_ui[page_index].descriptor != descriptor) return -1;
    g_ui[page_index].interactive = 1u;
    if (page_index == PAGE_CACHE) {
        g_cache_last_freed = 0u;
        refresh_cache_report();
    }
    else if (page_index == PAGE_MEMORY &&
            (g_monitor_state & MONITOR_STATE_MEMORY_PAGE))
        (void)memory_sample();
    render_page(page_index);
    return 0;
}

int shellpp_ui_page_pause(uint32_t page_index) {
    if (page_index >= PAGE_COUNT || !g_ui[page_index].active) return -1;
    g_ui[page_index].interactive = 0u;
    if (g_bindings_page == page_index) g_bindings_page = 0xffu;
    return 0;
}

int shellpp_ui_page_destroy(uint32_t page_index) {
    uint16_t generation;
    if (page_index >= PAGE_COUNT) return -1;
    if (page_index == PAGE_FILES && g_app_mode == APP_MODE_LIST)
        app_leave_list_mode();
    if (g_browser_owner == page_index) restore_cut();
    if (g_bindings_page == page_index) g_bindings_page = 0xffu;
    generation = g_ui[page_index].generation;
    clear_bytes(&g_ui[page_index], sizeof(g_ui[page_index]));
    g_ui[page_index].generation = generation;
    return 0;
}
