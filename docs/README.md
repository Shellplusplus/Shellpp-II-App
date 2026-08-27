# Shell++ II 10 Pro 技术文档

> [!IMPORTANT]
> **同一机型的新系统适配必须以该系统的完整固件 ABI 为前置输入。**
> 在完整 ABI 到位以前，只能进行固件分析、建立 ABI 缺口表和保存证据；不得创建可发布 target，不得填写占位地址，不得用相邻版本地址差值代替符号恢复，也不得把“能够编译”描述为“适配完成”。

本目录是 Shell++ II 在 Xiaomi Band 10 Pro 上的中央技术文档。它说明当前实现、固件边界、构建与打包流程、Lua 安装器协议、native App 注册、页面与文件系统实现、新固件适配、验证、发布、恢复和故障诊断。

文档只描述可由当前源码、target profile、构建工具或已记录运行结果支持的技术事实。它不包含产品宣传、参与动员、效果承诺或未经验证的推断。

## 1. 文档范围

整套程序由三个独立 Git 仓库组成：

| 组件 | 当前开发机路径 | 规范职责 |
| --- | --- | --- |
| nativeApp | `/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii` | 保存一套共享 C/汇编源码、稳定头文件和本目录文档 |
| 构建器 | `/Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build` | 保存固件 profile、target 补丁、ABI 头生成器、编译链接、ELF 验证、部署和资源重打包逻辑 |
| Lua 安装器 | `/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer` | 保存一个 Lua 入口、多个按固件版本命名的 bin、图标和最终安装包资源 |

固件和逆向资料位于 `/Users/ikun_cxkpro/Projects/固件修改/10p`。它们是 ABI 证据输入，不由本项目构建器生成或改写。

上述绝对路径是当前构建器的实际配置。构建器的安装器输出根目录必须是：

```text
/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer
```

迁移工作区时必须显式更新并审查 `build.sh` 中的路径；不要依赖大小写近似路径、未记录的符号链接或当前 shell 工作目录。

## 2. 关键结论

- nativeApp 长期维护一套共享源码。036 是共享行为基线。
- 043 的固件专用语义差异只存在于构建器 `targets/xiaomi-band-10-pro-3.101.043/patches/`；补丁只应用到 `out/<target>/target-src` 临时副本。
- 固件地址、固件常量和描述符尺寸只由 `targets/*.env` 提供，经生成器注入。共享源码没有默认固件 ABI。
- 无参数 `./build.sh` 编译所有 profile。只有全部所选目标都通过后，构建器才部署 bin 并重建安装包。
- 安装器只有一个 `main.lua`。它读取 `ro.build.version`，精确选择 `shellpp_ii-<major.minor.patch>.bin`，不做相邻版本 fallback。
- Supervisor 通过 `/dev/shellpp` 提供 384 字节状态和 16 字节控制协议。
- native App 注册后，固件会长期保留描述符和回调指针。逆向注销 ABI 尚未确认，因此不能在运行期卸载 module；清理后必须重启。
- 3.101.036 和 3.101.043 已由同一次全目标构建生成。043 已由用户确认可用；当前精确产物状态见[当前目标](current-targets.md)。

## 3. 文档地图

建议按下列顺序阅读：

1. [系统概览](system-overview.md)：构建时和运行时的数据流、稳定边界与完成定义。
2. [仓库结构](repository-layout.md)：所有手工文件、生成文件、legacy 文件和所有权。
3. [native 模块架构](native-module-architecture.md)：module 入口、Supervisor 和内部子系统。
4. [native App 注册](native-app-registration.md)：描述符布局、注册阶段、Launcher、通知和卸载限制。
5. [UI 与文件系统](ui-filesystem.md)：页面生命周期、浏览器、系统监控、缓存和重启。
6. [应用管理](application-management.md)：注册表读取、JSON 改写、数据目录和破坏性操作边界。
7. [Lua 安装器](installer-protocol.md)：版本检测、ELF 预检、Run 状态机和清理行为。
8. [固件 ABI](firmware-abi.md)：ABI 的范围、地址映射、函数原型和证据要求。
9. [构建系统](build-system.md)：profile 校验、多目标编译、ELF gate、部署和打包。
10. [固件适配指南](firmware-porting.md)：输入、逐阶段操作、修改位置、出口条件和禁止项。
11. [验证](validation.md)：主机、静态、打包和真机验证矩阵。
12. [发布与恢复](release-recovery.md)：发布记录、产物固定、失败恢复和设备回滚边界。
13. [故障排查](troubleshooting.md)：按症状和阶段定位问题。
14. [已知陷阱](known-pitfalls.md)：跨模块风险清单和防错规则。
15. [AI 交接规范](ai-handoff.md)：自动化代理的读取顺序、修改约束和完成审计。

规范参考：

- [状态与控制 ABI](reference/status-control-abi.md)
- [目标 profile 模式](reference/target-profile-schema.md)
- [错误码](reference/error-codes.md)
- [验证契约](reference/verification-contract.md)

## 4. 术语

| 术语 | 本文定义 |
| --- | --- |
| firmware / 固件 | 设备上精确的 Xiaomi Vela 系统版本及其与 profile 固定的 `vela_ap.bin` 镜像 |
| ABI | native module 调用固件所需的函数地址、数据地址、原型、结构布局、枚举、常量、时序和生命周期约束的完整集合 |
| profile | 构建器 `targets/<TARGET_ID>.env` 中一个固件目标的机器可读元数据和 ABI 数值 |
| target | 一个由机型、精确固件版本、profile 和可选专用补丁共同定义的构建目标 |
| shared source / 共享源码 | nativeApp `module/src` 中长期维护的 canonical 源码；当前以 3.101.036 行为为基线 |
| target patch | 仅在指定 target 的临时源码副本中应用的、有证据支持的语义差异补丁 |
| Supervisor | module 中注册 `/dev/shellpp`、接收 Lua 命令并协调 native App 注册的控制层 |
| native App | 通过固件 App/Page registry 注册、由 Launcher 打开的 Shell++ II 原生界面 |
| packaged tree | 安装器 `resources/_lua/_Lua`；`resource.bin` 的实际资源输入 |
| editing tree | 安装器 `_Lua`；便于直接编辑和检查的资源副本 |
| host test | 在 macOS mock 固件 ABI 上运行的 C 逻辑测试，不等同于真机 ABI 验证 |
| frozen artifact | 已记录文件名、大小和 SHA-256，后续变更必须显式解释的产物 |

## 5. 证据等级

文档使用以下状态，不允许混用：

| 状态 | 允许声明的结论 |
| --- | --- |
| `源码确认` | 结论可由当前参与构建的代码直接证明 |
| `静态恢复` | 结论来自固定哈希固件的反汇编、交叉引用、调用图、字符串或数据流分析 |
| `主机验证` | mock ABI 下的目标 C 逻辑测试通过；不能证明真实地址、栈或固件对象布局 |
| `构建验证` | profile、固件身份、编译、链接和静态 ELF gate 通过 |
| `打包验证` | 构建输出、两个 Lua 目录和反向解析的 `resource.bin` 逐字节一致 |
| `真机确认` | 指定固件和指定 bin 哈希已由设备操作结果确认 |
| `未验证` | 没有足够证据，不得转述为已支持 |

“构建验证”不包含函数原型、线程栈、固件 worker 时序、页面生命周期或写操作正确性。“Run completed”也不等于全部页面和破坏性功能已真机验证。

## 6. 权威来源

遇到文档与实现不一致时，按职责选择来源，而不是简单以单个文件覆盖全部事实：

| 事实 | 权威来源 |
| --- | --- |
| 固件版本、镜像身份、地址、常量、描述符尺寸、资源上限 | 构建器 `targets/*.env` |
| profile 允许字段与形式校验 | `generate_target_abi.py` |
| 实际编译输入、编译参数、target 补丁、部署语义 | `build.sh` |
| module 静态格式和地址白名单 gate | `verify_shellpp_elf.py` |
| 共享运行行为 | `build.sh` 实际编译的 nativeApp 源文件 |
| 目标专用运行行为 | 共享源码按顺序应用该 target 全部补丁后的 `target-src` |
| 安装器选择和命令序列 | 安装器 `_Lua/main.lua` |
| 安装包内容 | 反向解析后的 `resource.bin`，并与 packaged tree 比较 |
| 真机支持状态 | 带固件和 bin 哈希的当前验证记录 |

生成头 `out/<target>/generated/shellpp_target_abi.h`、对象文件、`target-src` 和 bin 都是可重建输出，不是长期手工维护来源。

## 7. 当前构建命令

全目标构建和部署：

```sh
cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
./build.sh
```

只构建一个已有 target：

```sh
./build.sh --target xiaomi-band-10-pro-3.101.043
```

共享源码、profile schema、生成器、验证器、部署或打包逻辑发生变化时必须执行全目标构建。单 target 模式只适用于已经隔离的目标专用迭代。

## 8. 适配完成定义

新增固件只有同时满足下列条件才算完成：

1. 固件镜像身份已由路径、大小和 SHA-256 固定。
2. 完整 ABI 的每个必需字段均有独立证据，原型和关键布局无未处理缺口。
3. 新 profile 通过严格生成器，且没有占位值、复制值或猜测值。
4. 必要语义差异被建模为 profile 字段或最小 target 补丁，没有改坏已验证旧目标。
5. 全目标构建通过，所有 target 的静态 ELF gate 通过。
6. 两个安装器 Lua 目录、最终 `resource.bin` 和 `hashCode` 已核对。
7. Lua 能精确识别版本、加载正确 bin，并拒绝错误或旧驻留 module。
8. 新目标全部注册页面和主要生命周期在真机上通过。
9. 只读功能和写操作按风险分级验证；未测试项目明确记录。
10. 固件、profile、bin、安装包哈希、测试日期、实际结果和已知限制均进入验证记录。

任何一项证据缺失时，状态应保持为“适配中”或相应较低证据等级。

## 9. 文档维护规则

- 所有项目技术文档统一保存到本 `docs/`，不要在三个仓库中维护互相漂移的第二套说明。
- 实现修改必须同步更新受影响的专题、参考表和验证记录。
- 新增或删除构建输入时更新[仓库结构](repository-layout.md)与[构建系统](build-system.md)。
- 更改 Lua/Supervisor 协议时同步修改两端和[状态与控制 ABI](reference/status-control-abi.md)。
- 更改 profile schema 时同步修改生成器、profile 参考、适配指南和所有现有 profile。
- 真机结论必须绑定精确产物哈希；不能只写“最新 bin”或“当前版本”。
- 历史日志只能解释当时的故障，不能作为新产物仍然崩溃的证据。
- 文档示例不能引入代码中不存在的 fallback、卸载、恢复或安全保证。
