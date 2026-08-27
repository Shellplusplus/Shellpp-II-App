# 故障排查

本页按构建、打包、安装、Supervisor、native App、UI、文件系统和设备日志分层诊断。3.101.036 与 3.101.043 当前均已确认可用；本页记录的崩溃形状是历史案例和未来回归的识别方法，不表示当前冻结产物仍存在同一问题。

## 1. 总体诊断顺序

出现问题时按以下顺序收集证据，不跨层猜测：

1. 记录设备型号、`ro.build.version`、操作时间、是否刚重启。
2. 记录安装器 `resource.bin`、目标 bin 的大小和 SHA-256。
3. 检查三个仓库路径、工作树和本次实际构建命令。
4. 确认 profile、固件镜像大小和 SHA-256。
5. 保存完整构建输出，确认所有所选 target 与 ELF gate 通过。
6. 比较 out、editing tree、packaged tree 和反向解包 payload。
7. 在设备上读取 Supervisor magic、ABI、firmware code 和状态 words。
8. 确认设备没有驻留旧 module；页面或 callback 改变后必须先重启。
9. 将故障缩小到一个 Run stage、一个页面或一个操作。
10. 立即采集新日志，用精确时间和重启边界排除历史记录。

不要从“系统重启”直接跳到某个 ABI 地址，也不要从“构建成功”推断设备加载了本次产物。

## 2. Profile 被拒绝

| 现象 | 常见原因 | 处理 |
| --- | --- | --- |
| `missing keys` | 完整 ABI 未填完 | 返回 ABI 恢复，不使用占位值 |
| `unknown keys` | 拼写错误或 schema 未扩展 | 修正拼写；新字段同步生成器、所有 profile、消费者和文档 |
| `invalid profile assignment` | 空白、引号或 shell 元字符 | 使用纯 `KEY=VALUE`，不要变量展开 |
| firmware code mismatch | 版本公式不一致 | 按 `major*1000000+minor*1000+patch` 重算 |
| nonzero Thumb address | 函数值为 0 或偶数 | 重新证明运行时函数入口和 Thumb 位 |
| style alignment | 数据地址未 4 B 对齐 | 重新确认数据对象，不能机械修整 |
| image missing | 路径大小写或资料移动 | 找回精确镜像并检查绝对路径 |

生成器通过只证明 schema 形式成立。地址身份、原型、descriptor/dirent、线程和时序错误仍可能在真机崩溃。

## 3. 固件镜像不匹配

`size mismatch` 或 `SHA-256 mismatch` 说明 profile 与当前文件不是同一分析输入。不要把实际哈希直接抄进 profile让构建继续。依次检查：

- 固件版本、地区或构建 variant；
- 解包是否截断、补齐、解密或修改镜像；
- ABI 分析使用的究竟是哪一个文件；
- 路径是否因大小写指向另一个目录；
- 同名镜像是否有多个不同哈希。

只有重新建立“镜像字节与 ABI 证据”的一一对应关系后才能更新 profile。

## 4. Patch 应用失败

Target patch 只应用于 `out/<target>/target-src`。失败通常表示共享 canonical 源改变，导致 patch context 漂移。

处理顺序：

1. 读取共享源码和失败 patch，不修改临时 `target-src`。
2. 确认共享变化属于用户还是本任务，不能回退未知修改。
3. 在共享源码当前版本上重新表达同一个目标专用语义。
4. 保持 patch 文件顺序和最小影响范围。
5. 重放该 target 全部 patch 和 host tests。
6. 执行全目标构建，确认 036 没有被 043 修复改变。

不要把 patch 内容直接合并到共享源码来绕过冲突，也不要为 036 新建补偿 patch。

## 5. 编译、链接或 ELF Gate 失败

| 信息 | 优先检查 |
| --- | --- |
| `shellpp_target_abi.h` 缺失 | 是否从构建器运行，而非直接在 nativeApp 调 clang |
| warning treated as error | 修正类型、原型或控制流；不要移除 `-Werror` |
| undefined imports | 是否引入 libc、builtin、unwind 或 loader 不提供的 symbol |
| `module_initialize` 缺失/重复 | `module_prelude.S`、`supervisor.c` 和链接输入 |
| unexpected allocated section | 编译选项、静态对象 attribute 或 linker script |
| SHT_RELA/unsupported relocation | 工具链输出与目标 loader 支持证据 |
| loaded footprint 超限 | 所有 SHF_ALLOC section 和 alignment，要求严格小于上限 |
| BSS 超限 | 全局数组、descriptor、scratch 和 target patch 增量 |
| analysis-only address | 源/profile 中仍有 `0x2c...` 映射 |
| address not Thumb | 函数地址为偶数或误把 data 当函数 |
| address not in whitelist | 硬编码地址或 schema 未表达的真实 ABI |

当前 036 BSS 余量仅 84 B，043 余量 704 B。单看某个数组大小不足以判断安全，要比较两个 target 的最终 ELF。

验证器只扫描直接出现的 literal。`direct target ABI literals` 数量不是 ABI 调用总数，也不能用“计数没变”证明无新硬编码。

## 6. 构建成功但设备收到旧内容

典型症状：本地 out 的功能已修复，但设备仍显示旧页面、旧标题或旧错误。

逐项确认：

- 本次命令退出码是否为 0，而不是只看到旧文件存在；
- 无参数构建是否完成所有 profile；
- out 与 `_Lua` 中目标 bin 是否逐字节相同；
- out 与 `resources/_lua/_Lua` 中目标 bin 是否相同；
- 两个资源目录的 `main.lua` 和图标是否相同；
- `resource.bin` 是否在本次构建中重建；
- 反向解析的 payload 是否与 packaged tree 相同；
- `hashCode` 是否对应最终 `resource.bin`；
- 设备是否在安装后重启并清除了旧 module。

`resources/_lua/_Lua` 是打包输入。只复制顶层 `_Lua` 不会证明最终安装包更新。覆盖磁盘 bin 也不会替换 RAM 中已驻留的 module。

## 7. 安装器提示固件不支持

检查：

1. `getprop ro.build.version` 是否严格返回三段十进制版本。
2. 是否存在 `SCRIPT_PATH/shellpp_ii-<exact-version>.bin`。
3. 文件大小是否在 512..262144 B。
4. 前 20 B 是否为 ELF32 little-endian、ET_REL、EM_ARM。
5. 设备包中的资源是否真的包含该文件。

Lua 使用精确匹配，不把 3.101.043 fallback 到 3.101.036。为通过检测而放宽版本匹配会把 ABI 错误推迟到系统崩溃。

## 8. `insmod` 或 LOAD Failed

| 现象 | 检查 |
| --- | --- |
| shell `insmod` 失败 | ELF loader 日志、module 名、文件可读性、section/relocation |
| `/dev/shellpp` 未出现 | constructor 是否执行、`register_driver` 地址/原型、设备名冲突 |
| status magic 不匹配 | 错误 driver、短读或非 Shell++ module |
| status ABI 不是 3 | 旧 Supervisor 驻留，必须重启 |
| word 12 不匹配 | 错误固件 bin或旧 target module 驻留 |
| word 13 为 0 | 保存首次 word 9，检查 driver 注册原始返回 |

不要重复 `insmod`，也不要在已有 `/dev/shellpp` 时尝试 live 替换。Lua 对同一启动周期只允许一次 Run，但人工 shell 操作仍可能绕过这层保护，不应这样测试。

## 9. Run 在某个 Stage 失败

Lua 的实际顺序是 notification、restore、install 0、install 1、install 2。保存 words 6、7、9、12 至 20。

| Stage | 主要依赖 | 典型排查 |
| --- | --- | --- |
| notification | 提交入口、0x58 record、class word、同步消费 | word 20、图标 staging、记录布局 |
| restore | Supervisor command parser | command magic、arg0、status state/error |
| install 0 | 兼容序列 no-op | 若失败多为协议或旧 module |
| install 1 | App lookup/install、descriptor、worker | App ID/package、offset、静态存储、有界 lookup |
| install 2 | App lookup、Launcher add | word 15、包名匹配、word 19、App ID |

Device write 返回 16 只表示 payload 被消费；命令失败通过 word 7/9 返回。word 18 至 20 是固件不透明诊断，不能套用项目 errno 表。

## 10. 通知出现但 Launcher 或 App 不正确

通知、App 注册与 Launcher 发布是三个独立状态：

- 通知出现不证明 App/Page descriptor 正确；
- Launcher 图标出现不证明点击后的 create callback 正确；
- App 首页打开不证明文件、应用、缓存、timer 和 top layer ABI 正确。

检查 words 15 至 20、App ID `0x00cd`、包名 `com.shellpp.ii`、图标 staging 内容和页面 descriptor 集合。`APP_INSTALL` 通过固件 worker 异步提交，立即一次 lookup 可能得到旧表；当前实现使用有界延迟重试，不能删除这个时序保护。

## 11. 点击 App 图标后系统重启

这类故障说明注册链至少部分执行，但页面 create 路径或其第一个固件调用不成立。按调用发生顺序检查：

- App/Page descriptor size 与内部 offset；
- descriptor、页面名、package、icon 和 callback 是否静态存活；
- page key 与注册页数；
- create 的 page/root/start-data 原型；
- content、title、label、style 和 list-row ABI；
- Activity event 枚举；
- create 时同步读取的文件或监控 ABI；
- 调用链栈和固件线程。

历史 043 故障中，旧 MiSans style 调用不适用于该固件；当前只在 043 target patch 中禁用。该案例说明固件专用行为必须隔离，不能用于删除 036 所需调用，也不能从“两个地址相近”推导兼容。

## 12. 主页可开，但功能入口重启系统

将入口拆分，不作为同一个“UI 崩溃”处理：

| 入口 | 第一组 ABI/资源 |
| --- | --- |
| 文件管理 | open/read/close、opendir/readdir/closedir、dirent、分页栈 |
| 应用管理 | 文件 ABI、registry 路径、流式 JSON、页面导航、rename |
| 缓存状态 | 目录遍历、类型、深度、链接和统计工作区 |
| 缓存实际清理 | unlink/rmdir、写操作顺序和恢复边界 |
| CPU 手动刷新 | proc/stat 路径、read/lseek、解析容量 |
| 内存手动刷新 | 内存统计路径、标准/NuttX 字段解析、buffer/栈 |
| 定时检测 | 手动刷新通过后，再查 timer create/delete 和 destroy 时序 |
| 悬浮显示 | timer 通过后，再查 top layer、label、size、align、hidden |

043 历史修复包括独立目录入口、降低栈帧、固定深度遍历、标准内存字段优先，以及按真实 XIP 映射恢复 timer/top-layer 入口。当前产物已确认可用；若未来同样症状回归，先核对实际加载哈希和旧驻留状态，再逐组验证，不直接复制历史地址。

## 13. 文件列表错误、为空或崩溃

检查：

- `opendir/readdir/closedir` 是否各自为独立正确入口；
- dirent 类型是否在 byte 0、名称是否从 `raw+1` 开始；
- `DT_DIR/REG/LNK` 值与目标固件一致；
- 名称是否 NUL 终止并小于 72 B；
- `.`、`..` 是否跳过；
- cursor 比较和分页前后项；
- 路径是否绝对且不含空组件或父目录穿越；
- 页面 workspace 是否被另一个模式或 timer 重入覆盖。

目录 API 地址不能用旧固件地址加固定差值恢复。错误 dirent 布局经常表现为随机文件名、分页异常或系统重启。

## 14. 应用管理页面或注册表异常

043 正确行为是从 page 1 导航到独立 page 8，标题为“应用管理”，并直接打开：

`/data/apps.json`

`/data/apps.json_hide`

症状与处理：

| 症状 | 优先判断 |
| --- | --- |
| 仍在 page 1 原位置切换，标题未变 | RAM 中仍驻留旧八页 module，重启后重装 |
| 页面独立但显示读取失败 | 核对加载的 bin、open/read/close ABI 和日志时间 |
| 文件缺失/空/无效却报失败 | 当前 043 patch 未生效，或设备运行旧 bin |
| 列表为空但无错误 | 043 会把无效/缺失 registry 归一化为空；检查真实文件 |
| 能读不能 hide/show | rename/write/临时文件、目标存在策略、JSON 截断 |
| 卸载后数据残留 | registry 操作与五个数据 root 删除是独立阶段 |

不要回退到 `/data/quickapp/apps.json`，也不要把 `/data/quickapp/` 当成当前 043 注册表文件。036 page 1 内部模式和较严格读取语义是其共享基线，不应因 043 修复而改变。

## 15. CPU/内存数值不正确

先区分“读取失败”和“解析成功但值错误”：

- 保存设备原始 proc 文本；
- 检查读取上限、NUL 终止、seek 与 close；
- CPU 需要比较累计 tick，不把单次字段直接当百分比；
- 标准 `MemTotal/MemAvailable` 成功后不得再用通用前三数字覆盖；
- NuttX `Umem` allocator 格式仍应优先使用自身字段；
- 计算需防除零、溢出和超过 100%。

Host tests 能发现 parser 回归，但不能证明设备路径、格式或固件文件 ABI。

## 16. 缓存清理统计不一致

检查五个候选 root、`include_logs` 选择、before/after/freed 的计算和每 root 的 deleted/failed/skipped。链接与未知类型应 skip，不应跟随。失败后不得只显示 freed 数而丢弃删除失败计数。

历史递归实现可能累积设备栈；043 使用最多 13 个显式目录帧。超过深度必须失败，不能静默漏扫后显示成功。

## 17. Hide/Show/Delete 部分完成

两个 registry 的 rename 不是跨文件事务。发生错误后：

1. 立即停止重复操作。
2. 保存当前 `apps.json`、`apps.json_hide` 和 `.shellpp.tmp`。
3. 验证 JSON 结构和 `InstalledApps`。
4. 按 package 比较重复、缺失与 `hideFlag`。
5. 使用同一设备同一固件的测试前备份恢复。
6. 重启让固件重新加载注册表。

Hide/show 采用 target-first，第二步失败通常产生重复而不是丢失。Delete 还可能已删除数据目录，注册表备份无法恢复这些数据。

## 18. 无法卸载或返回 -95/-16

这是设计保护，不是瞬时故障：

- `-95`：native App 已注册，没有已证明的反注册 ABI；
- `-16`：module uninitializer 阻止卸载，防止固件 callback 指向已释放 text。

正确路径是删除 `/data/shellpp-ii` 后重启。不得强制 unload、伪造成功或绕过 `shellpp_native_can_unload()`。

## 19. 日志判断

AstroBox 日志包可能同时包含：本次实时输出、日志采集触发的全线程 dump、历史 DFX event 和重启后的启动日志。归因前必须匹配：

- 固件版本；
- bin/resource 哈希；
- 操作的精确本地时间；
- 崩溃前最后一步；
- 重启边界；
- fault 文件自身时间。

`sched_dumpstack` 不一定是 crash；历史 PC/LR 不属于新产物。若没有与本次操作匹配的 fault，结论应写“日志证据不足”，而不是选择旧事件补全原因。

## 20. 新故障最小记录

```text
设备型号/固件:
本次 bin SHA-256:
resource.bin SHA-256:
设备是否刚重启:
操作开始与故障时间:
最后一个成功 stage/page/action:
Supervisor words 1,2,6,7,9,12-20:
日志包路径/SHA-256:
是否存在历史事件:
恢复动作和结果:
```

一次只扩大一个状态。发生系统重启、注册表损坏或恢复状态不明确后，先恢复干净状态，再测试下一路径。
