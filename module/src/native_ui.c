#include "shellpp_native_ui.h"
#include "shellpp_native_fs.h"

typedef void *(*lvx_content_create_t)(void *root);
typedef void *(*lvx_page_title_create_t)(void *root, const char *title,
    uint32_t mode, const void *back_callback, void *context);
typedef void *(*lvx_label_create_t)(void *parent);
typedef void (*lvx_label_set_text_t)(void *label, const char *text);
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

#define LVX_CONTENT_CREATE ((lvx_content_create_t)0x0ca4e991u)
#define LVX_PAGE_TITLE_CREATE ((lvx_page_title_create_t)0x0c4a99adu)
#define LVX_LABEL_CREATE ((lvx_label_create_t)0x0c589061u)
#define LVX_LABEL_SET_TEXT ((lvx_label_set_text_t)0x0c587f51u)
#define LVX_OBJECT_SET_SIZE ((lvx_object_set_size_t)0x0c588e79u)
#define LVX_OBJECT_ALIGN ((lvx_object_align_t)0x0c587c11u)
#define LVX_ALIGN_TO ((lvx_align_to_t)0x0c588501u)
#define LVX_SET_HIDDEN ((lvx_set_hidden_t)0x0c5879b9u)
#define LVX_STYLE_APPLY ((lvx_style_apply_t)0x0c49eb81u)
#define LVX_LIST_ROW_CREATE ((lvx_list_row_create_t)0x0c52b78du)
#define LVX_LIST_ROW_UPDATE ((lvx_list_row_update_t)0x0c4a7bedu)
#define LVX_LIST_ROW_TRAILING ((lvx_list_row_trailing_t)0x0c4a7f49u)
#define LVX_EVENT_ADD ((lvx_event_add_t)0x0c5881a9u)
#define LVX_EVENT_GET_USER_DATA ((lvx_event_get_user_data_t)0x0c588239u)
#define LVX_EVENT_GET_CODE ((lvx_event_get_code_t)0x0c588f59u)
#define ACTIVITY_NAVIGATE ((activity_navigate_t)0x0ca53aa1u)
#define ACTIVITY_FINISH ((activity_finish_t)0x0ca53131u)
#define STYLE_MISANS_DEMIBOLD_32 ((const void *)0x2010a02cu)

#define SHELLPP_APP_ID 0x00cdu
#define PAGE_COUNT 5u
#define PAGE_HOME 0u
#define PAGE_FILES 1u
#define PAGE_VIEWER 2u
#define PAGE_CACHE 3u
#define PAGE_ABOUT 4u
#define UI_MAX_ROWS 32u
#define CONTENT_WIDTH 336
#define CONTENT_HEIGHT 424
#define CONTENT_TOP_OFFSET 56
#define ALIGN_TOP_MID 2u
#define ALIGN_OUT_BOTTOM_MID 13u
#define EVENT_CLICKED 7u
#define TRAILING_NONE 0u
#define ROW_GAP 4
#define LABEL_SLICE 384u
#define HEX_RAW_OFFSET 8192u
#define HEX_SCREEN_LINES 10u
#define FS_TYPE_REGULAR 8u

enum browser_mode {
    BROWSER_LIST = 0,
    BROWSER_DETAIL = 1,
    BROWSER_TEXT = 2,
    BROWSER_HEX = 3,
    BROWSER_EDITOR = 4,
    BROWSER_KEYBOARD = 5,
    BROWSER_CURSOR = 6,
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
    ACTION_OPEN_EDITOR = 9,
    ACTION_CLIP_COPY = 10,
    ACTION_CLIP_MOVE = 11,
    ACTION_DELETE = 12,
    ACTION_TEXT_PREVIOUS = 13,
    ACTION_TEXT_NEXT = 14,
    ACTION_HEX_PREVIOUS = 15,
    ACTION_HEX_NEXT = 16,
    ACTION_EDITOR_KEYBOARD = 17,
    ACTION_EDITOR_CURSOR = 18,
    ACTION_EDITOR_NEWLINE = 19,
    ACTION_EDITOR_BACKSPACE = 20,
    ACTION_EDITOR_SAVE = 21,
    ACTION_EDITOR_RELOAD = 22,
    ACTION_KEY_PREVIOUS = 23,
    ACTION_KEY_INSERT = 24,
    ACTION_KEY_NEXT = 25,
    ACTION_KEY_SPACE = 26,
    ACTION_KEY_DONE = 27,
    ACTION_CURSOR_HOME = 28,
    ACTION_CURSOR_LEFT = 29,
    ACTION_CURSOR_RIGHT = 30,
    ACTION_CURSOR_END = 31,
    ACTION_CURSOR_DONE = 32,
    ACTION_CACHE_REFRESH = 33,
    ACTION_CACHE_LOGS = 34,
    ACTION_CACHE_CLEAR = 35,
    ACTION_BROWSER_REFRESH = 36,
    ACTION_BROWSER_BACK = 37,
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
    struct ui_binding bindings[UI_MAX_ROWS];
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

static struct ui_page g_ui[PAGE_COUNT];
static struct shellpp_fs_page g_directory_page;
static struct shellpp_fs_cursor g_after_cursor;
static struct shellpp_fs_cursor g_navigation_cursor;
static struct shellpp_cache_report g_cache_report;
static uint8_t g_workspace[SHELLPP_FS_EDIT_LIMIT + 1u];
static char g_current_path[SHELLPP_FS_PATH_CAP];
static char g_selected_path[SHELLPP_FS_PATH_CAP];
static char g_clipboard_path[SHELLPP_FS_PATH_CAP];
static char g_path_buffer[SHELLPP_FS_PATH_CAP];
static char g_status[160];
static char g_size_text[40];
static char g_entry_secondary[SHELLPP_FS_DIR_PAGE_ENTRIES][32];
static char g_cache_secondary[SHELLPP_FS_CACHE_ROOTS][64];
static char g_clipboard_secondary[144];
static char g_cache_total_text[48];
static char g_cache_freed_text[48];
static char g_editor_secondary[64];
static char g_key_text[8];
static char g_cursor_text[56];

static uint32_t g_workspace_length;
static uint32_t g_selected_size;
static uint32_t g_text_offset;
static uint32_t g_hex_file_offset;
static uint32_t g_hex_page_length;
static uint32_t g_hex_line_offset;
static uint32_t g_editor_length;
static uint32_t g_editor_cursor;
static uint32_t g_editor_original_length;
static uint32_t g_editor_original_hash;
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
static uint8_t g_editor_dirty;
static uint8_t g_reload_armed;
static uint8_t g_discard_armed;
static uint8_t g_keyboard_index;
static uint8_t g_cache_include_logs;
static uint8_t g_cache_clear_armed;
static uint8_t g_busy;
static uint8_t g_viewer_rebuild_pending;
static uint32_t g_cache_last_freed;

static const char g_empty[] = "";
static const char g_page_titles[PAGE_COUNT][32] = {
    "Shell++ II",
    "文件与应用管理",
    "文件查看",
    "缓存清理",
    "关于 Shell++ II",
};
static const char g_keyboard_chars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    "_-./:=,+*()[]{}#@!?'\"\\;<>|&%$";

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

static uint32_t hash_bytes(const uint8_t *bytes, uint32_t length) {
    uint32_t hash = 0x811c9dc5u;
    uint32_t index;
    for (index = 0; index < length; ++index) {
        hash ^= bytes[index];
        hash *= 0x01000193u;
    }
    return hash;
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
    clear_bytes(ui->bindings, sizeof(ui->bindings));
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

    /* Hide stale rows before painting the new directory. Do not update them
     * with empty text: this firmware still lays out an empty list-row as a
     * visible card, which creates placeholder cards below the real entries. */
    for (index = 0; index < UI_MAX_ROWS; ++index) {
        if (ui->rows[index]) {
            set_row_hidden(ui->rows[index], 1u);
        }
        clear_bytes(&ui->bindings[index], sizeof(ui->bindings[index]));
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
        ui->bindings[index].action = specs[index].action;
        ui->bindings[index].argument = specs[index].argument;
        ui->bindings[index].enabled = specs[index].enabled;
        set_row_enabled(ui->rows[index], specs[index].enabled);
    }
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

static void update_cursor_text(void) {
    char *cursor = g_cursor_text;
    char *end = g_cursor_text + sizeof(g_cursor_text);
    *cursor = '\0';
    cursor = append_text(cursor, end, "光标 " );
    cursor = append_u32(cursor, end, g_editor_cursor);
    cursor = append_text(cursor, end, " / " );
    (void)append_u32(cursor, end, g_editor_length);
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
    g_reload_armed = 0u;
    g_discard_armed = 0u;
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
    g_reload_armed = 0u;
    g_discard_armed = 0u;
    set_status("文件已选择");
    g_viewer_rebuild_pending = 1u;
    return SHELLPP_FS_OK;
}

static int open_text_view(void) {
    uint32_t index;
    int result;
    restore_cut();
    if (g_selected_is_link) { set_status("符号链接不跟随"); return SHELLPP_FS_ERR_UNSAFE_TYPE; }
    if (!g_selected_size_known || g_selected_size > SHELLPP_FS_VIEW_LIMIT) {
        set_status("文件过大或大小未知");
        return SHELLPP_FS_ERR_TOO_LARGE;
    }
    result = shellpp_fs_read_at(g_selected_path, 0u, g_workspace,
        SHELLPP_FS_TEXT_LIMIT, &g_workspace_length);
    if (result != SHELLPP_FS_OK) { set_operation_status("文本读取", result); return result; }
    for (index = 0; index < g_workspace_length; ++index) {
        uint8_t value = g_workspace[index];
        if (value < 32u && value != 9u && value != 10u && value != 13u)
            g_workspace[index] = (uint8_t)'.';
    }
    g_workspace[g_workspace_length] = 0u;
    g_text_offset = 0u;
    g_browser_mode = BROWSER_TEXT;
    set_status(g_selected_size > SHELLPP_FS_TEXT_LIMIT ?
        "仅显示前 4096 B" : "文本读取完成");
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

static void update_editor_dirty(void) {
    restore_cut();
    g_editor_dirty = g_editor_length != g_editor_original_length ||
        hash_bytes(g_workspace, g_editor_length) != g_editor_original_hash;
    update_cursor_text();
}

static int load_editor(void) {
    uint32_t read_count = 0u;
    int result;
    restore_cut();
    if (g_browser_read_only || g_selected_is_link ||
            !shellpp_fs_is_editable(g_selected_path)) {
        set_status("此文件不可编辑");
        return SHELLPP_FS_ERR_NOT_EDITABLE;
    }
    if (!g_selected_size_known || g_selected_size > SHELLPP_FS_EDIT_LIMIT) {
        set_status("超过 16384 B，禁止编辑");
        return SHELLPP_FS_ERR_TOO_LARGE;
    }
    result = shellpp_fs_read_at(g_selected_path, 0u, g_workspace,
        g_selected_size, &read_count);
    if (result != SHELLPP_FS_OK || read_count != g_selected_size) {
        set_operation_status("编辑读取", result == SHELLPP_FS_OK ?
            SHELLPP_FS_ERR_TRUNCATED : result);
        return result == SHELLPP_FS_OK ? SHELLPP_FS_ERR_TRUNCATED : result;
    }
    g_editor_length = read_count;
    g_workspace[g_editor_length] = 0u;
    g_editor_cursor = g_editor_length;
    g_editor_original_length = g_editor_length;
    g_editor_original_hash = hash_bytes(g_workspace, g_editor_length);
    g_editor_dirty = 0u;
    g_reload_armed = 0u;
    g_discard_armed = 0u;
    g_browser_mode = BROWSER_EDITOR;
    update_cursor_text();
    set_status("编辑器已加载");
    return SHELLPP_FS_OK;
}

static uint32_t previous_utf8(uint32_t position) {
    if (position == 0u) return 0u;
    --position;
    while (position > 0u && (g_workspace[position] & 0xc0u) == 0x80u) --position;
    return position;
}

static uint32_t next_utf8(uint32_t position) {
    if (position >= g_editor_length) return g_editor_length;
    ++position;
    while (position < g_editor_length &&
            (g_workspace[position] & 0xc0u) == 0x80u) ++position;
    return position;
}

static void editor_insert(uint8_t value) {
    uint32_t index;
    restore_cut();
    if (g_editor_length >= SHELLPP_FS_EDIT_LIMIT) {
        set_status("编辑缓冲区已满");
        return;
    }
    for (index = g_editor_length; index > g_editor_cursor; --index)
        g_workspace[index] = g_workspace[index - 1u];
    g_workspace[g_editor_cursor++] = value;
    ++g_editor_length;
    g_workspace[g_editor_length] = 0u;
    update_editor_dirty();
    set_status("内容已修改");
}

static void editor_backspace(void) {
    uint32_t previous;
    uint32_t index;
    restore_cut();
    if (!g_editor_cursor) return;
    previous = previous_utf8(g_editor_cursor);
    for (index = g_editor_cursor; index <= g_editor_length; ++index)
        g_workspace[previous + index - g_editor_cursor] = g_workspace[index];
    g_editor_length -= g_editor_cursor - previous;
    g_editor_cursor = previous;
    update_editor_dirty();
    set_status("已退格");
}

static int editor_save(void) {
    int result;
    restore_cut();
    result = shellpp_fs_save_atomic(g_selected_path, g_workspace, g_editor_length);
    if (result == SHELLPP_FS_OK) {
        g_selected_size = g_editor_length;
        g_selected_size_known = 1u;
        g_editor_original_length = g_editor_length;
        g_editor_original_hash = hash_bytes(g_workspace, g_editor_length);
        g_editor_dirty = 0u;
        g_reload_armed = 0u;
    }
    set_operation_status("保存", result);
    return result;
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
    struct row_spec specs[2];
    uint32_t count = 0u;
    add_spec(specs, &count, "文件与应用管理", "文件查看与缓存清理",
        ACTION_NAVIGATE, PAGE_FILES, 1u);
    add_spec(specs, &count, "关于", "关于 Shell++ II",
        ACTION_NAVIGATE, PAGE_ABOUT, 1u);
    apply_specs(PAGE_HOME, specs, count, 0, 0, 0);
}

static void render_file_group(void) {
    struct row_spec specs[2];
    uint32_t count = 0u;
    add_spec(specs, &count, "文件查看", "浏览、编辑、复制、移动与删除文件",
        ACTION_NAVIGATE, PAGE_VIEWER, 1u);
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
        uint8_t editable = mutable_file &&
            g_selected_size <= SHELLPP_FS_EDIT_LIMIT &&
            shellpp_fs_is_editable(g_selected_path);
        add_spec(specs, &count, "文本编辑器",
            editable ? "原子保存，最大 16384 B" : "此文件不可编辑",
            ACTION_OPEN_EDITOR, 0u, editable);
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
    struct row_spec specs[3];
    uint32_t count = 0u;
    uint32_t next_offset;
    restore_cut();
    if (g_text_offset > g_workspace_length) g_text_offset = g_workspace_length;
    g_text_offset = utf8_floor(g_text_offset, g_workspace_length);
    next_offset = utf8_floor(g_text_offset + LABEL_SLICE,
        g_workspace_length);
    prepare_cut(g_text_offset, g_workspace_length, LABEL_SLICE);
    add_spec(specs, &count, "上一段", "向前浏览文本",
        ACTION_TEXT_PREVIOUS, 0u, g_text_offset > 0u);
    add_spec(specs, &count, "下一段", "向后浏览文本",
        ACTION_TEXT_NEXT, 0u, next_offset < g_workspace_length);
    add_spec(specs, &count, "状态", g_status, ACTION_NONE, 0u, 0u);
    apply_specs(page_index, specs, count,
        (const char *)g_workspace + g_text_offset, 214, 220);
}

static void render_hex_view(uint32_t page_index) {
    struct row_spec specs[3];
    uint32_t count = 0u;
    uint8_t has_previous = g_hex_file_offset > 0u || g_hex_line_offset > 0u;
    uint8_t has_next = g_hex_line_offset + HEX_SCREEN_LINES * 8u <
        g_hex_page_length || g_hex_file_offset + g_hex_page_length <
        g_selected_size;
    add_spec(specs, &count, "上一页", "查看前一段字节",
        ACTION_HEX_PREVIOUS, 0u, has_previous);
    add_spec(specs, &count, "下一页", "查看后一段字节",
        ACTION_HEX_NEXT, 0u, has_next);
    add_spec(specs, &count, "状态", g_status, ACTION_NONE, 0u, 0u);
    apply_specs(page_index, specs, count, (const char *)g_workspace,
        254, 260);
}

static uint32_t editor_window_start(void) {
    uint32_t start = 0u;
    if (g_editor_cursor > LABEL_SLICE / 2u)
        start = g_editor_cursor - LABEL_SLICE / 2u;
    return utf8_floor(start, g_editor_length);
}

static void format_editor_secondary(void) {
    char *cursor = g_editor_secondary;
    char *end = cursor + sizeof(g_editor_secondary);
    *cursor = '\0';
    cursor = append_text(cursor, end, g_cursor_text);
    if (g_editor_dirty) (void)append_text(cursor, end, " · 未保存");
}

static void render_editor(uint32_t page_index) {
    struct row_spec specs[7];
    uint32_t count = 0u;
    uint32_t start;
    restore_cut();
    update_cursor_text();
    format_editor_secondary();
    start = editor_window_start();
    prepare_cut(start, g_editor_length, LABEL_SLICE);
    add_spec(specs, &count, "键盘", "选择并插入 ASCII 字符",
        ACTION_EDITOR_KEYBOARD, 0u, 1u);
    add_spec(specs, &count, "光标", g_editor_secondary,
        ACTION_EDITOR_CURSOR, 0u, 1u);
    add_spec(specs, &count, "换行", "在光标处插入换行",
        ACTION_EDITOR_NEWLINE, 0u, 1u);
    add_spec(specs, &count, "退格", "删除光标前一个字符",
        ACTION_EDITOR_BACKSPACE, 0u, g_editor_cursor > 0u);
    add_spec(specs, &count, g_editor_dirty ? "保存 *" : "保存",
        g_status, ACTION_EDITOR_SAVE, 0u, g_editor_dirty);
    add_spec(specs, &count,
        g_reload_armed ? "再次点击确认重载" : "重载",
        g_reload_armed ? "放弃未保存修改" : "重新从文件读取",
        ACTION_EDITOR_RELOAD, 0u, 1u);
    apply_specs(page_index, specs, count,
        (const char *)g_workspace + start, 144, 150);
}

static void render_keyboard(uint32_t page_index) {
    struct row_spec specs[5];
    uint32_t count = 0u;
    uint32_t key_count = (uint32_t)sizeof(g_keyboard_chars) - 1u;
    if (g_keyboard_index >= key_count) g_keyboard_index = 0u;
    g_key_text[0] = g_keyboard_chars[g_keyboard_index];
    g_key_text[1] = '\0';
    add_spec(specs, &count, "插入当前字符", g_key_text,
        ACTION_KEY_INSERT, 0u, 1u);
    add_spec(specs, &count, "上一个字符", "循环选择字符",
        ACTION_KEY_PREVIOUS, 0u, 1u);
    add_spec(specs, &count, "下一个字符", "循环选择字符",
        ACTION_KEY_NEXT, 0u, 1u);
    add_spec(specs, &count, "插入空格", "在光标处插入空格",
        ACTION_KEY_SPACE, 0u, 1u);
    add_spec(specs, &count, "完成", "返回编辑器",
        ACTION_KEY_DONE, 0u, 1u);
    apply_specs(page_index, specs, count, 0, 0, 0);
}

static void render_cursor(uint32_t page_index) {
    struct row_spec specs[5];
    uint32_t count = 0u;
    update_cursor_text();
    add_spec(specs, &count, "移到开头", g_cursor_text,
        ACTION_CURSOR_HOME, 0u, g_editor_cursor > 0u);
    add_spec(specs, &count, "左移", "向前移动一个 UTF-8 字符",
        ACTION_CURSOR_LEFT, 0u, g_editor_cursor > 0u);
    add_spec(specs, &count, "右移", "向后移动一个 UTF-8 字符",
        ACTION_CURSOR_RIGHT, 0u, g_editor_cursor < g_editor_length);
    add_spec(specs, &count, "移到末尾", g_cursor_text,
        ACTION_CURSOR_END, 0u, g_editor_cursor < g_editor_length);
    add_spec(specs, &count, "完成", "返回编辑器",
        ACTION_CURSOR_DONE, 0u, 1u);
    apply_specs(page_index, specs, count, 0, 0, 0);
}

static void render_browser(uint32_t page_index) {
    if (g_browser_owner != page_index) {
        start_browser(page_index);
    }
    switch (g_browser_mode) {
        case BROWSER_DETAIL: render_browser_detail(page_index); break;
        case BROWSER_TEXT: render_text_view(page_index); break;
        case BROWSER_HEX: render_hex_view(page_index); break;
        case BROWSER_EDITOR: render_editor(page_index); break;
        case BROWSER_KEYBOARD: render_keyboard(page_index); break;
        case BROWSER_CURSOR: render_cursor(page_index); break;
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

static void render_about(void) {
    struct row_spec specs[UI_MAX_ROWS];
    uint32_t count = 0u;
    add_spec(specs, &count, "Shell++ II", "Beta1",
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "com.shellpp.ii", "包名",
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "系统固件", "3.101.036",
        ACTION_NONE, 0u, 0u);
    add_spec(specs, &count, "开发人员", "@IKUN_CXKPRO",
        ACTION_NONE, 0u, 0u);
    apply_specs(PAGE_ABOUT, specs, count, 0, 0, 0);
}

static void render_page(uint32_t page_index) {
    if (page_index >= PAGE_COUNT || !g_ui[page_index].active) return;
    if (page_index == PAGE_VIEWER && g_viewer_rebuild_pending) {
        g_viewer_rebuild_pending = 0u;
        rebuild_viewer_content();
    }
    if (page_index == PAGE_HOME) render_home();
    else if (page_index == PAGE_FILES) render_file_group();
    else if (page_index == PAGE_VIEWER)
        render_browser(page_index);
    else if (page_index == PAGE_CACHE) render_cache();
    else if (page_index == PAGE_ABOUT) render_about();
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
    restore_cut();
    if (g_text_offset > LABEL_SLICE) g_text_offset =
        utf8_floor(g_text_offset - LABEL_SLICE, g_workspace_length);
    else g_text_offset = 0u;
}

static void text_next(void) {
    uint32_t next;
    restore_cut();
    next = utf8_floor(g_text_offset + LABEL_SLICE, g_workspace_length);
    if (next > g_text_offset && next < g_workspace_length)
        g_text_offset = next;
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

static void editor_reload(void) {
    if (g_editor_dirty && !g_reload_armed) {
        g_reload_armed = 1u;
        set_status("再次点击重载以放弃修改");
        return;
    }
    g_reload_armed = 0u;
    (void)load_editor();
}

static int perform_action(uint32_t page_index, uint8_t action,
        uint8_t argument) {
    uint32_t key_count = (uint32_t)sizeof(g_keyboard_chars) - 1u;
    if (action == ACTION_NAVIGATE) {
        if (argument < PAGE_COUNT) {
            ACTIVITY_NAVIGATE(((uint32_t)SHELLPP_APP_ID << 16) | argument,
                0u, 0u, 0u);
        }
        return 0;
    }
    if (page_index == PAGE_VIEWER &&
            g_browser_owner != page_index) return 1;
    if (action != ACTION_DELETE) g_delete_armed = 0u;
    if (action != ACTION_EDITOR_RELOAD) g_reload_armed = 0u;
    if (action != ACTION_CACHE_CLEAR) g_cache_clear_armed = 0u;
    if (g_browser_mode == BROWSER_EDITOR &&
            action != ACTION_EDITOR_RELOAD) g_discard_armed = 0u;
    switch (action) {
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
        case ACTION_OPEN_TEXT: (void)open_text_view(); break;
        case ACTION_OPEN_HEX: (void)load_hex_page(0u); break;
        case ACTION_OPEN_EDITOR: (void)load_editor(); break;
        case ACTION_CLIP_COPY: browser_set_clipboard(1u); break;
        case ACTION_CLIP_MOVE: browser_set_clipboard(2u); break;
        case ACTION_DELETE: browser_delete(); break;
        case ACTION_TEXT_PREVIOUS: text_previous(); break;
        case ACTION_TEXT_NEXT: text_next(); break;
        case ACTION_HEX_PREVIOUS: hex_previous(); break;
        case ACTION_HEX_NEXT: hex_next(); break;
        case ACTION_EDITOR_KEYBOARD:
            restore_cut(); g_browser_mode = BROWSER_KEYBOARD; break;
        case ACTION_EDITOR_CURSOR:
            restore_cut(); g_browser_mode = BROWSER_CURSOR; break;
        case ACTION_EDITOR_NEWLINE: editor_insert((uint8_t)'\n'); break;
        case ACTION_EDITOR_BACKSPACE: editor_backspace(); break;
        case ACTION_EDITOR_SAVE: (void)editor_save(); break;
        case ACTION_EDITOR_RELOAD: editor_reload(); break;
        case ACTION_KEY_PREVIOUS:
            g_keyboard_index = g_keyboard_index ?
                (uint8_t)(g_keyboard_index - 1u) : (uint8_t)(key_count - 1u);
            break;
        case ACTION_KEY_INSERT:
            editor_insert((uint8_t)g_keyboard_chars[g_keyboard_index]); break;
        case ACTION_KEY_NEXT:
            g_keyboard_index = (uint8_t)((g_keyboard_index + 1u) % key_count);
            break;
        case ACTION_KEY_SPACE: editor_insert((uint8_t)' '); break;
        case ACTION_KEY_DONE: restore_cut(); g_browser_mode = BROWSER_EDITOR; break;
        case ACTION_CURSOR_HOME: restore_cut(); g_editor_cursor = 0u;
            update_cursor_text(); break;
        case ACTION_CURSOR_LEFT: restore_cut();
            g_editor_cursor = previous_utf8(g_editor_cursor);
            update_cursor_text(); break;
        case ACTION_CURSOR_RIGHT: restore_cut();
            g_editor_cursor = next_utf8(g_editor_cursor);
            update_cursor_text(); break;
        case ACTION_CURSOR_END: restore_cut();
            g_editor_cursor = g_editor_length; update_cursor_text(); break;
        case ACTION_CURSOR_DONE: restore_cut(); g_browser_mode = BROWSER_EDITOR; break;
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
        } else if (g_browser_mode == BROWSER_KEYBOARD ||
                g_browser_mode == BROWSER_CURSOR) {
            restore_cut();
            g_browser_mode = BROWSER_EDITOR;
        } else if (g_browser_mode == BROWSER_EDITOR) {
            restore_cut();
            if (g_editor_dirty && !g_discard_armed) {
                g_discard_armed = 1u;
                set_status("再次点击返回以放弃未保存修改");
                return 1;
            }
            g_discard_armed = 0u;
            g_editor_dirty = 0u;
            g_browser_mode = BROWSER_DETAIL;
            set_status("已退出编辑器");
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
    binding = g_ui[page_index].bindings[slot];
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
}

void shellpp_ui_reset(void) {
    restore_cut();
    clear_bytes(g_ui, sizeof(g_ui));
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
    render_page(page_index);
    return 0;
}

int shellpp_ui_page_pause(uint32_t page_index) {
    if (page_index >= PAGE_COUNT || !g_ui[page_index].active) return -1;
    g_ui[page_index].interactive = 0u;
    return 0;
}

int shellpp_ui_page_destroy(uint32_t page_index) {
    uint16_t generation;
    if (page_index >= PAGE_COUNT) return -1;
    if (g_browser_owner == page_index) restore_cut();
    generation = g_ui[page_index].generation;
    clear_bytes(&g_ui[page_index], sizeof(g_ui[page_index]));
    g_ui[page_index].generation = generation;
    return 0;
}
