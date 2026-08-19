#ifndef SHELLPP_NATIVE_FS_H
#define SHELLPP_NATIVE_FS_H

#include <stdint.h>

#define SHELLPP_FS_PATH_CAP 384u
#define SHELLPP_FS_NAME_CAP 72u
#define SHELLPP_FS_DIR_PAGE_ENTRIES 30u
#define SHELLPP_FS_TEXT_LIMIT 4096u
#define SHELLPP_FS_EDIT_LIMIT 12288u
#define SHELLPP_FS_HEX_PAGE_SIZE 2048u
#define SHELLPP_FS_VIEW_LIMIT (3u * 1024u * 1024u)
#define SHELLPP_FS_COPY_CHUNK 4096u
#define SHELLPP_FS_CACHE_ROOTS 5u

enum shellpp_fs_result {
    SHELLPP_FS_OK = 0,
    SHELLPP_FS_ERR_ARGUMENT = -200,
    SHELLPP_FS_ERR_PATH = -201,
    SHELLPP_FS_ERR_OPEN = -202,
    SHELLPP_FS_ERR_READ = -203,
    SHELLPP_FS_ERR_WRITE = -204,
    SHELLPP_FS_ERR_CLOSE = -205,
    SHELLPP_FS_ERR_SEEK = -206,
    SHELLPP_FS_ERR_TOO_LARGE = -207,
    SHELLPP_FS_ERR_RENAME = -208,
    SHELLPP_FS_ERR_DELETE = -209,
    SHELLPP_FS_ERR_DIRECTORY = -210,
    SHELLPP_FS_ERR_NOT_EDITABLE = -211,
    SHELLPP_FS_ERR_SAME_PATH = -212,
    SHELLPP_FS_ERR_TRUNCATED = -213,
    SHELLPP_FS_ERR_UNSAFE_TYPE = -214,
    SHELLPP_FS_ERR_EXISTS = -215,
};

struct shellpp_fs_cursor {
    uint8_t valid;
    uint8_t is_dir;
    char name[SHELLPP_FS_NAME_CAP];
};

struct shellpp_fs_entry {
    char name[SHELLPP_FS_NAME_CAP];
    uint32_t size;
    uint8_t is_dir;
    uint8_t is_link;
    uint8_t size_known;
    uint8_t type;
};

struct shellpp_fs_page {
    struct shellpp_fs_entry entries[SHELLPP_FS_DIR_PAGE_ENTRIES];
    struct shellpp_fs_cursor first;
    struct shellpp_fs_cursor last;
    uint8_t count;
    uint8_t has_next;
};

struct shellpp_cache_root_report {
    const char *path;
    uint32_t bytes;
    uint32_t deleted;
    uint32_t failed;
    uint32_t skipped;
    uint8_t exists;
};

struct shellpp_cache_report {
    struct shellpp_cache_root_report roots[SHELLPP_FS_CACHE_ROOTS];
    uint32_t before_bytes;
    uint32_t after_bytes;
    uint32_t freed_bytes;
    uint8_t root_count;
};

int shellpp_fs_validate_path(const char *path);
int shellpp_fs_parent(const char *path, char *output, uint32_t capacity);
int shellpp_fs_join(const char *base, const char *name, char *output,
    uint32_t capacity);
const char *shellpp_fs_basename(const char *path);
int shellpp_fs_path_equal(const char *left, const char *right);

int shellpp_fs_list_page(const char *path,
    const struct shellpp_fs_cursor *after, struct shellpp_fs_page *page);
int shellpp_fs_previous_cursor(const char *path,
    const struct shellpp_fs_cursor *before, struct shellpp_fs_cursor *after);
int shellpp_fs_file_size(const char *path, uint32_t *size, uint8_t *saturated);
int shellpp_fs_path_type(const char *path, uint8_t *exists, uint8_t *type);
int shellpp_fs_read_at(const char *path, uint32_t offset, uint8_t *buffer,
    uint32_t capacity, uint32_t *read_count);
int shellpp_fs_is_editable(const char *path);
int shellpp_fs_save_atomic(const char *path, const uint8_t *data,
    uint32_t length);
int shellpp_fs_copy(const char *source, const char *target, uint8_t *scratch,
    uint32_t scratch_size);
int shellpp_fs_move(const char *source, const char *target, uint8_t *scratch,
    uint32_t scratch_size);
int shellpp_fs_delete_file(const char *path);

int shellpp_fs_cache_status(uint8_t include_logs,
    struct shellpp_cache_report *report);
int shellpp_fs_cache_clear(uint8_t include_logs,
    struct shellpp_cache_report *report);

#endif
