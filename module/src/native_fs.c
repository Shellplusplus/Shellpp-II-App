#include "shellpp_native_fs.h"

/* Xiaomi Band 10 Pro 3.101.036 uses the older NuttX flag layout. */
#define VELA_O_RDONLY 0x01
#define VELA_O_WRONLY 0x02
#define VELA_O_CREAT 0x04
#define VELA_O_TRUNC 0x20
#define VELA_SEEK_SET 0
#define VELA_SEEK_END 2
#define VELA_DT_DIR 4u
#define VELA_DT_REG 8u
#define VELA_DT_LNK 10u
#define WALK_DEPTH_LIMIT 12u

typedef int32_t (*vela_open_t)(const char *, int32_t, ...);
typedef int32_t (*vela_read_t)(int32_t, void *, uint32_t);
typedef int32_t (*vela_write_t)(int32_t, const void *, uint32_t);
typedef int32_t (*vela_close_t)(int32_t);
typedef int64_t (*vela_lseek_t)(int32_t, int64_t, int32_t);
typedef int32_t (*vela_unlink_t)(const char *);
typedef int32_t (*vela_rename_t)(const char *, const char *);
typedef void *(*vela_opendir_t)(const char *);
typedef int32_t (*vela_closedir_t)(void *);
typedef uint8_t *(*vela_readdir_t)(void *);
typedef int32_t (*vela_rmdir_t)(const char *);

#define VELA_OPEN ((vela_open_t)0x0c1c15b1u)
#define VELA_READ ((vela_read_t)0x0c1c1e25u)
#define VELA_WRITE ((vela_write_t)0x0c1c31c9u)
#define VELA_CLOSE ((vela_close_t)0x0c1aab71u)
#define VELA_LSEEK ((vela_lseek_t)0x0c1c10adu)
#define VELA_UNLINK ((vela_unlink_t)0x0c1c2eddu)
#define VELA_RENAME ((vela_rename_t)0x0c1c1e71u)
#define VELA_OPENDIR ((vela_opendir_t)0x0c1d50b1u)
#define VELA_CLOSEDIR ((vela_closedir_t)0x0c1d50edu)
#define VELA_READDIR ((vela_readdir_t)0x0c1d5119u)
#define VELA_RMDIR ((vela_rmdir_t)0x0c1c21d1u)

static struct shellpp_fs_entry g_candidates[SHELLPP_FS_DIR_PAGE_ENTRIES + 1u];
static char g_work_path[SHELLPP_FS_PATH_CAP];

static const char g_cache_path[] = "/data/shellpp-ii/cache";
static const char g_tmp_path[] = "/data/shellpp-ii/tmp";
static const char g_system_log_path[] = "/data/log";
static const char g_offline_log_path[] = "/data/offlinelog";
static const char g_shellpp_logs_path[] = "/data/shellpp-ii/logs";
static const char g_icon_path[] = "/data/shellpp-ii/shellpp_ii_icon.bin";

static uint32_t text_length(const char *text, uint32_t limit) {
    uint32_t length = 0;
    if (!text) return limit;
    while (length < limit && text[length]) ++length;
    return length;
}

static void clear_bytes(void *address, uint32_t length) {
    uint8_t *bytes = (uint8_t *)address;
    uint32_t index;
    for (index = 0; index < length; ++index) bytes[index] = 0;
}

/* Clang lowers bounded structure copies to this ARM EABI helper.  Keep it
 * inside the module because the firmware loader does not resolve compiler
 * runtime helpers for relocatable applications. */
void *__aeabi_memcpy(void *target, const void *source, uint32_t length) {
    uint8_t *output = (uint8_t *)target;
    const uint8_t *input = (const uint8_t *)source;
    uint32_t index;
    for (index = 0; index < length; ++index) output[index] = input[index];
    return target;
}

void *__aeabi_memcpy4(void *target, const void *source, uint32_t length) {
    return __aeabi_memcpy(target, source, length);
}

static int copy_text(char *target, uint32_t capacity, const char *source) {
    uint32_t length;
    uint32_t index;
    if (!target || capacity == 0u || !source) return SHELLPP_FS_ERR_ARGUMENT;
    length = text_length(source, capacity);
    if (length >= capacity) return SHELLPP_FS_ERR_PATH;
    for (index = 0; index < length; ++index) target[index] = source[index];
    target[length] = '\0';
    return SHELLPP_FS_OK;
}

static int byte_compare(const char *left, const char *right) {
    uint32_t index = 0;
    for (;;) {
        uint8_t a = (uint8_t)left[index];
        uint8_t b = (uint8_t)right[index];
        if (a != b) return a < b ? -1 : 1;
        if (a == 0u) return 0;
        ++index;
    }
}

int shellpp_fs_path_equal(const char *left, const char *right) {
    if (!left || !right) return 0;
    return byte_compare(left, right) == 0;
}

int shellpp_fs_validate_path(const char *path) {
    uint32_t index = 0;
    uint32_t component = 0;
    if (!path || path[0] != '/') return SHELLPP_FS_ERR_PATH;
    if (path[1] == '\0') return SHELLPP_FS_OK;
    for (index = 1u; index < SHELLPP_FS_PATH_CAP; ++index) {
        char value = path[index];
        if (value == '\0') {
            if (component == 0u) return SHELLPP_FS_ERR_PATH;
            return SHELLPP_FS_OK;
        }
        if (value == '/') {
            if (component == 0u) return SHELLPP_FS_ERR_PATH;
            if (component == 1u && path[index - 1u] == '.')
                return SHELLPP_FS_ERR_PATH;
            if (component == 2u && path[index - 2u] == '.' &&
                    path[index - 1u] == '.')
                return SHELLPP_FS_ERR_PATH;
            component = 0u;
        } else {
            ++component;
        }
    }
    return SHELLPP_FS_ERR_PATH;
}

int shellpp_fs_parent(const char *path, char *output, uint32_t capacity) {
    uint32_t length;
    uint32_t slash;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK || !output ||
            capacity < 2u) return SHELLPP_FS_ERR_PATH;
    if (path[1] == '\0') return copy_text(output, capacity, "/");
    length = text_length(path, SHELLPP_FS_PATH_CAP);
    slash = length;
    while (slash > 0u && path[slash - 1u] != '/') --slash;
    if (slash <= 1u) return copy_text(output, capacity, "/");
    /* slash is one past the final separator. Excluding that separator avoids
     * producing invalid parents such as /data/log/ at depth three and below. */
    --slash;
    if (slash + 1u > capacity) return SHELLPP_FS_ERR_PATH;
    for (uint32_t index = 0; index < slash; ++index) output[index] = path[index];
    output[slash] = '\0';
    return SHELLPP_FS_OK;
}

int shellpp_fs_join(const char *base, const char *name, char *output,
        uint32_t capacity) {
    uint32_t base_length;
    uint32_t name_length;
    uint32_t index;
    if (shellpp_fs_validate_path(base) != SHELLPP_FS_OK || !name || !output)
        return SHELLPP_FS_ERR_PATH;
    name_length = text_length(name, SHELLPP_FS_NAME_CAP);
    if (name_length == 0u || name_length >= SHELLPP_FS_NAME_CAP ||
            (name_length == 1u && name[0] == '.') ||
            (name_length == 2u && name[0] == '.' && name[1] == '.'))
        return SHELLPP_FS_ERR_PATH;
    for (index = 0; index < name_length; ++index)
        if (name[index] == '/') return SHELLPP_FS_ERR_PATH;
    base_length = text_length(base, SHELLPP_FS_PATH_CAP);
    if (base_length + name_length + (base_length > 1u ? 1u : 0u) + 1u >
            capacity) return SHELLPP_FS_ERR_PATH;
    for (index = 0; index < base_length; ++index) output[index] = base[index];
    if (base_length > 1u) output[base_length++] = '/';
    for (index = 0; index < name_length; ++index)
        output[base_length + index] = name[index];
    output[base_length + name_length] = '\0';
    return SHELLPP_FS_OK;
}

const char *shellpp_fs_basename(const char *path) {
    uint32_t index;
    uint32_t last = 0;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK) return 0;
    for (index = 0; path[index]; ++index) if (path[index] == '/') last = index + 1u;
    return path + last;
}

static int entry_compare(const struct shellpp_fs_entry *left,
        const struct shellpp_fs_entry *right) {
    if (left->is_dir != right->is_dir) return left->is_dir ? -1 : 1;
    return byte_compare(left->name, right->name);
}

static int entry_after_cursor(const struct shellpp_fs_entry *entry,
        const struct shellpp_fs_cursor *cursor) {
    if (!cursor || !cursor->valid) return 1;
    if (entry->is_dir != cursor->is_dir) return cursor->is_dir ? 1 : 0;
    return byte_compare(entry->name, cursor->name) > 0;
}

static int entry_before_cursor(const struct shellpp_fs_entry *entry,
        const struct shellpp_fs_cursor *cursor) {
    if (!cursor || !cursor->valid) return 0;
    if (entry->is_dir != cursor->is_dir) return entry->is_dir ? 1 : 0;
    return byte_compare(entry->name, cursor->name) < 0;
}

static void entry_to_cursor(const struct shellpp_fs_entry *entry,
        struct shellpp_fs_cursor *cursor) {
    cursor->valid = 1u;
    cursor->is_dir = entry->is_dir;
    (void)copy_text(cursor->name, sizeof(cursor->name), entry->name);
}

static int insert_candidate(const struct shellpp_fs_entry *entry,
        uint32_t *count) {
    uint32_t position = 0;
    uint32_t limit = SHELLPP_FS_DIR_PAGE_ENTRIES + 1u;
    while (position < *count && entry_compare(&g_candidates[position], entry) < 0)
        ++position;
    if (position >= limit) return 0;
    if (*count < limit) ++*count;
    for (uint32_t index = *count - 1u; index > position; --index)
        g_candidates[index] = g_candidates[index - 1u];
    g_candidates[position] = *entry;
    return 0;
}

int shellpp_fs_file_size(const char *path, uint32_t *size, uint8_t *saturated) {
    int32_t fd;
    int64_t result;
    int32_t close_result;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK || !size)
        return SHELLPP_FS_ERR_ARGUMENT;
    fd = VELA_OPEN(path, VELA_O_RDONLY, 0u);
    if (fd < 0) return SHELLPP_FS_ERR_OPEN;
    result = VELA_LSEEK(fd, 0, VELA_SEEK_END);
    close_result = VELA_CLOSE(fd);
    if (result < 0) return SHELLPP_FS_ERR_SEEK;
    if (close_result < 0) return SHELLPP_FS_ERR_CLOSE;
    if ((uint64_t)result > 0xffffffffu) {
        *size = 0xffffffffu;
        if (saturated) *saturated = 1u;
    } else {
        *size = (uint32_t)result;
        if (saturated) *saturated = 0u;
    }
    return SHELLPP_FS_OK;
}

int shellpp_fs_path_type(const char *path, uint8_t *exists, uint8_t *type) {
    void *directory;
    uint8_t *raw;
    const char *name;
    int result;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK || !exists || !type)
        return SHELLPP_FS_ERR_ARGUMENT;
    *exists = 0u;
    *type = 0u;
    if (path[1] == '\0') {
        *exists = 1u;
        *type = VELA_DT_DIR;
        return SHELLPP_FS_OK;
    }
    name = shellpp_fs_basename(path);
    result = shellpp_fs_parent(path, g_work_path, sizeof(g_work_path));
    if (result != SHELLPP_FS_OK) return result;
    directory = VELA_OPENDIR(g_work_path);
    if (!directory) return SHELLPP_FS_ERR_DIRECTORY;
    while ((raw = VELA_READDIR(directory)) != 0) {
        if (byte_compare((const char *)(raw + 1u), name) == 0) {
            *exists = 1u;
            *type = raw[0];
            break;
        }
    }
    if (VELA_CLOSEDIR(directory) < 0) return SHELLPP_FS_ERR_CLOSE;
    return SHELLPP_FS_OK;
}

int shellpp_fs_list_page(const char *path,
        const struct shellpp_fs_cursor *after, struct shellpp_fs_page *page) {
    void *directory;
    uint8_t *raw;
    uint32_t count = 0;
    struct shellpp_fs_page result;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK || !page)
        return SHELLPP_FS_ERR_ARGUMENT;
    directory = VELA_OPENDIR(path);
    if (!directory) return SHELLPP_FS_ERR_DIRECTORY;
    clear_bytes(&result, sizeof(result));
    clear_bytes(g_candidates, sizeof(g_candidates));
    while ((raw = VELA_READDIR(directory)) != 0) {
        struct shellpp_fs_entry entry;
        const char *name = (const char *)(raw + 1u);
        uint32_t name_length = text_length(name, SHELLPP_FS_NAME_CAP);
        clear_bytes(&entry, sizeof(entry));
        if (name_length == 0u || name_length >= SHELLPP_FS_NAME_CAP ||
                (name_length == 1u && name[0] == '.') ||
                (name_length == 2u && name[0] == '.' && name[1] == '.'))
            continue;
        (void)copy_text(entry.name, sizeof(entry.name), name);
        entry.type = raw[0];
        entry.is_dir = raw[0] == VELA_DT_DIR;
        entry.is_link = raw[0] == VELA_DT_LNK;
        if (!entry_after_cursor(&entry, after)) continue;
        if (raw[0] == VELA_DT_REG &&
                shellpp_fs_join(path, name, g_work_path, sizeof(g_work_path)) ==
                    SHELLPP_FS_OK &&
                shellpp_fs_file_size(g_work_path, &entry.size, 0) ==
                    SHELLPP_FS_OK)
            entry.size_known = 1u;
        (void)insert_candidate(&entry, &count);
    }
    if (VELA_CLOSEDIR(directory) < 0) return SHELLPP_FS_ERR_CLOSE;
    result.count = count > SHELLPP_FS_DIR_PAGE_ENTRIES ?
        SHELLPP_FS_DIR_PAGE_ENTRIES : (uint8_t)count;
    result.has_next = count > SHELLPP_FS_DIR_PAGE_ENTRIES;
    for (uint32_t index = 0; index < result.count; ++index)
        result.entries[index] = g_candidates[index];
    if (result.count) {
        entry_to_cursor(&result.entries[0], &result.first);
        entry_to_cursor(&result.entries[result.count - 1u], &result.last);
    }
    *page = result;
    return SHELLPP_FS_OK;
}

int shellpp_fs_previous_cursor(const char *path,
        const struct shellpp_fs_cursor *before,
        struct shellpp_fs_cursor *after) {
    void *directory;
    uint8_t *raw;
    uint32_t count = 0;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK || !before ||
            !before->valid || !after) return SHELLPP_FS_ERR_ARGUMENT;
    directory = VELA_OPENDIR(path);
    if (!directory) return SHELLPP_FS_ERR_DIRECTORY;
    clear_bytes(g_candidates, sizeof(g_candidates));
    while ((raw = VELA_READDIR(directory)) != 0) {
        struct shellpp_fs_entry entry;
        const char *name = (const char *)(raw + 1u);
        uint32_t name_length = text_length(name, SHELLPP_FS_NAME_CAP);
        uint32_t position;
        clear_bytes(&entry, sizeof(entry));
        if (name_length == 0u || name_length >= SHELLPP_FS_NAME_CAP ||
                (name_length == 1u && name[0] == '.') ||
                (name_length == 2u && name[0] == '.' && name[1] == '.'))
            continue;
        (void)copy_text(entry.name, sizeof(entry.name), name);
        entry.type = raw[0];
        entry.is_dir = raw[0] == VELA_DT_DIR;
        entry.is_link = raw[0] == VELA_DT_LNK;
        if (!entry_before_cursor(&entry, before)) continue;
        if (count < SHELLPP_FS_DIR_PAGE_ENTRIES + 1u) {
            position = count++;
            while (position > 0u &&
                    entry_compare(&g_candidates[position - 1u], &entry) > 0) {
                g_candidates[position] = g_candidates[position - 1u];
                --position;
            }
            g_candidates[position] = entry;
        } else if (entry_compare(&entry, &g_candidates[0]) > 0) {
            g_candidates[0] = entry;
            position = 0u;
            while (position + 1u < count &&
                    entry_compare(&g_candidates[position],
                        &g_candidates[position + 1u]) > 0) {
                struct shellpp_fs_entry swap = g_candidates[position];
                g_candidates[position] = g_candidates[position + 1u];
                g_candidates[position + 1u] = swap;
                ++position;
            }
        }
    }
    if (VELA_CLOSEDIR(directory) < 0) return SHELLPP_FS_ERR_CLOSE;
    clear_bytes(after, sizeof(*after));
    if (count > SHELLPP_FS_DIR_PAGE_ENTRIES)
        entry_to_cursor(&g_candidates[0], after);
    return SHELLPP_FS_OK;
}

int shellpp_fs_read_at(const char *path, uint32_t offset, uint8_t *buffer,
        uint32_t capacity, uint32_t *read_count) {
    int32_t fd;
    int32_t result;
    uint32_t total = 0;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK || !buffer ||
            !read_count) return SHELLPP_FS_ERR_ARGUMENT;
    fd = VELA_OPEN(path, VELA_O_RDONLY, 0u);
    if (fd < 0) return SHELLPP_FS_ERR_OPEN;
    if (offset && VELA_LSEEK(fd, (int64_t)offset, VELA_SEEK_SET) < 0) {
        (void)VELA_CLOSE(fd);
        return SHELLPP_FS_ERR_SEEK;
    }
    while (total < capacity) {
        result = VELA_READ(fd, buffer + total, capacity - total);
        if (result < 0) {
            (void)VELA_CLOSE(fd);
            return SHELLPP_FS_ERR_READ;
        }
        if (result == 0) break;
        total += (uint32_t)result;
    }
    if (VELA_CLOSE(fd) < 0) return SHELLPP_FS_ERR_CLOSE;
    *read_count = total;
    return SHELLPP_FS_OK;
}

static int extension_equal(const char *extension, const char *candidate) {
    while (*extension && *candidate) {
        char left = *extension++;
        char right = *candidate++;
        if (left >= 'A' && left <= 'Z') left = (char)(left + ('a' - 'A'));
        if (right >= 'A' && right <= 'Z') right = (char)(right + ('a' - 'A'));
        if (left != right) return 0;
    }
    return *extension == '\0' && *candidate == '\0';
}

int shellpp_fs_is_editable(const char *path) {
    static const char *const extensions[] = {
        "txt", "json", "log", "lua", "md", "csv",
        "ini", "conf", "cfg", "xml", "prop", "sh"
    };
    const char *name = shellpp_fs_basename(path);
    const char *extension = 0;
    uint32_t index;
    if (!name) return 0;
    for (index = 0; name[index]; ++index) if (name[index] == '.') extension = name + index + 1u;
    if (!extension || !*extension) return 0;
    for (index = 0; index < sizeof(extensions) / sizeof(extensions[0]); ++index)
        if (extension_equal(extension, extensions[index])) return 1;
    return 0;
}

static int write_all(int32_t fd, const uint8_t *data, uint32_t length) {
    uint32_t offset = 0;
    while (offset < length) {
        int32_t result = VELA_WRITE(fd, data + offset, length - offset);
        if (result <= 0) return SHELLPP_FS_ERR_WRITE;
        offset += (uint32_t)result;
    }
    return SHELLPP_FS_OK;
}

static int make_temp_path(const char *path) {
    static const char suffix[] = ".shellpp.tmp";
    uint32_t length = text_length(path, SHELLPP_FS_PATH_CAP);
    uint32_t suffix_length = sizeof(suffix);
    if (length >= SHELLPP_FS_PATH_CAP || length + suffix_length >
            sizeof(g_work_path)) return SHELLPP_FS_ERR_PATH;
    for (uint32_t index = 0; index < length; ++index) g_work_path[index] = path[index];
    for (uint32_t index = 0; index < suffix_length; ++index)
        g_work_path[length + index] = suffix[index];
    return SHELLPP_FS_OK;
}

int shellpp_fs_save_atomic(const char *path, const uint8_t *data,
        uint32_t length) {
    int32_t fd;
    int result;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK || (!data && length))
        return SHELLPP_FS_ERR_ARGUMENT;
    if (make_temp_path(path) != SHELLPP_FS_OK) return SHELLPP_FS_ERR_PATH;
    fd = VELA_OPEN(g_work_path, VELA_O_WRONLY | VELA_O_CREAT | VELA_O_TRUNC,
        0666u);
    if (fd < 0) return SHELLPP_FS_ERR_OPEN;
    result = write_all(fd, data, length);
    if (VELA_CLOSE(fd) < 0 && result == SHELLPP_FS_OK) result = SHELLPP_FS_ERR_CLOSE;
    if (result != SHELLPP_FS_OK) {
        (void)VELA_UNLINK(g_work_path);
        return result;
    }
    if (VELA_RENAME(g_work_path, path) < 0) {
        (void)VELA_UNLINK(g_work_path);
        return SHELLPP_FS_ERR_RENAME;
    }
    return SHELLPP_FS_OK;
}

int shellpp_fs_copy(const char *source, const char *target, uint8_t *scratch,
        uint32_t scratch_size) {
    int32_t input;
    int32_t output;
    int32_t count;
    int result = SHELLPP_FS_OK;
    uint32_t expected;
    uint32_t copied = 0;
    uint8_t exists;
    uint8_t type;
    if (shellpp_fs_validate_path(source) != SHELLPP_FS_OK ||
            shellpp_fs_validate_path(target) != SHELLPP_FS_OK || !scratch ||
            scratch_size < SHELLPP_FS_COPY_CHUNK) return SHELLPP_FS_ERR_ARGUMENT;
    if (shellpp_fs_path_equal(source, target)) return SHELLPP_FS_ERR_SAME_PATH;
    result = shellpp_fs_path_type(source, &exists, &type);
    if (result != SHELLPP_FS_OK) return result;
    if (!exists || type != VELA_DT_REG) return SHELLPP_FS_ERR_UNSAFE_TYPE;
    result = shellpp_fs_path_type(target, &exists, &type);
    if (result != SHELLPP_FS_OK) return result;
    if (exists) return SHELLPP_FS_ERR_EXISTS;
    if (shellpp_fs_file_size(source, &expected, 0) != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_OPEN;
    if (make_temp_path(target) != SHELLPP_FS_OK) return SHELLPP_FS_ERR_PATH;
    input = VELA_OPEN(source, VELA_O_RDONLY, 0u);
    if (input < 0) return SHELLPP_FS_ERR_OPEN;
    output = VELA_OPEN(g_work_path, VELA_O_WRONLY | VELA_O_CREAT | VELA_O_TRUNC,
        0666u);
    if (output < 0) {
        (void)VELA_CLOSE(input);
        return SHELLPP_FS_ERR_OPEN;
    }
    for (;;) {
        count = VELA_READ(input, scratch, SHELLPP_FS_COPY_CHUNK);
        if (count < 0) { result = SHELLPP_FS_ERR_READ; break; }
        if (count == 0) break;
        result = write_all(output, scratch, (uint32_t)count);
        if (result != SHELLPP_FS_OK) break;
        copied += (uint32_t)count;
    }
    if (VELA_CLOSE(input) < 0 && result == SHELLPP_FS_OK) result = SHELLPP_FS_ERR_CLOSE;
    if (VELA_CLOSE(output) < 0 && result == SHELLPP_FS_OK) result = SHELLPP_FS_ERR_CLOSE;
    if (result == SHELLPP_FS_OK && copied != expected) result = SHELLPP_FS_ERR_TRUNCATED;
    if (result == SHELLPP_FS_OK && VELA_RENAME(g_work_path, target) < 0)
        result = SHELLPP_FS_ERR_RENAME;
    if (result != SHELLPP_FS_OK) (void)VELA_UNLINK(g_work_path);
    return result;
}

int shellpp_fs_move(const char *source, const char *target, uint8_t *scratch,
        uint32_t scratch_size) {
    int result;
    uint8_t exists;
    uint8_t type;
    if (shellpp_fs_path_equal(source, target)) return SHELLPP_FS_ERR_SAME_PATH;
    result = shellpp_fs_path_type(source, &exists, &type);
    if (result != SHELLPP_FS_OK) return result;
    if (!exists || type != VELA_DT_REG) return SHELLPP_FS_ERR_UNSAFE_TYPE;
    result = shellpp_fs_path_type(target, &exists, &type);
    if (result != SHELLPP_FS_OK) return result;
    if (exists) return SHELLPP_FS_ERR_EXISTS;
    if (VELA_RENAME(source, target) == 0) return SHELLPP_FS_OK;
    result = shellpp_fs_copy(source, target, scratch, scratch_size);
    if (result != SHELLPP_FS_OK) return result;
    if (VELA_UNLINK(source) < 0) return SHELLPP_FS_ERR_DELETE;
    return SHELLPP_FS_OK;
}

int shellpp_fs_delete_file(const char *path) {
    uint8_t exists;
    uint8_t type;
    int result;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_PATH;
    result = shellpp_fs_path_type(path, &exists, &type);
    if (result != SHELLPP_FS_OK) return result;
    if (!exists || (type != VELA_DT_REG && type != VELA_DT_LNK))
        return SHELLPP_FS_ERR_UNSAFE_TYPE;
    return VELA_UNLINK(path) == 0 ? SHELLPP_FS_OK : SHELLPP_FS_ERR_DELETE;
}

static int root_type(const char *path, uint8_t *exists) {
    void *directory;
    uint8_t *raw;
    const char *name = shellpp_fs_basename(path);
    int result;
    if (!name) return SHELLPP_FS_ERR_PATH;
    *exists = 0u;
    result = shellpp_fs_parent(path, g_work_path, sizeof(g_work_path));
    if (result != SHELLPP_FS_OK) return result;
    directory = VELA_OPENDIR(g_work_path);
    if (!directory) return SHELLPP_FS_ERR_DIRECTORY;
    while ((raw = VELA_READDIR(directory)) != 0) {
        if (byte_compare((const char *)(raw + 1u), name) == 0) {
            uint8_t type = raw[0];
            *exists = 1u;
            if (VELA_CLOSEDIR(directory) < 0) return SHELLPP_FS_ERR_CLOSE;
            return type == VELA_DT_DIR ? SHELLPP_FS_OK :
                SHELLPP_FS_ERR_UNSAFE_TYPE;
        }
    }
    if (VELA_CLOSEDIR(directory) < 0) return SHELLPP_FS_ERR_CLOSE;
    return SHELLPP_FS_OK;
}

static void add_saturated(uint32_t *value, uint32_t amount) {
    if (0xffffffffu - *value < amount) *value = 0xffffffffu;
    else *value += amount;
}

static int walk_directory(uint32_t path_length, uint32_t depth, uint8_t clear,
        struct shellpp_cache_root_report *report) {
    void *directory;
    uint8_t *raw;
    int result = SHELLPP_FS_OK;
    if (depth > WALK_DEPTH_LIMIT) { ++report->failed; return SHELLPP_FS_ERR_PATH; }
    directory = VELA_OPENDIR(g_work_path);
    if (!directory) { ++report->failed; return SHELLPP_FS_ERR_DIRECTORY; }
    while ((raw = VELA_READDIR(directory)) != 0) {
        const char *name = (const char *)(raw + 1u);
        uint32_t name_length = text_length(name, SHELLPP_FS_NAME_CAP);
        uint32_t child_length;
        uint32_t size;
        if (name_length == 0u || name_length >= SHELLPP_FS_NAME_CAP ||
                (name_length == 1u && name[0] == '.') ||
                (name_length == 2u && name[0] == '.' && name[1] == '.'))
            continue;
        child_length = path_length + (path_length > 1u ? 1u : 0u) + name_length;
        if (child_length + 1u > sizeof(g_work_path)) { ++report->failed; continue; }
        if (path_length > 1u) g_work_path[path_length] = '/';
        for (uint32_t index = 0; index < name_length; ++index)
            g_work_path[path_length + (path_length > 1u ? 1u : 0u) + index] =
                name[index];
        g_work_path[child_length] = '\0';
        if (shellpp_fs_path_equal(g_work_path, g_icon_path)) {
            ++report->skipped;
        } else if (raw[0] == VELA_DT_DIR) {
            int child_result = walk_directory(child_length, depth + 1u, clear, report);
            if (child_result != SHELLPP_FS_OK) result = child_result;
            g_work_path[child_length] = '\0';
            if (clear) {
                if (VELA_RMDIR(g_work_path) == 0) ++report->deleted;
                else { ++report->failed; result = SHELLPP_FS_ERR_DELETE; }
            }
        } else if (raw[0] == VELA_DT_REG) {
            if (shellpp_fs_file_size(g_work_path, &size, 0) == SHELLPP_FS_OK)
                add_saturated(&report->bytes, size);
            else ++report->failed;
            if (clear) {
                if (VELA_UNLINK(g_work_path) == 0) ++report->deleted;
                else { ++report->failed; result = SHELLPP_FS_ERR_DELETE; }
            }
        } else {
            /* Links and unknown device types are never opened or followed. */
            ++report->skipped;
        }
        g_work_path[path_length] = '\0';
    }
    if (VELA_CLOSEDIR(directory) < 0) { ++report->failed; result = SHELLPP_FS_ERR_CLOSE; }
    return result;
}

static void setup_cache_report(uint8_t include_logs,
        struct shellpp_cache_report *report) {
    clear_bytes(report, sizeof(*report));
    report->roots[0].path = g_cache_path;
    report->roots[1].path = g_tmp_path;
    report->roots[2].path = g_system_log_path;
    report->roots[3].path = g_offline_log_path;
    report->roots[4].path = g_shellpp_logs_path;
    /* System log roots are always visible and cleanable. Shell++'s own log
     * directory remains the explicit optional category. */
    report->root_count = include_logs ? 5u : 4u;
}

static int scan_cache(uint8_t include_logs, uint8_t clear,
        struct shellpp_cache_report *report) {
    int overall = SHELLPP_FS_OK;
    setup_cache_report(include_logs, report);
    for (uint32_t index = 0; index < report->root_count; ++index) {
        struct shellpp_cache_root_report *root = &report->roots[index];
        int check = root_type(root->path, &root->exists);
        if (check != SHELLPP_FS_OK) {
            if (root->exists) { ++root->failed; overall = check; }
            continue;
        }
        if (!root->exists) continue;
        (void)copy_text(g_work_path, sizeof(g_work_path), root->path);
        if (walk_directory(text_length(g_work_path, sizeof(g_work_path)), 0u,
                clear, root) != SHELLPP_FS_OK) overall = SHELLPP_FS_ERR_DIRECTORY;
        add_saturated(&report->before_bytes, root->bytes);
    }
    return overall;
}

int shellpp_fs_cache_status(uint8_t include_logs,
        struct shellpp_cache_report *report) {
    if (!report) return SHELLPP_FS_ERR_ARGUMENT;
    return scan_cache(include_logs, 0u, report);
}

int shellpp_fs_cache_clear(uint8_t include_logs,
        struct shellpp_cache_report *report) {
    struct shellpp_cache_report after;
    int result;
    if (!report) return SHELLPP_FS_ERR_ARGUMENT;
    result = scan_cache(include_logs, 1u, report);
    if (scan_cache(include_logs, 0u, &after) != SHELLPP_FS_OK &&
            result == SHELLPP_FS_OK) result = SHELLPP_FS_ERR_DIRECTORY;
    report->after_bytes = after.before_bytes;
    report->freed_bytes = report->before_bytes >= report->after_bytes ?
        report->before_bytes - report->after_bytes : 0u;
    return result;
}
