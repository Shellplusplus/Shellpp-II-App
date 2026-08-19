#include "shellpp_ii_module.h"

/* The firmware loader does not export the compiler unwind personalities.
 * This module never unwinds, so keep the ELF self-contained as the previous
 * Shell++ build did. */
__attribute__((used, naked)) void __aeabi_unwind_cpp_pr0(void) { __asm__("bx lr"); }
__attribute__((used, naked)) void __aeabi_unwind_cpp_pr1(void) { __asm__("bx lr"); }
__attribute__((used, naked)) void __aeabi_unwind_cpp_pr2(void) { __asm__("bx lr"); }
typedef int (*open_t)(const char *, int, ...);
typedef int (*write_t)(int, const void *, unsigned int);
typedef int (*close_t)(int);
typedef int (*read_t)(int, void *, unsigned int);
typedef int (*work_queue_t)(int qid, void *work, void (*worker)(void *), void *arg, unsigned long long delay);
typedef void *(*find_by_id_t)(unsigned short app_id);
typedef void *(*find_launch_page_t)(unsigned short app_id);
typedef int (*launcher_add_t)(unsigned short app_id);

#define SYM_OPEN2  ((open_t)  0x006486E9)
#define SYM_WRITE2 ((write_t) 0x00648C01)
#define SYM_CLOSE2 ((close_t) 0x00603731)
#define SYM_READ2  ((read_t)  0x006487F9)
#define SYM_WQ      ((work_queue_t) 0x00620AF5)
#define ORIG_FIND_BY_ID ((find_by_id_t) 0x010414C1)
#define ORIG_LAUNCH_PAGE ((find_launch_page_t) 0x010416A9)
#define FN_LAUNCHER_ADD ((launcher_add_t) 0x00EFBC81)
#define O_WRONLY (1<<1)
#define O_CREAT  (1<<2)
#define O_TRUNC  (1<<5)
#define O_RDONLY 0
#define LPWORK 1
#define OUR_APP_ID 0x79
#define TABLE_ADDR 0x01571640
#define TABLE_HOOK_SLOT 0x4021AC00
#define AM_HEAD_SLOT 0x4021ac2c
#define OUR_PAGE_ID 0x00790000u

/* ---- Lua ---- */
typedef void *(*lua_newstate_t)(void);
typedef void  (*lua_openlibs_t)(void *L);
typedef int   (*lua_loadbufferx_t)(void *L, const char *buf, unsigned int sz, const char *name, const char *mode);
typedef int   (*lua_pcallk_t)(void *L, int nargs, int nres, int err, long ctx, void *k, long olderr);
typedef void  (*lua_close_t)(void *L);
#define LUA_NEWSTATE ((lua_newstate_t)  0x00674BF9)
#define LUA_OPENLIBS ((lua_openlibs_t)  0x0068194D)
#define LUA_LOADBUF  ((lua_loadbufferx_t) 0x0067428D)
#define LUA_PCALLK   ((lua_pcallk_t)    0x00672CFD)
#define LUA_CLOSE    ((lua_close_t)     0x0067C8B1)


/* ---- posix_spawn 串行执行 ---- */
typedef int (*spawn_t)(unsigned int *pid, const char *path, void *fa, void *attr, char *const *argv, char *const *envp);
typedef int (*fa_init_t)(void *fa);
typedef int (*fa_addopen_t)(void *fa, int fd, const char *path, int oflags, unsigned int mode);
typedef int (*fa_destroy_t)(void *fa);
typedef int (*attr_init_t)(void *attr);
typedef int (*attr_destroy_t)(void *attr);
typedef int (*waitpid_t)(unsigned int pid, int *stat, int opts);
#define SPAWN       ((spawn_t)       0x008CD299)
#define FA_INIT     ((fa_init_t)     0x008C877D)
#define FA_ADDOPEN  ((fa_addopen_t)  0x008C86D9)
#define FA_DESTROY  ((fa_destroy_t)  0x008C874D)
#define ATTR_INIT   ((attr_init_t)   0x006568B1)
#define ATTR_DESTROY ((attr_destroy_t) 0x0065690D)
#define WAITPID     ((waitpid_t)     0x008CD1C1)
static unsigned char g_fa[64];
static unsigned char g_attr[64];

/* ---- LVGL ---- */
typedef void *(*obj_create_t)(void *parent, int copy);
typedef void  (*obj_set_pos_t)(void *obj, int x, int y);
typedef void  (*obj_set_size_t)(void *obj, int w, int h);
typedef void  (*obj_add_flag_t)(void *obj, unsigned int flag);
typedef void  (*obj_remove_flag_t)(void *obj, unsigned int flag);
typedef void  (*obj_add_event_cb_t)(void *obj, void (*cb)(void *), unsigned int filter, void *ud);
typedef void *(*event_get_user_data_t)(void *e);
typedef void *(*label_create_t)(void *parent);
typedef void  (*label_set_text_t)(void *label, const char *text);
typedef void *(*title_create_t)(void *parent, const char *text, int a, int b, int c);
typedef void  (*lv_timer_cb_t)(void *timer);
typedef void *(*lv_timer_create_t)(void *cb, unsigned int period, void *ud);
typedef void  (*lv_timer_set_repeat_t)(void *timer, unsigned int cnt);

#define OBJ_CREATE   ((obj_create_t)  0x00916239)
#define OBJ_SET_POS  ((obj_set_pos_t) 0x00917895)
#define OBJ_SET_SIZE ((obj_set_size_t) 0x00917961)
#define OBJ_ADD_FLAG ((obj_add_flag_t) 0x0091629D)
#define OBJ_REM_FLAG ((obj_remove_flag_t) 0x00916349)
#define OBJ_ADD_CB   ((obj_add_event_cb_t) 0x00917341)
#define EVT_USERDATA ((event_get_user_data_t) 0x00930A7D)
#define LABEL_CREATE ((label_create_t) 0x009410B9)
#define LABEL_SET    ((label_set_text_t) 0x00941B1D)
#define TITLE_CREATE ((title_create_t) 0x00ED1B81)
#define TIMER_CREATE ((lv_timer_create_t) 0x00933325)
#define TIMER_REPEAT ((lv_timer_set_repeat_t) 0x0093354D)
#define FLAG_HIDDEN 1u
#define FLAG_CLICKABLE 2u
#define EVT_CLICKED 4u


/* ---- libc 文件读取（模块里 open/read 异常，fopen/fread 正常） ---- */
typedef void *(*fopen_t)(const char *, const char *);
typedef unsigned int (*fread_t)(void *, unsigned int, unsigned int, void *);
typedef int (*fclose_t)(void *);
#define FOPEN  ((fopen_t)  0x008C8B11)
#define FREAD  ((fread_t)  0x008C8D21)
#define FCLOSE ((fclose_t) 0x008C8BE5)

/* ---- 页面回调 ---- */
static void my_on_signal(void *a, void *b) { (void)a; (void)b; }

/* ---- 模块静态 ---- */
static unsigned char g_work[48];
static unsigned char g_work2[48];
static unsigned char g_table[256];
static unsigned char g_desc[128];
static unsigned char g_app_info[132];
static unsigned char g_result[2048];
__attribute__((used, section(".data")))
static volatile unsigned int shellpp_ii_data_anchor = 1;

static void *g_page1;
static void *g_page2;
static void *g_out_label;
static unsigned int g_cur_cmd;

/* ---- 图标 ---- */
typedef struct {
    unsigned int header0, wh, stride_res, data_size;
    const unsigned char *data;
} icon_dsc_t;
/* Keep the resident Supervisor under the firmware loader's practical memory
 * ceiling.  A 120x120 RGBA scratch icon alone consumed 57,600 bytes before
 * Shell++ could even register /dev/shellpp, causing modlib to reject the
 * module.  The launcher descriptor remains valid with this compact native
 * placeholder; the final artwork can be supplied from an external resource
 * after the loading path is proven on hardware. */
#define ICON_W 1
#define ICON_H 1
static unsigned char g_icon_px[ICON_W * ICON_H * 4] __attribute__((aligned(8))) = {
    0x20, 0xA8, 0xF2, 0xFF
};
static icon_dsc_t g_icon;
static void init_icon(void)
{
    g_icon.header0 = 0x00000919u;
    g_icon.wh = (ICON_W << 16) | ICON_H;
    g_icon.stride_res = ((ICON_W * 4) << 16);
    g_icon.data_size = sizeof(g_icon_px);
    g_icon.data = g_icon_px;
}

static const char *name_cb(void *app_info) { (void)app_info; return "Shell++ II"; }

static void *my_find_by_id(unsigned short app_id)
{
    if (app_id == OUR_APP_ID) return (void *)g_app_info;
    return ORIG_FIND_BY_ID(app_id);
}

static void *my_find_launch_page(unsigned short app_id)
{
    if (app_id == OUR_APP_ID) return (void *)g_desc;
    return ORIG_LAUNCH_PAGE(app_id);
}

/* ---- 命令执行（work_queue 上下文） ---- */

static void wlog68(const char *tag, unsigned int len)
{
    int fd = SYM_OPEN2("/data/h69_dbg.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd >= 0) { SYM_WRITE2(fd, tag, len); SYM_CLOSE2(fd); }
}


static void wlog3(unsigned int a, unsigned int b, unsigned int c)
{
    int fd;
    unsigned char d[40];
    unsigned int m = 0, i, v;
    fd = SYM_OPEN2("/data/h71_obj.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) return;
    v = a; d[m++] = '1'; d[m++] = '=';
    for (i = 0; i < 8; i++) { d[m++] = "0123456789abcdef"[(v >> (28 - i * 4)) & 0xf]; }
    d[m++] = ' ';
    v = b; d[m++] = '2'; d[m++] = '=';
    for (i = 0; i < 8; i++) { d[m++] = "0123456789abcdef"[(v >> (28 - i * 4)) & 0xf]; }
    d[m++] = ' ';
    v = c; d[m++] = '3'; d[m++] = '=';
    for (i = 0; i < 8; i++) { d[m++] = "0123456789abcdef"[(v >> (28 - i * 4)) & 0xf]; }
    d[m++] = ';';
    SYM_WRITE2(fd, d, m);
    SYM_CLOSE2(fd);
}

typedef void *(*ll_get_head_t)(void *ll);
typedef void *(*ll_get_next_t)(void *ll, void *node);
typedef void  (*ll_remove_t)(void *ll, void *node);
#define LL_HEAD   ((ll_get_head_t) 0x009313F5)
#define LL_NEXT   ((ll_get_next_t) 0x00931415)
#define LL_REMOVE ((ll_remove_t)   0x009312F5)

static void add_worker(void *arg);
static int shellpp_ii_uninit(void *arg);
static void publish_launcher(void);

/* XiaomiVela 3.101.036 private NuttX driver registration ABI, recovered from
 * the Canopus supervisor's sup_register_device relocation. */
typedef int (*register_driver_t)(const char *, void *, unsigned int, void *);
typedef int (*device_cb_t)(void *, ...);
typedef struct {
    void *open; void *close; void *read; void *write;
    void *pad[4]; void *ioctl; void *tail[6];
} shellpp_file_operations_t;
#define REGISTER_DRIVER ((register_driver_t)0x0C1A0D51)
#define SHELLPP_DEVICE "/dev/shellpp"
#define SHELLPP_MAGIC 0x53505331u /* SPS1 */
#define SHELLPP_CMD_RESTORE 0x53510001u
#define SHELLPP_CMD_INSTALL 0x53510002u
#define SHELLPP_CMD_UNINSTALL 0x53510003u
#define SHELLPP_RESULT_COMPLETED 5u

static volatile unsigned int shellpp_state;
static volatile unsigned int shellpp_pending_command;
static volatile unsigned int shellpp_pending_stage;
static volatile int shellpp_error;
static shellpp_file_operations_t shellpp_file_ops __attribute__((aligned(4)));
static unsigned char shellpp_status[384] __attribute__((aligned(4)));

static int shellpp_open(void *file) { (void)file; return 0; }
static int shellpp_close(void *file) { (void)file; return 0; }
static int shellpp_read(void *file, void *buffer, unsigned int count);
static int shellpp_write(void *file, const void *buffer, unsigned int count);

/* Keep this diagnostic independent of /dev/shellpp: it is the only useful
 * evidence when register_driver itself fails before the device exists. */
static void shellpp_log_register(unsigned int phase, int rc)
{
    unsigned char line[40];
    const char *digits = "0123456789abcdef";
    unsigned int value, i, n = 0;
    int fd = SYM_OPEN2("/data/shellpp-ii-supervisor.log",
        O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) return;
    line[n++] = 'p'; line[n++] = '=';
    value = phase;
    for (i = 0; i < 8; ++i) line[n++] = digits[(value >> (28 - i * 4)) & 15];
    line[n++] = ' '; line[n++] = 'r'; line[n++] = 'c'; line[n++] = '=';
    value = (unsigned int)rc;
    for (i = 0; i < 8; ++i) line[n++] = digits[(value >> (28 - i * 4)) & 15];
    line[n++] = '\n';
    SYM_WRITE2(fd, line, n);
    SYM_CLOSE2(fd);
}

static void shellpp_publish_stage(void *arg) {
    unsigned int stage = (unsigned int)(unsigned long)arg;
    if (stage == 0) {
        shellpp_state = 5; shellpp_error = 0; return;
    }
    if (stage == 1) {
        add_worker(0);
        shellpp_state = 5; shellpp_error = 0; return;
    }
    if (stage == 2) {
        publish_launcher();
        shellpp_state = 5; shellpp_error = 0; return;
    }
    shellpp_state = 15; shellpp_error = -22;
}

static int shellpp_read(void *file, void *buffer, unsigned int count) {
    unsigned int i;
    (void)file;
    if (!buffer || count < sizeof(shellpp_status)) return -22;
    for (i = 0; i < sizeof(shellpp_status); ++i) shellpp_status[i] = 0;
    *(unsigned int *)(shellpp_status + 0) = SHELLPP_MAGIC;
    *(unsigned int *)(shellpp_status + 4) = 1;
    *(unsigned int *)(shellpp_status + 20) = shellpp_pending_command;
    *(unsigned int *)(shellpp_status + 24) = shellpp_state;
    *(int *)(shellpp_status + 32) = shellpp_error;
    *(unsigned int *)(shellpp_status + 36) = 2;
    for (i = 0; i < sizeof(shellpp_status); ++i) ((unsigned char *)buffer)[i] = shellpp_status[i];
    return sizeof(shellpp_status);
}

static int shellpp_write(void *file, const void *buffer, unsigned int count) {
    const unsigned int *words = (const unsigned int *)buffer;
    (void)file;
    if (!buffer || count < 16 || words[0] != SHELLPP_MAGIC) return -22;
    shellpp_pending_command = words[1];
    shellpp_pending_stage = words[2];
    shellpp_state = 8;
    if (words[1] == SHELLPP_CMD_INSTALL) {
        SYM_WQ(LPWORK, (void *)g_work2, shellpp_publish_stage,
            (void *)(unsigned long)words[2], 1);
    } else if (words[1] == SHELLPP_CMD_RESTORE) {
        SYM_WQ(LPWORK, (void *)g_work2, shellpp_publish_stage, (void *)0, 1);
    } else if (words[1] == SHELLPP_CMD_UNINSTALL) {
        shellpp_ii_uninit(0); shellpp_state = 5; shellpp_error = 0;
    } else { shellpp_state = 15; shellpp_error = -22; }
    return 16;
}

static int shellpp_register_device(void) {
    unsigned int i;
    unsigned char *raw = (unsigned char *)&shellpp_file_ops;
    for (i = 0; i < sizeof(shellpp_file_ops); ++i) raw[i] = 0;
    shellpp_file_ops.open = (void *)shellpp_open;
    shellpp_file_ops.close = (void *)shellpp_close;
    shellpp_file_ops.read = (void *)shellpp_read;
    shellpp_file_ops.write = (void *)shellpp_write;
    shellpp_state = 5; shellpp_error = 0;
    return REGISTER_DRIVER(SHELLPP_DEVICE, &shellpp_file_ops, 0666, 0);
}

/* The watch firmware runs module constructors from .init_array.  Canopus
 * registers its supervisor device from this phase; module_initialize is only
 * the modlib bookkeeping entry. */
static void shellpp_ii_ctor(void) __attribute__((constructor));
static void shellpp_init_worker(void *arg)
{
    int rc;
    (void)arg;
    rc = shellpp_register_device();
    shellpp_log_register(2, rc);
    if (rc != 0) {
        shellpp_state = 15;
        shellpp_error = rc;
    }
}

static void shellpp_ii_ctor(void)
{
    /* Match Canopus/Term: constructors only enqueue work.  Calling firmware
     * registration synchronously from modlib init is too early on miwear. */
    SYM_WQ(LPWORK, (void *)g_work, shellpp_init_worker, (void *)0, 300);
}

static unsigned int g_pending;
static void *g_timer;

static void ui_poll_cb(void *timer)
{
    (void)timer;
    if (g_pending) {
        wlog68("U", 1);
        g_pending = 0;
        wlog3((unsigned int)g_page1, (unsigned int)g_page2, (unsigned int)g_out_label);
        if (g_out_label) {
            ((void (*)(void *, unsigned int, int))0x0091CDF9)(g_out_label, 0x0147575Cu, 0);
            LABEL_SET(g_out_label, (const char *)g_result);
        }
        {
            int fd2 = SYM_OPEN2("/data/h72_res.txt", O_WRONLY | O_CREAT | O_TRUNC);
            if (fd2 >= 0) {
                unsigned char *rp = g_result;
                unsigned int i, j;
                for (i = 0; i < 8 && rp[i]; i++) { SYM_WRITE2(fd2, rp + i, 1); }
                SYM_WRITE2(fd2, ";", 1);
                SYM_CLOSE2(fd2);
            }
        }
        if (g_page1) OBJ_ADD_FLAG(g_page1, FLAG_HIDDEN);
        if (g_page2) OBJ_REM_FLAG(g_page2, FLAG_HIDDEN);
    }
}

static void exec_worker(void *arg)
{
    (void)arg;
    const char *cmd;
    unsigned int di;
    int fd;
    unsigned int pid = 0;
    int status = 0;
    char *argv[4];

    switch (g_cur_cmd) {
    case 1: cmd = "ls -l /data"; break;
    case 2: cmd = "dmesg"; break;
    default: cmd = "uname -a"; break;
    }
    argv[0] = "nsh";
    argv[1] = "-c";
    argv[2] = (char *)cmd;
    argv[3] = 0;
    FA_INIT(g_fa);
    ATTR_INIT(g_attr);
    FA_ADDOPEN(g_fa, 0, "/dev/null", 0, 0);
    FA_ADDOPEN(g_fa, 1, "/data/term_out.txt", (1 << 1) | (1 << 2) | (1 << 5), 0x1FF);
    (void)SPAWN(&pid, "/bin/nsh", g_fa, g_attr, argv, 0);
    (void)WAITPID(pid, &status, 0);
    FA_DESTROY(g_fa);
    ATTR_DESTROY(g_attr);

    /* 读输出（fopen/fread 循环） */
    {
        void *fp = FOPEN("/data/term_out.txt", "r");
        if (fp) {
            unsigned int total = 0;
            unsigned int n;
            while (total < 2000) {
                n = FREAD(g_result + total, 1, 2000 - total, fp);
                if (n == 0) break;
                total += n;
            }
            FCLOSE(fp);
            g_result[total] = 0;
        } else {
            g_result[0] = 0;
        }
    }

    {
        int fd3 = SYM_OPEN2("/data/h74_read.txt", O_WRONLY | O_CREAT | O_TRUNC);
        if (fd3 >= 0) {
            unsigned char d[16];
            unsigned int m = 0;
            d[m++] = (unsigned char)('0' + (g_result[0] != 0));
            d[m++] = g_result[0];
            d[m++] = g_result[1];
            d[m++] = g_result[2];
            d[m++] = g_result[3];
            d[m++] = ';';
            SYM_WRITE2(fd3, d, m);
            SYM_CLOSE2(fd3);
        }
    }
    g_pending = 1;   /* 通知 UI 线程轮询更新 */
}

static void cmd_cb(void *e)
{
    g_cur_cmd = (unsigned int)(unsigned long)EVT_USERDATA(e);
    wlog68("C", 1);
    SYM_WQ(LPWORK, (void *)g_work, exec_worker, (void *)0, 10);
}

static void back_cb(void *e)
{
    (void)e;
    if (g_page1) OBJ_REM_FLAG(g_page1, FLAG_HIDDEN);
    if (g_page2) OBJ_ADD_FLAG(g_page2, FLAG_HIDDEN);
}

static void create_item(void *parent, const char *text, unsigned int cmd_id, int y)
{
    void *it = OBJ_CREATE(parent, 0);
    void *lb;
    if (!it) return;
    OBJ_SET_SIZE(it, 440, 60);
    OBJ_SET_POS(it, 13, y);
    OBJ_ADD_FLAG(it, FLAG_CLICKABLE);
    lb = LABEL_CREATE(it);
    if (lb) {
        OBJ_SET_POS(lb, 15, 18);
        LABEL_SET(lb, text);
    }
    OBJ_ADD_CB(it, cmd_cb, EVT_CLICKED, (void *)(unsigned long)cmd_id);
}

/* ---- on_create：两页 UI ---- */
static int my_on_create(void *r0, void *r1)
{
    (void)r0;
    if (r1) {
        void *back;
        TITLE_CREATE(r1, "Shell++ II", 0, 0, 0);
        if (!g_timer) g_timer = TIMER_CREATE(ui_poll_cb, 50, 0);
        g_page1 = OBJ_CREATE(r1, 0);
        if (g_page1) {
            OBJ_SET_SIZE(g_page1, 466, 400);
            OBJ_SET_POS(g_page1, 0, 58);
            create_item(g_page1, "ls -l /data", 1, 0);
            create_item(g_page1, "dmesg", 2, 70);
            create_item(g_page1, "uname -a", 3, 140);
        }
        g_page2 = OBJ_CREATE(r1, 0);
        if (g_page2) {
            OBJ_SET_SIZE(g_page2, 466, 400);
            OBJ_SET_POS(g_page2, 0, 58);
            g_out_label = LABEL_CREATE(g_page2);
            if (g_out_label) {
                OBJ_SET_POS(g_out_label, 12, 50);
                LABEL_SET(g_out_label, "");
            }
            back = OBJ_CREATE(g_page2, 0);
            if (back) {
                void *bl;
                OBJ_SET_SIZE(back, 100, 40);
                OBJ_SET_POS(back, 12, 8);
                OBJ_ADD_FLAG(back, FLAG_CLICKABLE);
                bl = LABEL_CREATE(back);
                if (bl) LABEL_SET(bl, "< Back");
                OBJ_ADD_CB(back, back_cb, EVT_CLICKED, 0);
            }
            OBJ_ADD_FLAG(g_page2, FLAG_HIDDEN);
        }
    }
    wlog3((unsigned int)g_page1, (unsigned int)g_page2, (unsigned int)g_out_label);
    return 0;
}

/* ---- 描述符插入 ---- */
static void insert_page_desc(void)
{
    unsigned int i;
    unsigned int head_node, old_first;
    for (i = 0; i < sizeof(g_desc); i++) g_desc[i] = 0;
    *(unsigned int *)(g_desc + 0x00) = 0x4021acac;
    *(unsigned int *)(g_desc + 0x10) = (unsigned int)"term";
    *(unsigned int *)(g_desc + 0x14) = OUR_PAGE_ID;
    *(unsigned int *)(g_desc + 0x18) = 0x00000006;
    *(unsigned int *)(g_desc + 0x28) = 0x02000504;
    *(unsigned int *)(g_desc + 0x2c) = 0x4021ac2c;
    *(unsigned int *)(g_desc + 0x34) = (unsigned int)my_on_signal;
    *(unsigned int *)(g_desc + 0x48) = 0x4021acac;
    *(unsigned int *)(g_desc + 0x4c) = (unsigned int)my_on_create;
    *(unsigned int *)(g_desc + 0x50) = (unsigned int)my_on_signal;
    *(unsigned int *)(g_desc + 0x5c) = (unsigned int)my_on_signal;
    *(unsigned int *)(g_desc + 0x64) = (unsigned int)my_on_signal;
    *(unsigned int *)(g_desc + 0x74) = 0x42ac0000;
    *(unsigned int *)(g_desc + 0x78) = 0x42940000;
    *(unsigned int *)(g_desc + 0x7c) = 0x42ac0000;

    head_node = *(unsigned int *)AM_HEAD_SLOT;
    old_first = *(unsigned int *)(head_node + 68);
    *(unsigned int *)(g_desc + 64) = head_node;
    *(unsigned int *)(g_desc + 68) = old_first;
    *(unsigned int *)(head_node + 68) = (unsigned int)g_desc;
    if (old_first) *(unsigned int *)(old_first + 64) = (unsigned int)g_desc;
}

static void add_worker(void *arg)
{
    (void)arg;
    unsigned int i;
    init_icon();
    for (i = 0; i < sizeof(g_app_info); i++) g_app_info[i] = 0;
    *(unsigned int *)(g_app_info + 8)    = (unsigned int)"com.shellpp.ii";
    *(unsigned int *)(g_app_info + 12)   = (unsigned int)&g_icon;
    *(unsigned short *)(g_app_info + 16) = OUR_APP_ID;
    *(unsigned int *)(g_app_info + 28)   = (unsigned int)name_cb;

    {
        volatile unsigned char *src = (volatile unsigned char *)TABLE_ADDR;
        for (i = 0; i < sizeof(g_table); i++) g_table[i] = src[i];
        *(unsigned int *)(g_table + 4)  = (unsigned int)my_find_by_id;
        *(unsigned int *)(g_table + 76) = (unsigned int)my_find_launch_page;
        *(volatile unsigned int *)TABLE_HOOK_SLOT = (unsigned int)g_table;
    }

    /* Launcher publication is deliberately deferred to INSTALL stage 2. */
    /* {
        void *node = LL_HEAD((void *)0x4020d6a0);
        unsigned int exists = 0;
        while (node) {
            unsigned int pkg = *(unsigned int *)((unsigned int)node + 8);
            unsigned int i = 0, match = 1;
            const char *k;
            if (pkg) {
                k = "com.shellpp.ii";
                while (k[i]) {
                    if (*(unsigned char *)(pkg + i) != (unsigned char)k[i]) { match = 0; break; }
                    i++;
                }
                if (match && *(unsigned char *)(pkg + i) == 0) { exists = 1; break; }
            }
            node = LL_NEXT((void *)0x4020d6a0, node);
        }
        if (!exists) (void)FN_LAUNCHER_ADD(OUR_APP_ID);
    } */
    insert_page_desc();
}

static void publish_launcher(void) {
    void *node = LL_HEAD((void *)0x4020d6a0);
    unsigned int exists = 0;
    while (node) {
        unsigned int pkg = *(unsigned int *)((unsigned int)node + 8);
        unsigned int i = 0, match = 1;
        const char *k = "com.shellpp.ii";
        if (pkg) {
            while (k[i]) {
                if (*(unsigned char *)(pkg + i) != (unsigned char)k[i]) { match = 0; break; }
                i++;
            }
            if (match && *(unsigned char *)(pkg + i) == 0) { exists = 1; break; }
        }
        node = LL_NEXT((void *)0x4020d6a0, node);
    }
    if (!exists) (void)FN_LAUNCHER_ADD(OUR_APP_ID);
}

static int shellpp_ii_uninit(void *arg)
{
    (void)arg;
    /* 0. 删除 LVGL 轮询定时器（避免悬空回调卡死） */
    if (g_timer) ((void (*)(void *))0x009333D5)(g_timer);
    /* 1. 恢复 packagemanager vtable 指针 */
    *(volatile unsigned int *)TABLE_HOOK_SLOT = TABLE_ADDR;
    /* 2. 从 activitymanager 页面链表摘除 g_desc */
    {
        unsigned int prev, next;
        if (g_desc[0] != 0 || *(unsigned int *)(g_desc + 64) != 0) {
            prev = *(unsigned int *)(g_desc + 64);
            next = *(unsigned int *)(g_desc + 68);
            if (prev) *(unsigned int *)(prev + 68) = next;
            if (next) *(unsigned int *)(next + 64) = prev;
        }
    }
    /* 3. 从 launcher 链表摘除 com.shellpp.ii 图标节点 */
    {
        void *node = LL_HEAD((void *)0x4020d6a0);
        while (node) {
            unsigned int pkg = *(unsigned int *)((unsigned int)node + 8);
            unsigned int i = 0, match = 1;
            const char *k;
            if (pkg) {
                k = "com.shellpp.ii";
                while (k[i]) {
                    if (*(unsigned char *)(pkg + i) != (unsigned char)k[i]) { match = 0; break; }
                    i++;
                }
                if (match && *(unsigned char *)(pkg + i) == 0) {
                    LL_REMOVE((void *)0x4020d6a0, node);
                    break;
                }
            }
            node = LL_NEXT((void *)0x4020d6a0, node);
        }
    }
    return 0;
}

int module_initialize(struct shellpp_ii_mod_info *modinfo)
{
    modinfo->uninitializer = shellpp_ii_uninit;
    modinfo->arg = 0; modinfo->exports = 0; modinfo->nexports = 0;
    return 0;
}
