# UI 与文件系统实现

## 1. 范围

本篇说明 `native_ui.c` 与 `native_fs.c` 的当前实现，包括页面生命周期、事件安全、文件浏览、文本/Hex 查看、复制/移动/删除、CPU/内存监控、缓存清理和重启。应用注册表的 JSON 语义和应用数据删除单独见[应用管理](application-management.md)。

本文描述实际参与构建的代码。界面文案中出现“编辑”不表示当前存在通用文本编辑和保存操作；当前文件浏览器只提供查看、复制、移动和删除。`SHELLPP_FS_EDIT_LIMIT` 及命名为 editor 的共享 workspace 目前主要用于内存复用和应用管理，不应据此宣称文件编辑功能已实现。

## 2. 固件 ABI 边界

文件系统不链接 libc/POSIX import，而是通过 profile 注入的固件函数地址调用：

- `open`、`read`、`write`、`close`、`lseek`；
- `unlink`、`rename`、`rmdir`；
- `opendir`、`readdir`、`closedir`。

以下看似标准的值也来自 profile，不能使用宿主系统常量替代：

- `O_RDONLY`、`O_WRONLY`、`O_CREAT`、`O_TRUNC`；
- `SEEK_SET`、`SEEK_END`；
- `DT_DIR`、`DT_REG`、`DT_LNK`。

当前代码把 `readdir()` 返回记录的第 1 字节解释为类型、从 `raw + 1` 解释零结尾名称。这个 dirent 布局属于固件 ABI。新固件即使函数地址正确，只要记录布局不同，目录浏览和所有递归操作仍可能崩溃。

UI 通过 profile 注入 LVX/LVGL object、label、list row、event、timer、Activity navigate/finish 和 restart API。函数地址、原型、枚举、调用线程和对象所有权都必须在新固件完整 ABI 中确认。

## 3. 页面和状态模型

### 3.1 共享页面

共享 036 基线包含八页：

| index | 页面标题 | 实现职责 |
| ---: | --- | --- |
| 0 | `Shell++ II` | 首页和三个功能组入口 |
| 1 | `文件与应用管理` | 文件、应用、缓存入口；036 应用列表复用本页 |
| 2 | `文件查看` | 文件浏览器的 list/detail/text/hex 状态机 |
| 3 | `缓存清理` | 缓存统计、日志选项、二次确认清理 |
| 4 | `关于 Shell++ II` | 版本、包名、固件版本和开发信息 |
| 5 | `显示` | CPU 与内存监控入口 |
| 6 | `占用显示` | 复用为 CPU 或内存页面 |
| 7 | `重启` | 当前界面提供二次确认的系统重载 |

043 target 增加 page 8 `应用管理`，应用列表不再替换 page 1 内容。该行为只来自 `0006-separate-app-manager-page.patch`。

### 3.2 `ui_page` 状态

每页记录：

- root、content、title、descriptor 和可选 label；
- 最多 32 个 list row pointer；
- generation；
- active 和 interactive 标志；
- 036 共享实现还在每页保存 32 个 binding；
- 043 为满足 BSS 上限，只保存当前前台页的一组全局 binding，并同时记录其 page 和 generation。

043 的 binding 复用依赖“同一时刻只有前台页可交互”。如果未来固件允许多页同时接收事件或存在异步 row callback，这一假设必须重新验证。

## 4. 页面生命周期

### 4.1 reset

`shellpp_ui_reset()` 在 App descriptor 初始化前执行：

1. 删除 CPU/内存 timer 并隐藏 top-layer overlay；
2. 清理应用列表状态；
3. 恢复为显示文本而临时插入的字符串终止字节；
4. 清零页面、目录、游标、缓存报告、共享 workspace、路径和状态；
5. 重置浏览器、监控、clipboard、确认开关和 busy 状态。

overlay 属于 display top layer，不属于页面 root。仅清零页面结构不会删除 overlay 或 timer，因此 reset 必须显式停止它们。

### 4.2 create

`shellpp_ui_page_create(index, descriptor, root)`：

- 校验 index、descriptor 和 root；
- 将旧 generation 加一，跳过 0；
- 清零该页并保存 root/descriptor；
- 创建 336 × 424 content，在顶部偏移 56；
- 首页创建无返回按钮标题，其余页面创建带 back callback 的标题；
- 生成包含 generation/page/slot 的 event cookie；
- 根据页面初始化 browser、cache、memory sample 或应用列表；
- 首次 render。

任一 content/title 创建失败返回 `-1`。不能在创建失败后继续假定 page state 完整。

### 4.3 resume、pause、destroy

- resume 要求 active 且 descriptor pointer 精确匹配，设置 interactive，刷新 cache 或 memory 并重新 render；
- pause 只将 interactive 清零，阻止旧事件操作后台页；
- destroy 清理该页状态，但保留 generation 值，使已经排队的旧 callback 失效；
- 如果被销毁页拥有 browser 或应用 workspace，相应临时状态会被归还或清理。

不要把 pause 当作 destroy，也不要在 destroy 后继续使用 page root、row 或 binding pointer。

## 5. 事件安全

row event cookie 编码：

```text
bits 31..16: generation
bits 15..8 : page index
bits 7..0  : row slot
```

事件只在以下条件全部成立时执行：

- event 非空且是 profile 指定的 clicked event；
- 全局 `g_busy` 为 0；
- page 和 slot 在范围内；
- page active、interactive；
- page generation 与 cookie 相同；
- 043 还要求全局 binding owner 的 page/generation 相同；
- binding enabled 且 action 不是 `ACTION_NONE`。

执行期间设置 `g_busy`，避免同一事件循环中的嵌套点击重入共享 workspace。generation 校验是防止页面销毁后旧 event 操作新页面状态的关键机制，不得为简化代码删除。

## 6. 固定容量和内存复用

公共限制由 `shellpp_native_fs.h` 定义：

| 限制 | 当前值 | 作用 |
| --- | ---: | --- |
| 路径容量 | 384 字节 | 包含终止符的绝对路径缓冲区 |
| 名称容量 | 72 字节 | 单个目录项名称 |
| 目录页 | 30 项 | 另保留第 31 项判断 has-next |
| 文本读取块 | 4096 字节 | 一次文件文本读取 |
| workspace | 11900 + 1 字节 | 文件查看与应用管理互斥复用 |
| Hex 原始页 | 2048 字节 | 一次读取的原始字节 |
| 可查看文件上限 | 3 MiB | 大于此值不进入 text/hex viewer |
| copy chunk | 4096 字节 | 文件复制 scratch 最小值 |
| 遍历深度 | 12 | 删除、大小统计和 cache walk 上限 |

目录页结构与最多 256 个应用 metadata 复用同一个 union；文件 workspace 与应用 JSON workspace 复用另一个 union。应用管理和文件浏览不能并发占用这些存储。

043 补丁进一步复用 filesystem candidate、512 字节读取缓冲、路径缓冲、JSON key/name/package scratch 和 row specification storage，并用 `_Static_assert` 检查容量与对齐。此设计以 LVGL 事件串行且无异步重入为前提。

## 7. 路径规则

`shellpp_fs_validate_path()` 只接受：

- 以 `/` 开头的绝对路径；
- `/` 本身；
- 不含空组件、尾随 `/`、`.` 或 `..` 组件；
- 在 384 字节上限内有终止符的路径。

`shellpp_fs_join()` 还要求名称：

- 非空且短于 72 字节；
- 不是 `.` 或 `..`；
- 不包含 `/`。

路径校验是组件级边界，不是权限或受保护目录策略。一个通过语法校验的路径仍可能指向系统关键文件；破坏性功能必须另有明确目标约束和人工测试范围。

## 8. 目录分页

目录列表按“目录优先，然后名称字节序”排序。算法在遍历目录时只保留当前 cursor 之后最小的 31 个候选：

- 返回前 30 项；
- 第 31 项只用于设置 `has_next`；
- first/last cursor 保存 `is_dir + name`；
- previous page 通过重新扫描并计算前一个 cursor，不依赖随机访问目录流。

`.` 和 `..` 被跳过。名称为空或超过容量也被跳过。普通文件大小通过 open/lseek 获取；目录和链接不通过打开目标获取内容。

`readdir` 类型值或名称偏移错误会同时破坏排序、链接识别、递归和删除，因此这是新固件最重要的真机只读 gate 之一。

## 9. 文件浏览器状态机

浏览器有四种状态：

```text
LIST -> DETAIL -> TEXT
               -> HEX
```

首次进入 page 2 从 `/` 加载第一页。

### 9.1 LIST

显示当前路径、状态、可选 clipboard 粘贴、父目录、最多 30 个条目以及前后页。选择目录进入该目录；选择非目录进入 DETAIL。

### 9.2 DETAIL

显示文件名、大小和操作：

- 文本查看；
- Hex 查看；
- 复制；
- 移动；
- 二次确认删除。

只有已知大小、不超过 3 MiB、且不是符号链接的对象可查看。只有普通文件可复制和移动；链接只允许删除链接本身。

### 9.3 TEXT

每次从文件读取最多 4096 字节。除 tab、LF、CR 外的 ASCII 控制字节替换为 `.`。屏幕按 384 字节 slice 分页，并在 UTF-8 continuation byte 前回退分页边界，减少从多字节字符中间切断的情况。

文本查看不修改文件。当前没有编辑、保存或将显示 buffer 写回原文件的 action。

### 9.4 HEX

每次读取最多 2048 字节，屏幕每行显示 8 字节，每屏 10 行，带 8 位十六进制文件偏移。页内翻动后再加载下一原始块。

Hex 查看同样只读。

## 10. 复制、移动和删除

### 10.1 复制

`shellpp_fs_copy()`：

1. 校验源/目标绝对路径和 scratch 至少 4096 字节；
2. 拒绝相同路径；
3. 要求源存在且是普通文件；
4. 要求目标不存在；
5. 读取预期源大小；
6. 写入 `<target>.shellpp.tmp`；
7. 循环处理 partial read/write；
8. 关闭两端并要求 copied size 等于 expected；
9. rename 临时文件到目标；
10. 任一失败删除临时文件。

这避免在复制中途直接创建半成品目标，但没有 `fsync` 或断电持久性保证。固定临时文件名意味着相同目标不能并发复制。

### 10.2 移动

移动先尝试原子 `rename(source, target)`。失败时执行 copy，再 unlink 源。若 copy 成功但 unlink 失败，目标和源会同时存在，并返回 delete error；调用者不能将这种情况报告成完整移动成功。

### 10.3 删除

文件浏览器只调用 `shellpp_fs_delete_file()`：

- 普通文件使用 unlink；
- 符号链接使用 unlink，但不跟随；
- 目录和未知类型被拒绝；
- UI 要求连续两次点击确认。

通用目录递归删除不暴露为文件浏览器 action，只用于应用数据等受限路径。

## 11. 原子 writer

文件系统提供流式原子 writer：

- begin 打开 `<path>.shellpp.tmp`；
- write 处理 partial write；
- commit 关闭后 rename 到目标；
- abort 关闭并 unlink 临时文件。

`g_work_path` 会被其他文件操作复用，所以 commit/abort 在最后一步重新计算临时路径。当前主要消费者是应用注册表改写，不是通用文本编辑。

同样需要注意：rename 原子性依赖目标文件系统语义，代码没有目录 `fsync`，不能对突然断电给出超出固件保证的承诺。

## 12. CPU 监控

CPU 数据来自 `/proc/cpuload`：

- 最多读取 95 字节并额外 probe 是否截断；
- 解析前两个数字；
- 只有一个值时用第一个整数值；
- 有两个值且第一个非零时计算 `second * 100 / first`；
- 结果限制到 0..100；
- 输出 `CPU:<n>%`。

页面支持手动刷新、500 ms timer 监控和 top-layer 悬浮 label。读取失败时返回明确的 open/read/close/truncated 错误；空或不可解析内容显示 `ERR:empty` 或 `NODATA`，不能当作 0% 的证明。

## 13. 内存监控

内存数据来自 `/proc/meminfo`。共享实现的解析优先级较复杂：

1. 每次调用先流式扫描完整输入中的 `Umem` 行，读取该行前三个数字作为 total/used/free；
2. 未得到有效 Umem 时读取最多 511 字节；
3. 尝试标准 `MemTotal`/`Total`、`MemFree`/`Free`、`MemAvailable`、`Buffers`、`Cached`；
4. 尝试 Umem 或 NuttX 三数字形式；
5. 按输入是否包含 `KB` 决定是否从 byte 转为 KiB；
6. 输出 `<used>/<total> <percent>%`。

043 的 `0005-prefer-standard-memory-fields.patch` 防止通用三数字 fallback 在标准字段已经存在时覆盖标准解析结果。不得把这一补丁合并进 036 后仍声称 036 二进制未变。

CPU 和内存共用一个 500 ms timer，并分别维护 page/overlay enable 状态。只有 CPU 与内存监控都不需要 timer 时才删除它。overlay 必须在 reset 时显式隐藏。

## 14. 缓存统计与清理

默认 roots：

| index | path | 是否默认包含 |
| ---: | --- | --- |
| 0 | `/data/shellpp-ii/cache` | 是 |
| 1 | `/data/shellpp-ii/tmp` | 是 |
| 2 | `/data/log` | 是 |
| 3 | `/data/offlinelog` | 是 |
| 4 | `/data/shellpp-ii/logs` | 用户开启后 |

统计和清理只遍历所列 roots，保留 root 本身。Shell++ II 图标 `/data/shellpp-ii/shellpp_ii_icon.bin` 被显式跳过。普通文件可统计/删除；目录递归；链接、设备和未知类型不跟随，计入 skipped。

UI 清理需要二次确认。报告包含每 root 的 exists、bytes、deleted、failed、skipped，以及 before/after/freed。数值累加饱和到 `UINT32_MAX`，因此接近 4 GiB 时不是精确 64 位统计。

043 用固定深度显式目录帧替代递归，并在清理后用轻量 recount 计算 remaining bytes，避免在较小有效栈和 BSS 上同时保存两个完整 report。

系统日志 roots 默认参与清理，这是破坏性行为。真机验证必须使用明确授权和可恢复场景，不能因为 cache 页面能打开就直接执行清理。

## 15. 重启

UI 中存在两条底层路径：

- soft：调用 profile 中确认的 firmware soft restart 函数，然后永久 `wfi`；
- hard：构造固定大小 file-actions/spawn-attr buffer，spawn `/bin/nsh -c reboot`，waitpid，销毁属性后永久 `wfi`。

当前共享 `重启` 页面渲染只提供“重载系统”，对应 soft 路径；hard action 代码仍存在，但没有由当前 `render_restart()` 生成可点击 row。文档和测试不能把不可达 action 描述为现行 UI 功能。

应用管理另有二次确认的 soft restart 入口，用于注册表修改后生效。

## 16. 043 栈和 BSS 适配

043 的崩溃曾集中在文件、应用、缓存和监控等交互路径。当前专用补丁通过以下方式隔离修复：

- 目录页直接写入 caller-owned output，避免大结构栈副本；
- 512/96 字节 parser buffer 迁移到共享静态 scratch；
- 应用路径、JSON key/name/package 和 row spec 复用已有 workspace；
- 目录大小、缓存和删除遍历使用最多 13 帧的显式 cursor stack；
- 只保留当前前台页 binding，释放第九页所需 BSS；
- `_Static_assert` 固定所有复用容量和对齐。

这些补丁降低编译器可见栈帧，但仍不能测量固件 ABI 函数内部栈、interrupt 余量或所有异步重入。最终证据仍是真机页面和操作测试。

## 17. 新固件验证顺序

文件/UI ABI 应按风险逐步验证：

1. 打开首页和普通静态页面；
2. create/resume/pause/destroy 循环，无旧 callback；
3. 只读打开 `/`，确认 dirent 类型、名称、排序和分页；
4. 选择已知普通文件，只查看大小；
5. 在小型测试文件上验证 text/hex；
6. CPU/内存单次采样；
7. timer 和 overlay 开关，离页/销毁后无残留；
8. cache 只读统计；
9. 在专用测试目录验证 copy/move/delete；
10. 最后才在授权环境验证 cache clear、应用注册表写入和重启。

任一步崩溃时停止扩大范围。不要用后续写操作“测试是否只是显示错误”。

## 18. 常见陷阱

- 使用宿主 POSIX flag 或 dirent 定义替代 profile ABI；
- 将 `raw + 1` dirent 名称布局无条件迁移到新固件；
- 把文件大小未知当成 0；
- 跟随符号链接进行查看或递归；
- 允许目标文件已存在时覆盖；
- 忽略 partial write、close 或 rename 失败；
- 把 copy 成功、source unlink 失败描述为 move 成功；
- 误称通用文本编辑已实现；
- 忽略 top-layer overlay 在 page destroy 之外的生命周期；
- 删除 generation 校验或 `g_busy`；
- 将静态 scratch 引入可能重入的 timer/事件路径；
- 只看 `.bss` 总量，不分析完整 callback 调用链的栈；
- 用 043 栈补丁修改 036 共享源码；
- 将 cache status 成功推断为 cache clear 安全。
