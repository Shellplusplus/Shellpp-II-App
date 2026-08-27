# 发布与恢复

本页定义双固件安装包的发布冻结、失败隔离、设备恢复和回滚边界。发布对象不是单个 bin，而是一个相互一致的集合：一个 Lua、全部受支持固件 bin、图标、`resource.bin`、`hashCode` 和对应构建证据。

## 1. 发布前提

发布候选必须满足：

- 当前支持的每个 profile 都能从固定哈希固件镜像构建；
- 共享源码、target patch 和构建工具的工作树状态已记录；
- Shell++ 专用 ELF verifier 对全部候选 bin 退出 0；
- Lua 语法通过；
- host tests 与变更范围相匹配；
- editing tree 与 packaged tree 的 Lua、bin 和图标一致；
- `resource.bin` 已由 packaged tree 重建并反向核对；
- `hashCode` 已根据当前 capability、manifest 与 `resource.bin` 重算；
- 真机结论绑定精确固件版本和 bin 哈希。

构建器输出 `OK` 但后续部署、重打包或比较失败时，整次发布失败。不得从临时输出中挑一个 bin 手工覆盖安装器并称为成功发布。

## 2. 标准发布命令

```sh
cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
./build.sh
```

无参数命令是发布路径。`--target` 适用于固件专用开发迭代，不是共享源码或构建工具变更后的完整发布证明。

发布记录至少保存：

| 类别 | 必需记录 |
| --- | --- |
| 源输入 | 三仓库 commit/工作树状态，当前构建输入列表 |
| 固件 | 版本、镜像绝对路径、大小、SHA-256 |
| target | target ID、profile、target patch 列表及顺序 |
| 构建 | 命令、日期、退出码、完整 verifier 摘要 |
| bin | 文件名、大小、SHF_ALLOC、`.bss`、SHA-256 |
| 安装包 | Lua、图标、`resource.bin`、`hashCode` 的 SHA-256 |
| 比较 | out、editing tree、packaged tree、反向解包 payload 的比较结果 |
| 设备 | 型号、固件版本、bin 哈希、已执行场景和结果 |

## 3. 发布冻结顺序

1. 记录三仓库 `git status --short`，辨别本任务修改与既有用户修改。
2. 执行无参数全目标构建。
3. 保存每个 verifier 的完整输出与退出码。
4. 计算构建输出与两个安装器目录中各 bin 的 SHA-256。
5. 反向解析 `resource.bin`，逐字节比较 Lua、全部 bin 和图标。
6. 保存 `resource.bin` 与 `hashCode` 哈希。
7. 在设备重启后的干净驻留状态测试 Lua 版本选择、Run、打开 App 和受影响功能。
8. 将新哈希和证据等级更新到[当前目标](current-targets.md)。

冻结只描述一组字节。修改任一源文件、profile、patch、编译参数、工具链、资源或 manifest 后，即使文件名未变，旧冻结记录也不再适用于新产物。

## 4. 构建失败时的恢复

`build.sh` 先在 stage 中完成所选目标，再进入部署。任一目标失败时，应保留安装器中的旧完整集合。处理方式是修复第一个失败原因后重复同一命令：

- profile 或镜像身份失败：核对输入，不把当前文件哈希直接抄入 profile；
- patch 失败：确认共享源码是否变化，更新目标专用 patch 的上下文并重新审查语义；
- 编译或链接失败：修复类型、未定义符号、section 或 relocation，不删除安全 gate；
- ELF 尺寸失败：定位真实 footprint/BSS 增量，不以提高上限掩盖问题；
- 部署比较失败：检查目录、权限、磁盘和源/目标内容；
- resource 重打包失败：修复 packaged tree 或 record 约束，再完整重建。

不要手工拼接 `resource.bin`，不要只重算 `hashCode`，不要删除旧目标 bin 来让数量检查通过。

## 5. 安装失败后的设备恢复

Lua 的恢复动作与 native module 生命周期不同：

| 动作 | 影响 | 不会完成的事 |
| --- | --- | --- |
| Uninstall | 删除 `/data/shellpp-ii` staging，并提示重启 | 不发送 `CMD_UNINSTALL`，也不解除固件保存的 callback |
| Clear Env | 删除 `/data/shellpp-ii` 中由安装器 staging 的文件 | 不清除 RAM 中的 driver/App/Page registry |
| Reboot | 清除驻留 module 和 RAM 注册记录 | 不自动恢复被应用管理改写的注册表文件 |

推荐恢复流程：

1. 若界面仍可用，先记录 `/dev/shellpp` 状态 words 1、2、6、7、9、12 至 20。
2. 保存故障发生的精确本地时间、固件版本、安装包和 bin 哈希。
3. 不重复 `insmod`，不强制卸载已注册 module。
4. 清理 `/data/shellpp-ii` staging 环境。
5. 重启设备，确认 `/dev/shellpp` 和旧 Launcher/App 注册已随 RAM 清除。
6. 用已冻结的上一套完整安装包重新安装。

覆盖磁盘上的 bin 不能替换已经驻留的 module。只要固件还保留旧描述符和 callback，测试结果就可能仍来自旧代码。

## 6. 应用注册表恢复

应用隐藏、显示和卸载会操作固件注册表或应用数据，属于破坏性路径。测试前必须备份设备上的实际文件；043 当前注册表是：

```text
/data/apps.json
/data/apps.json_hide
```

恢复时：

- 只恢复从同一设备、同一固件、同一测试前状态取得的备份；
- 先检查 JSON 完整、`InstalledApps` 结构存在、包名唯一；
- 使用原子替换语义写回，不能在原文件上边解析边覆盖；
- 保留文件权限和固件可接受的编码；
- 恢复注册表后重启，让固件重新建立内存状态；
- 已删除的应用数据目录无法由注册表备份恢复，必须另有数据备份或重新安装应用。

不要用 036 的路径假设恢复 043，也不要把 `/data/quickapp/` 根目录本身误当成 043 当前注册表文件。

## 7. 旧版本回滚限制

可恢复的对象分三类：

- 构建产物：可从已冻结的源码/profile/工具链重建，或使用已保存的完整安装包。
- 安装器 staging：可由 Clear Env 删除并由安装重新创建。
- 设备持久数据：只有事先备份才能可靠恢复。

回滚必须使用一套完整匹配的 Lua、全部 bin、图标、`resource.bin` 和 `hashCode`。不能把旧 bin 放进新 `resource.bin`，也不能在 043 系统上强制加载 036 bin。Lua 的固件代码与状态 word 12 检查是最低防线，不是 ABI 正确性的替代。

## 8. 日志与故障证据

采集 AstroBox 日志时记录操作发生时间和重启边界。日志包可能包含：

- 本次设备输出；
- 收集工具主动触发的 `sched_dumpstack`；
- 早于本次测试的历史 DFX/Fault 文件；
- 重启后的新启动日志。

只有时间、固件版本、操作步骤和重启边界匹配的记录才能归因给本次产物。历史 PC/LR 不能作为当前 bin 仍然崩溃的证据。

## 9. 发布后核验

发布后至少重新确认：

- Lua 显示的版本与系统 `ro.build.version` 一致；
- 被选择的文件名是精确版本 bin；
- `/dev/shellpp` ABI 为 3，word 12 等于目标 firmware code；
- 第二次 Run 被驻留检查阻止，不重复注册；
- 打开 Launcher 图标进入正确页面；
- 036 保持八页基线，043 的应用管理进入独立第九页；
- 移除或切换版本前执行重启。

详细测试项见[验证](validation.md)，症状处理见[故障排查](troubleshooting.md)。
