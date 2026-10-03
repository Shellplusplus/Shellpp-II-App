#include "shellpp_ii_module.h"
#include "shellpp_native_app.h"
#include "shellpp_firmware_abi.h"

/* The firmware module loader does not provide compiler unwind personalities.
 * This Supervisor does not unwind, but Clang emits the index reference. */
__attribute__((used, naked)) void __aeabi_unwind_cpp_pr0(void) { __asm__("bx lr"); }

typedef int (*register_driver_t)(const char *, void *, unsigned int, void *);
typedef int (*unregister_driver_t)(const char *);
#define REGISTER_DRIVER ((register_driver_t)SHELLPP_ABI_REGISTER_DRIVER_ADDR)
#define UNREGISTER_DRIVER ((unregister_driver_t)SHELLPP_ABI_UNREGISTER_DRIVER_ADDR)
#define SHELLPP_DEVICE "/dev/shellpp"
#define SHELLPP_MAGIC 0x53505331u
#define SHELLPP_STATUS_ABI 3u
#define SHELLPP_CMD_RESTORE 0x5351000au
#define SHELLPP_CMD_INSTALL 0x53510002u
#define SHELLPP_CMD_UNINSTALL 0x53510003u
#define SHELLPP_CMD_NOTIFY_LOADED 0x53510004u
#define RESULT_COMPLETED 5u
#define RESULT_RUNNING 8u
#define RESULT_FAILED 15u

#define SHELLPP_CMD_INSTALL_DQ 0x53510012u
#define SHELLPP_CMD_UNINSTALL_DQ 0x53510013u
#define SHELLPP_CMD_NOTIFY_DQ 0x53510014u
#define SHELLPP_CMD_SETTINGS_DQ 0x5351001bu
#define SHELLPP_DQ_SETTINGS_BUILD 0x04350901u

int shellpp_settings_start(void);

typedef int  (*ho_open_t)(const char *, int, ...);
typedef int  (*ho_write_t)(int, const void *, unsigned int);
typedef int  (*ho_close_t)(int);
typedef int  (*ho_lseek_t)(int, int, int);
typedef void *(*ho_timer_create_t)(void *cb, unsigned int period, void *ud);

#define HO_LOG_PATH "/data/shellpp-ii-supervisor.log"

#define HO_OPEN   ((ho_open_t) SHELLPP_ABI_OPEN_ADDR)
#define HO_WRITE  ((ho_write_t) SHELLPP_ABI_WRITE_ADDR)
#define HO_CLOSE  ((ho_close_t) SHELLPP_ABI_CLOSE_ADDR)
#define HO_SEEK   ((ho_lseek_t) SHELLPP_ABI_LSEEK_ADDR)
#define HO_TIMER_CREATE ((ho_timer_create_t) SHELLPP_ABI_LV_TIMER_CREATE_ADDR)

#define HO_PERIOD_MS 50u
#define HO_RING 8u

static volatile unsigned int g_command, g_stage, g_state, g_registered;
static volatile int g_error;

static volatile unsigned int g_dq_cmd[HO_RING], g_dq_stage[HO_RING];
static unsigned int g_dq_head , g_dq_tail ;
static unsigned int g_dq_drained;
static unsigned int g_dq_tries;
static unsigned int g_dq_live;
static void *g_dq_timer;

static int ho_open_log(void) {
    return HO_OPEN(HO_LOG_PATH, SHELLPP_ABI_O_WRONLY | SHELLPP_ABI_O_CREAT, 0666u);
}
static void ho_emit(char tag, unsigned int a, unsigned int b, int two) {
    char line[20];
    unsigned int i, n = 0;
    int fd = ho_open_log();
    if (fd < 0) return;
    (void)HO_SEEK(fd, 0, SHELLPP_ABI_SEEK_END);
    line[n++] = tag;
    line[n++] = ' ';
    for (i = 0; i < 8; ++i) line[n++] = "0123456789abcdef"[(a >> (28 - i * 4)) & 15u];
    if (two) {
        line[n++] = ' ';
        for (i = 0; i < 8; ++i) line[n++] = "0123456789abcdef"[(b >> (28 - i * 4)) & 15u];
    }
    line[n++] = '\n';
    (void)HO_WRITE(fd, line, n);
    (void)HO_CLOSE(fd);
}
static void ho_log(char tag, int value) { ho_emit(tag, (unsigned int)value, 0u, 0); }
static void ho_log2(char tag, int a, int b) { ho_emit(tag, (unsigned int)a, (unsigned int)b, 1); }

static unsigned int ho_sp(void) {
    unsigned int sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));
    return sp;
}

static int dq_dispatch(unsigned int cmd, unsigned int stage) {
    if (cmd == SHELLPP_CMD_INSTALL_DQ) {
        if (stage > 2u) return -22;
        if (stage == 0u) return 0;
        return shellpp_native_install_stage(stage);
    }
    if (cmd == SHELLPP_CMD_NOTIFY_DQ) return shellpp_native_notify_loaded(stage);
    if (cmd == SHELLPP_CMD_UNINSTALL_DQ) return shellpp_native_uninstall();
    if (cmd == SHELLPP_CMD_SETTINGS_DQ) {
        if (stage != SHELLPP_DQ_SETTINGS_BUILD) return -22;
        return shellpp_settings_start();
    }
    return -22;
}

static unsigned int dq_is_deferred(unsigned int cmd) {
    return cmd == SHELLPP_CMD_INSTALL_DQ || cmd == SHELLPP_CMD_UNINSTALL_DQ ||
           cmd == SHELLPP_CMD_NOTIFY_DQ || cmd == SHELLPP_CMD_SETTINGS_DQ;
}

static void dq_timer_cb(void *timer) {
    unsigned int head = g_dq_head, cmd, stage;
    int rc;
    if (timer != g_dq_timer) return;
    g_dq_live = 1;
    if (head == g_dq_tail) return;
    cmd = g_dq_cmd[head];
    stage = g_dq_stage[head];
    ho_log2('D', (int)cmd, (int)stage);
    rc = dq_dispatch(cmd, stage);
    ho_log2('t', (int)stage, rc);
    g_error = rc;
    g_state = rc == 0 ? RESULT_COMPLETED : RESULT_FAILED;
    g_dq_head = (head + 1u) % HO_RING;
    g_dq_drained++;
}

#define HO_TIMER_TRIES 8u

static void dq_ensure_timer(void) {
    void *t;
    if (g_dq_live || g_dq_tries >= HO_TIMER_TRIES) return;
    ++g_dq_tries;
    t = HO_TIMER_CREATE((void *)dq_timer_cb, HO_PERIOD_MS, 0);
    ho_log('C', (int)(long)t);
    if (t) {
        g_dq_timer = t;
    } else if (g_dq_tries >= HO_TIMER_TRIES) {
        g_state = RESULT_FAILED;
    }
}

static int dq_enqueue(unsigned int cmd, unsigned int stage) {
    unsigned int tail = g_dq_tail, next = (tail + 1u) % HO_RING;
    dq_ensure_timer();
    if (next == g_dq_head) {
        ho_log2('B', (int)cmd, (int)stage);
        return -16;
    }
    g_dq_cmd[tail] = cmd;
    g_dq_stage[tail] = stage;
    __asm__ volatile("" ::: "memory");
    g_dq_tail = next;
    ho_log2('Q', (int)cmd, (int)stage);
    return 0;
}

struct file_operations_prefix { void *open; void *close; void *read; void *write; void *reserved[8]; };
static struct file_operations_prefix g_fops;
/* Keep the standard writable initialized-data section expected by modlib. */
__attribute__((used, section(".data"))) static volatile unsigned int g_data_anchor = 1;
static unsigned char g_status[384];

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
    *(unsigned int *)(g_status + 44) = SHELLPP_ABI_FIRMWARE_CODE;
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
        rc = shellpp_native_notify_loaded(g_stage);
    } else if (g_command == SHELLPP_CMD_RESTORE ||
        (g_command == SHELLPP_CMD_INSTALL && g_stage == 0)) {
        rc = 0;
    } else if (g_command == SHELLPP_CMD_INSTALL &&
               (g_stage == 1 || g_stage == 2)) {
        rc = shellpp_native_install_stage(g_stage);
    } else if (g_command == SHELLPP_CMD_UNINSTALL) {
        rc = shellpp_native_uninstall();
    } else if (dq_is_deferred(g_command)) {
        rc = dq_enqueue(g_command, g_stage);
    } else {
        rc = -22;
    }
    if (rc == 0 && dq_is_deferred(g_command)) {
        g_state = RESULT_RUNNING;
    } else {
        g_error = rc;
        g_state = rc == 0 ? RESULT_COMPLETED : RESULT_FAILED;
    }
    /* The Canopus device protocol reports command failure through its status
     * snapshot. Preserve the completed write so Lua can read that status. */
    return 16;
}

static int control_write_bb(void *file, const void *buffer, unsigned int count) {
    if (buffer && count >= 16u) {
        const unsigned int *p = (const unsigned int *)buffer;
        if (p[0] == SHELLPP_MAGIC) {
            ho_log2('W', (int)p[1], (int)p[2]);
            ho_log('S', (int)ho_sp());
        }
    }
    return control_write(file, buffer, count);
}

#define HO_FPOS_OFF 8
static unsigned int fpos_get(void *file, int *eof) {
    long long pos = 0;
    *eof = 0;
    if (file) pos = *(volatile long long *)((char *)file + HO_FPOS_OFF);
    if (pos <= 0) return 0u;
    if (pos >= (long long)sizeof(g_status)) { *eof = 1; return (unsigned int)sizeof(g_status); }
    return (unsigned int)pos;
}
static void fpos_set(void *file, unsigned int pos) {
    if (file) *(volatile long long *)((char *)file + HO_FPOS_OFF) = (long long)pos;
}

static int control_read_win(void *file, void *buffer, unsigned int count) {
    unsigned int i, off, n;
    int eof;
    if (!buffer) return -22;
    if (g_dq_tail != 0u) dq_ensure_timer();
    off = fpos_get(file, &eof);
    if (eof) return 0;
    (void)control_read(file, g_status, sizeof(g_status));
    n = (unsigned int)sizeof(g_status) - off;
    if (n > count) n = count;
    for (i = 0; i < n; ++i) ((unsigned char *)buffer)[i] = g_status[off + i];
    fpos_set(file, off + n);
    return (int)n;
}

static void shellpp_supervisor_ctor(void) __attribute__((constructor));
static void shellpp_supervisor_ctor(void) {
    unsigned int i; int rc;
    for (i = 0; i < sizeof(g_fops); ++i) ((unsigned char *)&g_fops)[i] = 0;
    g_fops.open = (void *)control_open; g_fops.close = (void *)control_close;
    g_fops.read = (void *)control_read_win;
    g_fops.write = (void *)control_write_bb;
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
