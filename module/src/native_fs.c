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
static const char g_cpu_path[] = "/proc/cpuload";
static const char g_memory_path[] = "/proc/meminfo";

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

int shellpp_fs_reader_open(const char *path,
        struct shellpp_fs_reader *reader) {
    int32_t fd;
    if (!reader || shellpp_fs_validate_path(path) != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_ARGUMENT;
    reader->fd = -1;
    fd = VELA_OPEN(path, VELA_O_RDONLY, 0u);
    if (fd < 0) return SHELLPP_FS_ERR_OPEN;
    reader->fd = fd;
    return SHELLPP_FS_OK;
}

int shellpp_fs_reader_read(struct shellpp_fs_reader *reader,
        uint8_t *buffer, uint32_t capacity, uint32_t *read_count) {
    int32_t result;
    if (!reader || reader->fd < 0 || !buffer || !capacity || !read_count)
        return SHELLPP_FS_ERR_ARGUMENT;
    result = VELA_READ(reader->fd, buffer, capacity);
    if (result < 0) return SHELLPP_FS_ERR_READ;
    if ((uint32_t)result > capacity) return SHELLPP_FS_ERR_READ;
    *read_count = (uint32_t)result;
    return SHELLPP_FS_OK;
}

void shellpp_fs_reader_close(struct shellpp_fs_reader *reader) {
    if (!reader) return;
    if (reader->fd >= 0) (void)VELA_CLOSE(reader->fd);
    reader->fd = -1;
}

static void cpu_set_text(char *text, uint32_t capacity, const char *value) {
    (void)copy_text(text, capacity, value);
    if (capacity) text[capacity - 1u] = '\0';
}

static void cpu_set_percent_text(char *text, uint32_t capacity,
        uint32_t percent) {
    char digits[11];
    uint32_t digit_count = 0u;
    uint32_t index;
    uint32_t cursor = 0u;
    if (!capacity) return;
    if (percent > 100u) percent = 100u;
    do {
        digits[digit_count++] = (char)('0' + (percent % 10u));
        percent /= 10u;
    } while (percent && digit_count < sizeof(digits));
    if (capacity > 1u) text[cursor++] = 'C';
    if (cursor + 1u < capacity) text[cursor++] = 'P';
    if (cursor + 1u < capacity) text[cursor++] = 'U';
    if (cursor + 1u < capacity) text[cursor++] = ':';
    for (index = digit_count; index > 0u && cursor + 1u < capacity; --index)
        text[cursor++] = digits[index - 1u];
    if (cursor + 1u < capacity) text[cursor++] = '%';
    text[cursor] = '\0';
}

int shellpp_fs_read_cpu(char *text, uint32_t capacity, uint32_t *percent) {
    uint8_t raw[96];
    uint8_t probe;
    uint32_t values[2];
    uint32_t value_count = 0u;
    uint32_t length = 0u;
    uint32_t remaining;
    uint32_t index = 0u;
    int32_t fd;
    int32_t read_count;
    int32_t close_result;
    if (!text || !capacity) return SHELLPP_FS_ERR_ARGUMENT;
    text[0] = '\0';
    if (percent) *percent = 0u;
    fd = VELA_OPEN(g_cpu_path, VELA_O_RDONLY, 0u);
    if (fd < 0) {
        cpu_set_text(text, capacity, "ERR:open");
        return SHELLPP_FS_ERR_OPEN;
    }
    while (length < sizeof(raw) - 1u) {
        remaining = sizeof(raw) - 1u - length;
        read_count = VELA_READ(fd, raw + length, remaining);
        if (read_count < 0 || (uint32_t)read_count > remaining) {
            (void)VELA_CLOSE(fd);
            cpu_set_text(text, capacity, "ERR:read");
            return SHELLPP_FS_ERR_READ;
        }
        if (read_count == 0) break;
        length += (uint32_t)read_count;
    }
    if (length == sizeof(raw) - 1u) {
        read_count = VELA_READ(fd, &probe, 1u);
        if (read_count < 0 || read_count > 1) {
            (void)VELA_CLOSE(fd);
            cpu_set_text(text, capacity, "ERR:read");
            return SHELLPP_FS_ERR_READ;
        }
        if (read_count != 0) {
            (void)VELA_CLOSE(fd);
            cpu_set_text(text, capacity, "ERR:truncated");
            return SHELLPP_FS_ERR_TRUNCATED;
        }
    }
    close_result = VELA_CLOSE(fd);
    if (close_result < 0) {
        cpu_set_text(text, capacity, "ERR:close");
        return SHELLPP_FS_ERR_CLOSE;
    }
    if (length == 0u) {
        cpu_set_text(text, capacity, "ERR:empty");
        return SHELLPP_FS_OK;
    }
    raw[length] = 0u;
    while (index < length && value_count < 2u) {
        uint32_t whole = 0u;
        uint32_t fraction = 0u;
        uint32_t fraction_digits = 0u;
        uint8_t seen_dot = 0u;
        while (index < length &&
                !((raw[index] >= '0' && raw[index] <= '9') ||
                    raw[index] == '.')) ++index;
        if (index >= length) break;
        while (index < length) {
            uint8_t value = raw[index];
            if (value >= '0' && value <= '9') {
                if (!seen_dot) {
                    if (whole < 42949u)
                        whole = whole * 10u + (uint32_t)(value - '0');
                } else if (fraction_digits < 3u) {
                    fraction = fraction * 10u + (uint32_t)(value - '0');
                    ++fraction_digits;
                }
                ++index;
            } else if (value == '.' && !seen_dot) {
                seen_dot = 1u;
                ++index;
            } else {
                break;
            }
        }
        while (fraction_digits < 3u) {
            fraction *= 10u;
            ++fraction_digits;
        }
        /* Keep the scaled value at or below UINT32_MAX / 100 before the
         * two-value percentage calculation below. /proc/cpuload values are
         * normally tiny; this only protects malformed input. */
        values[value_count] = whole > 42949u ? 42949672u :
            whole * 1000u + fraction;
        if (values[value_count] > 42949672u)
            values[value_count] = 42949672u;
        ++value_count;
    }
    if (!value_count) {
        cpu_set_text(text, capacity, "NODATA");
        return SHELLPP_FS_OK;
    }
    {
        uint32_t result = values[0] / 1000u;
        if (value_count >= 2u && values[0])
            result = (values[1] * 100u) / values[0];
        if (result > 100u) result = 100u;
        if (percent) *percent = result;
        cpu_set_percent_text(text, capacity, result);
    }
    return SHELLPP_FS_OK;
}

static int memory_label_equal(const uint8_t *raw, uint32_t length,
        uint32_t offset, const char *label) {
    uint32_t index = 0u;
    while (label[index]) {
        if (offset + index >= length || raw[offset + index] !=
                (uint8_t)label[index]) return 0;
        ++index;
    }
    return 1;
}

static uint32_t memory_find_value(const uint8_t *raw, uint32_t length,
        const char *label, uint8_t *found) {
    uint32_t index;
    if (found) *found = 0u;
    for (index = 0u; index < length; ++index) {
        uint32_t cursor;
        uint32_t value = 0u;
        uint8_t has_digit = 0u;
        if (!memory_label_equal(raw, length, index, label)) continue;
        for (cursor = index; label[cursor - index]; ++cursor) {}
        while (cursor < length && (raw[cursor] == ' ' ||
                raw[cursor] == '\t')) ++cursor;
        while (cursor < length && raw[cursor] >= '0' && raw[cursor] <= '9') {
            if (value <= 429496729u)
                value = value * 10u + (uint32_t)(raw[cursor] - '0');
            has_digit = 1u;
            ++cursor;
        }
        if (has_digit) {
            if (found) *found = 1u;
            return value;
        }
    }
    return 0u;
}

static uint8_t memory_contains_kb(const uint8_t *raw, uint32_t length) {
    uint32_t index;
    for (index = 0u; index + 1u < length; ++index) {
        uint8_t first = raw[index];
        uint8_t second = raw[index + 1u];
        if ((first == 'K' || first == 'k') &&
                (second == 'B' || second == 'b')) return 1u;
    }
    return 0u;
}

static uint32_t memory_next_number(const uint8_t *raw, uint32_t length,
        uint32_t *cursor, uint8_t *found) {
    uint32_t value = 0u;
    if (found) *found = 0u;
    while (*cursor < length && (raw[*cursor] < '0' ||
            raw[*cursor] > '9')) ++*cursor;
    while (*cursor < length && raw[*cursor] >= '0' && raw[*cursor] <= '9') {
        if (value <= 429496729u)
            value = value * 10u + (uint32_t)(raw[*cursor] - '0');
        if (found) *found = 1u;
        ++*cursor;
    }
    return value;
}

static uint32_t memory_next_line_number(const uint8_t *raw, uint32_t end,
        uint32_t *cursor, uint8_t *found) {
    uint32_t value = 0u;
    if (found) *found = 0u;
    while (*cursor < end && (raw[*cursor] < '0' ||
            raw[*cursor] > '9')) ++*cursor;
    while (*cursor < end && raw[*cursor] >= '0' && raw[*cursor] <= '9') {
        if (value <= 429496729u)
            value = value * 10u + (uint32_t)(raw[*cursor] - '0');
        if (found) *found = 1u;
        ++*cursor;
    }
    return value;
}

static uint8_t memory_find_umem(const uint8_t *raw, uint32_t length,
        uint32_t *total, uint32_t *used, uint32_t *free_bytes) {
    uint32_t start = 0u;
    while (start < length) {
        uint32_t end = start;
        uint32_t cursor;
        uint32_t index;
        uint8_t has_a;
        uint8_t has_b;
        uint8_t has_c;
        while (end < length && raw[end] != '\n' && raw[end] != '\r') ++end;
        for (index = start; index + 4u <= end; ++index) {
            if (!memory_label_equal(raw, length, index, "Umem")) continue;
            /* Firmware 3.101.036 prints the allocator name at the end of
             * its statistics row: "total used free ... Umem".  Lua
             * intentionally parses all numbers on that line; start at the
             * line head here too, rather than after the Umem label. */
            cursor = start;
            *total = memory_next_line_number(raw, end, &cursor, &has_a);
            *used = memory_next_line_number(raw, end, &cursor, &has_b);
            *free_bytes = memory_next_line_number(raw, end, &cursor, &has_c);
            if (has_a && has_b && has_c) return 1u;
        }
        while (end < length && (raw[end] == '\n' || raw[end] == '\r')) ++end;
        start = end;
    }
    /* Shell++ Lua next scans all numeric fields for a NuttX-style
     * total/used/free triple.  It deliberately does this even when standard
     * MemTotal fields exist, because Xiaomi's allocator report may be a
     * separate table rather than a literal "Umem:" line. */
    {
        uint32_t scan = 0u;
        uint8_t has_a;
        uint8_t has_b;
        uint8_t has_c;
        while (scan < length) {
            uint32_t a = memory_next_number(raw, length, &scan, &has_a);
            uint32_t b = memory_next_number(raw, length, &scan, &has_b);
            uint32_t c = memory_next_number(raw, length, &scan, &has_c);
            if (!has_a || !has_b || !has_c) break;
            if (a >= 1000u && b <= a && c <= a) {
                *total = a;
                *used = b;
                *free_bytes = c;
                return 1u;
            }
        }
    }
    return 0u;
}

static void memory_append_unsigned(char **cursor, const char *end,
        uint32_t value) {
    char digits[11];
    uint32_t count = 0u;
    do {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value && count < sizeof(digits));
    while (count && *cursor < end) *(*cursor)++ = digits[--count];
}

static void memory_append_text(char **cursor, const char *end,
        const char *text) {
    while (*text && *cursor < end) *(*cursor)++ = *text++;
}

static void memory_append_kb(char **cursor, const char *end, uint32_t kb) {
    if (kb >= 1024u) {
        uint32_t whole = kb / 1024u;
        uint32_t tenths = ((kb % 1024u) * 10u) / 1024u;
        memory_append_unsigned(cursor, end, whole);
        if (*cursor < end) *(*cursor)++ = '.';
        memory_append_unsigned(cursor, end, tenths);
        memory_append_text(cursor, end, "MB");
    } else {
        memory_append_unsigned(cursor, end, kb);
        memory_append_text(cursor, end, "KB");
    }
}

static void memory_set_unavailable(char *text, uint32_t capacity) {
    cpu_set_text(text, capacity, "MEM:N/A");
}

/* Read the live allocator table incrementally. Keep only the current line's
 * first three numbers and the Umem match state. */
static int memory_read_live_umem(uint32_t *total, uint32_t *used,
        uint32_t *free_bytes) {
    uint8_t chunk[96];
    uint32_t numbers[3];
    uint32_t number_value = 0u;
    uint32_t number_count = 0u;
    uint8_t number_active = 0u;
    uint8_t umem_state = 0u;
    int32_t fd = VELA_OPEN(g_memory_path, VELA_O_RDONLY, 0u);
    if (fd < 0) return -1;
    for (;;) {
        int32_t count = VELA_READ(fd, chunk, sizeof(chunk));
        uint32_t index;
        if (count < 0) { (void)VELA_CLOSE(fd); return -1; }
        if (count == 0) break;
        for (index = 0u; index < (uint32_t)count; ++index) {
            uint8_t value = chunk[index];
            if (value == '\n' || value == '\r') {
                if (number_active && number_count < 3u)
                    numbers[number_count++] = number_value;
                if (umem_state == 4u && number_count >= 3u) {
                    *total = numbers[0];
                    *used = numbers[1];
                    *free_bytes = numbers[2];
                    (void)VELA_CLOSE(fd);
                    return 1;
                }
                number_value = 0u;
                number_count = 0u;
                number_active = 0u;
                umem_state = 0u;
            } else {
                if (value >= '0' && value <= '9') {
                    if (!number_active) {
                        number_active = 1u;
                        number_value = 0u;
                    }
                    if (number_value <= 429496729u)
                        number_value = number_value * 10u +
                            (uint32_t)(value - '0');
                } else if (number_active) {
                    if (number_count < 3u) numbers[number_count++] = number_value;
                    number_value = 0u;
                    number_active = 0u;
                }
                if (umem_state == 0u && value == 'U') umem_state = 1u;
                else if (umem_state == 1u && value == 'm') umem_state = 2u;
                else if (umem_state == 2u && value == 'e') umem_state = 3u;
                else if (umem_state == 3u && value == 'm') umem_state = 4u;
                else if (value == 'U') umem_state = 1u;
                else if (value != ' ' && value != '\t') umem_state = 0u;
            }
        }
    }
    if (number_active && number_count < 3u)
        numbers[number_count++] = number_value;
    if (umem_state == 4u && number_count >= 3u) {
        *total = numbers[0];
        *used = numbers[1];
        *free_bytes = numbers[2];
        (void)VELA_CLOSE(fd);
        return 1;
    }
    return VELA_CLOSE(fd) < 0 ? -1 : 0;
}

int shellpp_fs_read_memory(char *text, uint32_t capacity, uint32_t *percent) {
    uint8_t raw[512];
    uint8_t probe;
    uint8_t found_total;
    uint8_t found_free;
    uint8_t found_available;
    uint8_t unit_is_kb;
    uint32_t total;
    uint32_t free_bytes;
    uint32_t available;
    uint32_t buffers;
    uint32_t cached;
    uint32_t used = 0u;
    uint32_t nuttx_total = 0u;
    uint32_t nuttx_used = 0u;
    uint32_t nuttx_free = 0u;
    uint8_t found_used = 0u;
    uint8_t found_nuttx;
    uint32_t result;
    uint32_t length = 0u;
    uint32_t remaining;
    int32_t fd;
    int32_t read_count;
    int32_t close_result;
    char *cursor;
    char *end;
    if (!text || !capacity) return SHELLPP_FS_ERR_ARGUMENT;
    text[0] = '\0';
    if (percent) *percent = 0u;
    /* Xiaomi 3.101.036 exposes the allocator's authoritative live values on
     * the `Umem` row.  This branch scans the complete current stream every
     * call; it has no cached sample or hard-coded statistic. */
    result = (uint32_t)memory_read_live_umem(&nuttx_total, &nuttx_used,
        &nuttx_free);
    if (result == 1u && nuttx_total && nuttx_used <= nuttx_total) {
        total = nuttx_total / 1024u;
        used = nuttx_used / 1024u;
        result = total ? (used * 100u) / total : 0u;
        if (result > 100u) result = 100u;
        if (percent) *percent = result;
        cursor = text;
        end = text + capacity - 1u;
        memory_append_kb(&cursor, end, used);
        if (cursor < end) *cursor++ = '/';
        memory_append_kb(&cursor, end, total);
        if (cursor < end) *cursor++ = ' ';
        memory_append_unsigned(&cursor, end, result);
        if (cursor < end) *cursor++ = '%';
        *cursor = '\0';
        return SHELLPP_FS_OK;
    }
    fd = VELA_OPEN(g_memory_path, VELA_O_RDONLY, 0u);
    if (fd < 0) {
        memory_set_unavailable(text, capacity);
        return SHELLPP_FS_ERR_OPEN;
    }
    while (length < sizeof(raw) - 1u) {
        remaining = sizeof(raw) - 1u - length;
        read_count = VELA_READ(fd, raw + length, remaining);
        if (read_count < 0 || (uint32_t)read_count > remaining) {
            (void)VELA_CLOSE(fd);
            memory_set_unavailable(text, capacity);
            return SHELLPP_FS_ERR_READ;
        }
        if (read_count == 0) break;
        length += (uint32_t)read_count;
    }
    if (length == sizeof(raw) - 1u) {
        read_count = VELA_READ(fd, &probe, 1u);
        if (read_count != 0) {
            (void)VELA_CLOSE(fd);
            memory_set_unavailable(text, capacity);
            return SHELLPP_FS_ERR_TRUNCATED;
        }
    }
    close_result = VELA_CLOSE(fd);
    if (close_result < 0) {
        memory_set_unavailable(text, capacity);
        return SHELLPP_FS_ERR_CLOSE;
    }
    if (!length) {
        memory_set_unavailable(text, capacity);
        return SHELLPP_FS_OK;
    }
    unit_is_kb = memory_contains_kb(raw, length);
    total = memory_find_value(raw, length, "MemTotal:", &found_total);
    if (!found_total) total = memory_find_value(raw, length, "Total:",
        &found_total);
    free_bytes = memory_find_value(raw, length, "MemFree:", &found_free);
    if (!found_free) free_bytes = memory_find_value(raw, length, "Free:",
        &found_free);
    available = memory_find_value(raw, length, "MemAvailable:",
        &found_available);
    buffers = memory_find_value(raw, length, "Buffers:", 0);
    cached = memory_find_value(raw, length, "Cached:", 0);
    /* This matches Shell++ Lua's parseNuttXMemoryInfo(): on Xiaomi's NuttX
     * builds the Umem line carries the allocator's authoritative used value. */
    found_nuttx = memory_find_umem(raw, length, &nuttx_total, &nuttx_used,
        &nuttx_free);
    if (!found_total && found_nuttx) {
        total = nuttx_total;
        free_bytes = nuttx_free;
        available = nuttx_free;
        found_total = 1u;
        found_free = 1u;
        found_available = 1u;
    }
    if (!found_total && !found_nuttx) {
        uint32_t scan = 0u;
        uint8_t have_a;
        uint8_t have_b;
        uint8_t have_c;
        while (scan < length) {
            uint32_t a = memory_next_number(raw, length, &scan, &have_a);
            uint32_t b = memory_next_number(raw, length, &scan, &have_b);
            uint32_t c = memory_next_number(raw, length, &scan, &have_c);
            if (!have_a || !have_b || !have_c) break;
            if (a >= 1000u && b <= a && c <= a) {
                total = a;
                free_bytes = c;
                available = c;
                used = b;
                found_used = 1u;
                found_total = 1u;
                found_free = 1u;
                found_available = 1u;
                break;
            }
        }
    }
    if (!found_total || !total) {
        memory_set_unavailable(text, capacity);
        return SHELLPP_FS_OK;
    }
    if (!unit_is_kb) {
        total /= 1024u;
        free_bytes /= 1024u;
        available /= 1024u;
        buffers /= 1024u;
        cached /= 1024u;
        if (found_nuttx) {
            nuttx_used /= 1024u;
            nuttx_free /= 1024u;
        }
    }
    if (!found_available) {
        available = free_bytes;
        if (available <= 4294967295u - buffers) available += buffers;
        if (available <= 4294967295u - cached) available += cached;
    }
    if (available > total) available = total;
    if (found_nuttx && nuttx_used) used = nuttx_used;
    else if (!found_used) used = total - available;
    if (used > total) used = total;
    result = total ? (used * 100u) / total : 0u;
    if (result > 100u) result = 100u;
    if (percent) *percent = result;
    cursor = text;
    end = text + capacity - 1u;
    memory_append_kb(&cursor, end, used);
    if (cursor < end) *cursor++ = '/';
    memory_append_kb(&cursor, end, total);
    if (cursor < end) *cursor++ = ' ';
    memory_append_unsigned(&cursor, end, result);
    if (cursor < end) *cursor++ = '%';
    *cursor = '\0';
    return SHELLPP_FS_OK;
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

int shellpp_fs_atomic_begin(const char *path,
        struct shellpp_fs_atomic_writer *writer) {
    int32_t fd;
    if (!writer || shellpp_fs_validate_path(path) != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_ARGUMENT;
    writer->fd = -1;
    if (make_temp_path(path) != SHELLPP_FS_OK) return SHELLPP_FS_ERR_PATH;
    fd = VELA_OPEN(g_work_path, VELA_O_WRONLY | VELA_O_CREAT | VELA_O_TRUNC,
        0666u);
    if (fd < 0) return SHELLPP_FS_ERR_OPEN;
    writer->fd = fd;
    return SHELLPP_FS_OK;
}

int shellpp_fs_atomic_write(struct shellpp_fs_atomic_writer *writer,
        const uint8_t *data, uint32_t length) {
    if (!writer || writer->fd < 0 || (!data && length))
        return SHELLPP_FS_ERR_ARGUMENT;
    return length ? write_all(writer->fd, data, length) : SHELLPP_FS_OK;
}

int shellpp_fs_atomic_commit(const char *path,
        struct shellpp_fs_atomic_writer *writer) {
    int result = SHELLPP_FS_OK;
    if (!writer || writer->fd < 0 ||
            shellpp_fs_validate_path(path) != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_ARGUMENT;
    /* Other filesystem reads use g_work_path as scratch while the stream is
     * being produced. Recreate this writer's deterministic temporary path
     * immediately before the final rename. */
    if (make_temp_path(path) != SHELLPP_FS_OK) return SHELLPP_FS_ERR_PATH;
    if (VELA_CLOSE(writer->fd) < 0) result = SHELLPP_FS_ERR_CLOSE;
    writer->fd = -1;
    if (result == SHELLPP_FS_OK && VELA_RENAME(g_work_path, path) < 0)
        result = SHELLPP_FS_ERR_RENAME;
    if (result != SHELLPP_FS_OK) (void)VELA_UNLINK(g_work_path);
    return result;
}

void shellpp_fs_atomic_abort(const char *path,
        struct shellpp_fs_atomic_writer *writer) {
    if (!writer) return;
    if (writer->fd >= 0) (void)VELA_CLOSE(writer->fd);
    writer->fd = -1;
    if (shellpp_fs_validate_path(path) == SHELLPP_FS_OK &&
            make_temp_path(path) == SHELLPP_FS_OK)
        (void)VELA_UNLINK(g_work_path);
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

static int remove_tree_walk(uint32_t path_length, uint32_t depth) {
    void *directory;
    uint8_t *raw;
    int result = SHELLPP_FS_OK;
    if (depth > WALK_DEPTH_LIMIT) return SHELLPP_FS_ERR_PATH;
    directory = VELA_OPENDIR(g_work_path);
    if (!directory) return SHELLPP_FS_ERR_DIRECTORY;
    while ((raw = VELA_READDIR(directory)) != 0) {
        const char *name = (const char *)(raw + 1u);
        uint32_t name_length = text_length(name, SHELLPP_FS_NAME_CAP);
        uint32_t separator = path_length > 1u ? 1u : 0u;
        uint32_t child_length;
        int child_result;
        if (name_length == 0u || name_length >= SHELLPP_FS_NAME_CAP ||
                (name_length == 1u && name[0] == '.') ||
                (name_length == 2u && name[0] == '.' && name[1] == '.'))
            continue;
        child_length = path_length + separator + name_length;
        if (child_length + 1u > sizeof(g_work_path)) {
            result = SHELLPP_FS_ERR_PATH;
            continue;
        }
        if (separator) g_work_path[path_length] = '/';
        for (uint32_t index = 0u; index < name_length; ++index)
            g_work_path[path_length + separator + index] = name[index];
        g_work_path[child_length] = '\0';
        if (raw[0] == VELA_DT_DIR) {
            child_result = remove_tree_walk(child_length, depth + 1u);
            if (child_result != SHELLPP_FS_OK) result = child_result;
            g_work_path[child_length] = '\0';
            if (child_result == SHELLPP_FS_OK && VELA_RMDIR(g_work_path) < 0)
                result = SHELLPP_FS_ERR_DELETE;
        } else if (raw[0] == VELA_DT_REG || raw[0] == VELA_DT_LNK) {
            if (VELA_UNLINK(g_work_path) < 0) result = SHELLPP_FS_ERR_DELETE;
        } else {
            /* Device nodes and unknown types are never removed implicitly. */
            result = SHELLPP_FS_ERR_UNSAFE_TYPE;
        }
        g_work_path[path_length] = '\0';
    }
    if (VELA_CLOSEDIR(directory) < 0 && result == SHELLPP_FS_OK)
        result = SHELLPP_FS_ERR_CLOSE;
    return result;
}

int shellpp_fs_remove_tree(const char *path) {
    uint8_t exists;
    uint8_t type;
    uint32_t length;
    int result;
    if (shellpp_fs_validate_path(path) != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_PATH;
    result = shellpp_fs_path_type(path, &exists, &type);
    if (result != SHELLPP_FS_OK) return result;
    if (!exists) return SHELLPP_FS_OK;
    if (type != VELA_DT_DIR) return SHELLPP_FS_ERR_UNSAFE_TYPE;
    if (copy_text(g_work_path, sizeof(g_work_path), path) != SHELLPP_FS_OK)
        return SHELLPP_FS_ERR_PATH;
    length = text_length(g_work_path, sizeof(g_work_path));
    result = remove_tree_walk(length, 0u);
    if (result != SHELLPP_FS_OK) return result;
    return VELA_RMDIR(g_work_path) == 0 ? SHELLPP_FS_OK :
        SHELLPP_FS_ERR_DELETE;
}

static uint8_t app_package_component_valid(const char *package_name) {
    uint32_t index;
    uint32_t length;
    if (!package_name) return 0u;
    length = text_length(package_name, SHELLPP_FS_NAME_CAP);
    if (!length || length >= SHELLPP_FS_NAME_CAP) return 0u;
    for (index = 0u; index < length; ++index) {
        uint8_t value = (uint8_t)package_name[index];
        if (!((value >= 'a' && value <= 'z') ||
                (value >= 'A' && value <= 'Z') ||
                (value >= '0' && value <= '9') || value == '.' ||
                value == '_' || value == '-')) return 0u;
    }
    return 1u;
}

static void add_saturated(uint32_t *value, uint32_t amount);

static int measure_directory_walk(uint32_t path_length, uint32_t depth,
        uint32_t *total) {
    void *directory;
    uint8_t *raw;
    int result = SHELLPP_FS_OK;
    if (depth > WALK_DEPTH_LIMIT || !total)
        return SHELLPP_FS_ERR_PATH;
    directory = VELA_OPENDIR(g_work_path);
    if (!directory) return SHELLPP_FS_ERR_DIRECTORY;
    while ((raw = VELA_READDIR(directory)) != 0) {
        const char *name = (const char *)(raw + 1u);
        uint32_t name_length = text_length(name, SHELLPP_FS_NAME_CAP);
        uint32_t child_length;
        uint32_t size;
        if (!name_length || name_length >= SHELLPP_FS_NAME_CAP ||
                (name_length == 1u && name[0] == '.') ||
                (name_length == 2u && name[0] == '.' && name[1] == '.'))
            continue;
        child_length = path_length + (path_length > 1u ? 1u : 0u) +
            name_length;
        if (child_length + 1u > sizeof(g_work_path)) {
            result = SHELLPP_FS_ERR_PATH;
            continue;
        }
        if (path_length > 1u) g_work_path[path_length] = '/';
        for (uint32_t index = 0u; index < name_length; ++index)
            g_work_path[path_length + (path_length > 1u ? 1u : 0u) + index] =
                name[index];
        g_work_path[child_length] = '\0';
        if (raw[0] == VELA_DT_DIR) {
            if (measure_directory_walk(child_length, depth + 1u, total) !=
                    SHELLPP_FS_OK)
                result = SHELLPP_FS_ERR_DIRECTORY;
        } else if (raw[0] == VELA_DT_REG) {
            if (shellpp_fs_file_size(g_work_path, &size, 0u) ==
                    SHELLPP_FS_OK)
                add_saturated(total, size);
            else
                result = SHELLPP_FS_ERR_READ;
        }
        g_work_path[path_length] = '\0';
    }
    if (VELA_CLOSEDIR(directory) < 0 && result == SHELLPP_FS_OK)
        result = SHELLPP_FS_ERR_CLOSE;
    return result;
}

int shellpp_fs_app_size(const char *package_name, uint32_t *size) {
    static const char *const roots[] = {
        "/data/app", "/data/quickapp/system", "/data/quickapp/files"
    };
    char path[SHELLPP_FS_PATH_CAP];
    uint32_t index;
    int result = SHELLPP_FS_OK;
    if (!size || !app_package_component_valid(package_name))
        return SHELLPP_FS_ERR_PATH;
    *size = 0u;
    for (index = 0u; index < sizeof(roots) / sizeof(roots[0]); ++index) {
        uint8_t exists;
        uint8_t type;
        int current = shellpp_fs_path_type(roots[index], &exists, &type);
        uint32_t path_length;
        if (current != SHELLPP_FS_OK) {
            result = current;
            continue;
        }
        if (!exists) continue;
        if (type != VELA_DT_DIR) { result = SHELLPP_FS_ERR_UNSAFE_TYPE; continue; }
        if (shellpp_fs_join(roots[index], package_name, path, sizeof(path)) !=
                SHELLPP_FS_OK) return SHELLPP_FS_ERR_PATH;
        current = shellpp_fs_path_type(path, &exists, &type);
        if (current != SHELLPP_FS_OK) { result = current; continue; }
        if (!exists) continue;
        if (type != VELA_DT_DIR) { result = SHELLPP_FS_ERR_UNSAFE_TYPE; continue; }
        if (copy_text(g_work_path, sizeof(g_work_path), path) != SHELLPP_FS_OK)
            return SHELLPP_FS_ERR_PATH;
        path_length = text_length(g_work_path, sizeof(g_work_path));
        current = measure_directory_walk(path_length, 0u, size);
        if (current != SHELLPP_FS_OK) result = current;
    }
    return result;
}

int shellpp_fs_delete_app_package(const char *package_name) {
    static const char *const roots[] = {
        "/data/app",
        "/data/quickapp/system",
        "/data/cache",
        "/data/files",
        "/data/mass",
    };
    char path[SHELLPP_FS_PATH_CAP];
    uint32_t index;
    int result = SHELLPP_FS_OK;
    if (!app_package_component_valid(package_name)) return SHELLPP_FS_ERR_PATH;
    for (index = 0u; index < sizeof(roots) / sizeof(roots[0]); ++index) {
        int current;
        uint8_t exists;
        uint8_t type;
        current = shellpp_fs_path_type(roots[index], &exists, &type);
        if (current != SHELLPP_FS_OK) {
            if (result == SHELLPP_FS_OK) result = current;
            continue;
        }
        if (!exists) continue;
        if (type != VELA_DT_DIR) {
            if (result == SHELLPP_FS_OK) result = SHELLPP_FS_ERR_UNSAFE_TYPE;
            continue;
        }
        if (shellpp_fs_join(roots[index], package_name, path, sizeof(path)) !=
                SHELLPP_FS_OK) return SHELLPP_FS_ERR_PATH;
        current = shellpp_fs_remove_tree(path);
        if (current != SHELLPP_FS_OK && result == SHELLPP_FS_OK) result = current;
    }
    return result;
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
