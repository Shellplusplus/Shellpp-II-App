# 已知陷阱与防错清单

本页集中记录跨仓库、固件 ABI、构建、Lua、module 生命周期、UI、文件系统和验证中已知的高风险误区。每项都给出错误后果和正确处理方式。

## 1. 路径与仓库

| 陷阱 | 后果 | 防错规则 |
| --- | --- | --- |
| 混用 `Shellpp-ii`、`shellpp-ii` 或其他大小写 | 在大小写敏感文件系统上读写另一个路径 | 使用当前构建器配置中的绝对路径并逐字符核对 |
| 把 nativeApp、构建器、安装器当成一个 Git 仓库 | 漏查工作树或错误回退用户修改 | 三个仓库分别执行状态和 diff 检查 |
| 清理整个 `out`、安装器资源或工作树 | 删除用户产物或无关资源 | 只处理构建器声明的 target stage 和 Shell++ 资源 |
| 把 `module.c`、`app_payload.c` 当作当前实现 | 阅读或修改不参与编译的 legacy 代码 | 以 `build.sh` 的五个输入为准 |
| 修改生成的 `target-src` 或 ABI 头 | 下次构建丢失且来源不可追踪 | 修改 canonical 源、profile 或 target patch |

## 2. 新固件 ABI

- 同机型新系统也必须具备完整 ABI；固件镜像、几个地址或一份符号列表都不等于完整 ABI。
- 版本间地址不是固定平移。不要把旧地址加差值，也不要使用搜索得到的第一个相似函数。
- 函数地址是设备运行时地址并包含 Thumb 位；数据地址不加 Thumb 位并遵守对齐。
- 逆向工具中的 `0x2c...` analysis mapping 不能进入 module。当前两个固定镜像的 AP XIP 映射为 `runtime = 0x0c0c0000 + file_offset`；新镜像仍需独立校准。
- 能够从某地址反汇编出合法指令不证明它是函数入口；代码中段也可能看似合法。
- 函数身份之外还要恢复原型、参数所有权、结构布局、同步/异步语义、线程和生命周期。
- `dirent` 类型值相同不代表结构布局相同；错误布局会把名称、类型或边界读错并导致系统崩溃。
- descriptor size 相同不证明内部 offset 相同。App/Page callback offset 必须逐固件有证据。
- profile 生成通过只证明字段形式有效，不证明地址或语义正确。

## 3. 共享源码与 target patch

- 036 是已验证共享基线。043 修复不得进入 036 路径。
- 仅地址、常量、枚举和尺寸差异进入 profile；无法由数据表达的固件语义差异才进入 target patch。
- 不为每个固件复制一套永久 nativeApp 源码；这会导致功能与修复漂移。
- patch 必须应用到临时 `out/<target>/target-src`，不能修改 canonical 源后再“恢复”。
- patch 顺序是行为的一部分。重命名或重排必须重新验证全部后续 patch。
- 单目标构建可以隐藏共享回归；共享源码、生成器、验证器、链接器、部署或 Lua 变化后必须全目标构建。
- 新固件若命名和协议不变，正常只新增 profile 及必要 patch；不要为每个版本在 Lua 中维护地址表。

## 4. ELF 与资源上限

- 输出是 ARM ELF32 `ET_REL` module，不是裸机器码，也不是可执行 `ET_EXEC`。`.bin` 后缀不改变 ELF 类型。
- `e_entry` 必须为 0；loader 使用导出的 `module_initialize`。
- undefined symbol、额外 allocated section、RELA 或未经证明的 relocation 不能通过“放宽 verifier”解决。
- SHF_ALLOC footprint 必须严格小于 `MAX_LOADED_SIZE`；等于上限也失败。
- `.bss` 可以等于 `MAX_BSS_SIZE`，超过才失败。
- 当前 036 `.bss` 只剩 84 B 余量，043 只剩 704 B。新增全局数组前必须计算所有 target。
- 降低某个函数栈帧不能证明整条调用链安全；需要考虑嵌套、timer callback、固件 worker 和设备线程栈。
- 验证器只扫描直接出现的 literal。Clang 可用 literal base 加立即数折叠邻近地址，因此计数不等于 ABI 使用数。
- 静态 ELF 通过不证明函数地址、原型、结构布局或真机行为。

## 5. 安装器资源树

- `_Lua` 是 editing tree；`resources/_lua/_Lua` 是 `resource.bin` 的真实输入。
- 只更新 `_Lua` 会出现“本地看见新 bin，设备仍安装旧 bin”。
- 两个目录必须各有同一个 `main.lua`、图标和全部版本化 bin。
- `resource.bin` 必须从 packaged tree 重建，随后反向解包逐字节核对。
- `hashCode` 必须在新 `resource.bin` 之后重算；旧 hash 会使安装包身份不一致。
- 不要手工编辑二进制 record table，不要只改 mtime 或文件名。
- 顶层目录里存在多个 bin 是设计要求，不是重复文件；Lua 只选择精确版本。

## 6. Lua 版本选择与 Run

- 使用 `ro.build.version` 的精确 `major.minor.patch` 匹配；不能 fallback 到相邻固件。
- 文件名必须为 `shellpp_ii-<exact-version>.bin`，版本字符串与 profile 一致。
- Lua 的 ELF 头预检只能排除明显错误文件，不能替代完整 verifier 或 module loader。
- 在 `insmod` 前检查 `/dev/shellpp`。存在旧 Supervisor 时必须校验状态 ABI 和 firmware code，不重复加载。
- Run 是有顺序的五步协议；不要跳过通知、restore/install 0、install 1 或 install 2 后仍报告完整成功。
- device `write` 返回 16 只表示控制消息被消费；实际命令错误在 status word 7/9。
- word 18 至 20 是固件原始结果，不可套用项目 errno 表解释。
- 一个固件代码相同的驻留 module 仍可能是旧源码产物；页面数量或 callback 变化后必须重启。

## 7. module 与 native App 生命周期

- constructor 在 loader 加载阶段注册 `/dev/shellpp`；`module_initialize` 只设置 uninitializer。
- `APP_INSTALL` 由固件 worker 异步提交，紧接着一次 lookup 可能看到旧表；当前实现使用有界重试。
- App/Page descriptor、页面名、包名、图标路径和 callback 必须是静态存储；栈对象在注册返回后会失效。
- `APP_INSTALL` 返回值是不透明诊断；项目以 lookup 和包名匹配确认注册。
- Launcher 已出现只证明注册链的一部分，不证明页面 UI ABI 可用。
- 固件长期保存 module text 中的 callback。App 注册后强制 unload 会留下悬空指针并可能在下次点击时崩溃。
- 当前没有已证明的 App/Page 反注册 ABI。`-95` 和 `-16` 是保护机制，正确移除路径是清理后重启。
- 覆盖磁盘 bin 不会替换 RAM 中旧 module，也不会改变已注册页面数。

## 8. UI 与页面

- 036 注册 8 页；043 target patch 注册 9 页。不能在旧 module 驻留时验证新 page 8。
- 036 应用管理是 page 1 内部模式；043 应用管理是独立 page 8。不要把一方行为写回另一方。
- Activity create/resume/pause/destroy 的对象、时序和 timer 清理都属于 ABI。主页能打开不代表其他页可用。
- 043 禁用了旧 MiSans style 调用；不能把该禁用合并到 036 并声称旧 bin 未变。
- 交互 binding 复用依赖当前只有前台页接收事件、row spec 在同步 apply 完成前有效。引入异步渲染或后台扫描后必须重审。
- 页面 destroy 前必须删除 timer、隐藏/销毁浮层并使旧 event cookie 失效，避免 callback 访问复用状态。
- 手动 CPU/内存刷新、timer 刷新和 top-layer 浮层是三组不同 ABI；某一组通过不能代表另外两组。

## 9. 文件系统和应用管理

- 所有设备路径必须绝对化、限制容量并拒绝 `.`、`..`、空组件和越界拼接。
- 符号链接只处理链接本身，目录统计、清理与删除不得跟随链接跨出目标树。
- 未知/device 类型必须失败或跳过，不能按普通文件删除。
- 固定深度显式遍历超过上限时必须失败；不能静默遗漏后仍显示成功。
- 文本查看、Hex 查看、编辑、复制和分页分别有不同大小上限；不要用一个上限推断另一个。
- 写操作必须先写临时文件、完整 close，再 rename 提交；失败时清理临时文件，不能原地截断注册表。
- 目标存在、同路径和 short write 都是失败条件；不能为方便覆盖用户数据。
- 043 注册表是 `/data/apps.json` 与 `/data/apps.json_hide`。`/data/quickapp/` 是 10 Pro 应用数据体系的一部分，不是当前 043 注册表文件。
- 注册表缺失或无效在只读列表中归一化为空列表，不应误报为固件 ABI 读取失败。
- JSON 重写必须保留未选条目与可解析结构；字符串转义、截断和重复 package 需显式处理。
- 隐藏/显示只移动注册条目；卸载还会删除数据目录，无法仅靠注册表备份恢复。
- 测试应用卸载、缓存清理、文件删除和重启前必须准备可恢复数据。

## 10. 故障日志与验证结论

- AstroBox 压缩包可能混有历史 DFX 事件；以精确点击时间和重启边界筛选。
- `sched_dumpstack` 可能是日志收集动作，不一定是 crash。
- 旧日志中的 PC/LR 不能自动归因给新 bin。
- “构建成功”“Run 完成”“通知出现”“Launcher 可见”“主页打开”和“全功能可用”是不同层级。
- host test 证明 mock 环境逻辑，不证明真实固件地址、dirent、线程栈和 LVGL 对象布局。
- 只测试 happy path 不能证明写操作可恢复；还要覆盖缺失文件、空文件、short read/write、目标存在、深度超限和中途失败。
- 真机记录必须绑定固件版本、bin SHA-256、安装包 SHA-256、步骤、结果和日期。

## 11. 提交前快速审计

```text
[ ] 新固件具有完整 ABI 和固定哈希镜像
[ ] 036 canonical 行为未被其他 target 修复改变
[ ] target patch 只作用于临时源码且顺序明确
[ ] 全目标构建退出 0
[ ] SHF_ALLOC 与 .bss 均在各 profile 限制内
[ ] Lua 精确匹配，未知固件拒绝加载
[ ] 两个 Lua 目录内容一致
[ ] resource.bin 已反向核对，hashCode 已重算
[ ] 新页面/回调测试前设备已重启
[ ] 破坏性测试已有备份与恢复路径
[ ] 文档状态与当前哈希、当前真机结论一致
```
