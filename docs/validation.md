# 验证

本页定义 Shell++ II 的分层验证合同。每一层只能证明其直接覆盖的性质，不能用低层通过替代高层证据。所有设备结论必须绑定机型、精确固件版本、bin SHA-256 和实际执行场景。

## 1. 证据层级

| 层级 | 权威输入 | 通过条件 | 仍不能证明 |
| --- | --- | --- | --- |
| profile schema | `targets/*.env`、`generate_target_abi.py` | 必填字段齐全、格式和基础数值约束成立 | 地址身份、原型、运行行为 |
| 固件身份 | profile 中镜像路径/大小/SHA-256 | 实际镜像三项完全匹配 | ABI 恢复正确 |
| 编译链接 | 五个真实输入、生成头、target patch、编译参数 | 所选目标全部编译，`rust-lld -r` 成功 | loader 和设备 ABI 可用 |
| ELF 静态 gate | `verify_shellpp_elf.py` | 格式、section、symbol、relocation、地址白名单和资源限制通过 | 函数语义、结构布局、栈、时序 |
| host test | patch 后 C 源、mock 固件 ABI、ASan/UBSan | 断言通过且 sanitizer 无报告 | 真实固件地址、dirent、LVGL、设备线程栈 |
| Lua 静态检查 | 安装器 `main.lua` | `luac -p` 通过，代码审查确认选择/协议一致 | 设备 shell 命令、module loader 和 native 行为 |
| 部署一致性 | out、editing tree、packaged tree | 每个版本化 bin 逐字节一致 | 最终包确实包含这些 payload |
| 打包一致性 | 反向解析 `resource.bin` | Lua、全部 bin、图标与 packaged tree 逐字节一致，hash 已重算 | 设备实际使用本次包 |
| 真机确认 | 指定固件、指定哈希、操作记录 | 已执行场景符合预期且无异常重启/数据损坏 | 未执行路径、其他固件、长期稳定性 |

“构建验证”不能写成“真机确认”。“Run 完成”也不能推导文件、应用、缓存、监控和破坏性操作全部成立。

## 2. 当前支持状态

3.101.036 与 3.101.043 均已由同一次全目标构建生成、部署并打包，且用户已确认两个固件版本可用。当前哈希与资源尺寸见[当前目标](current-targets.md)。

这个结论允许描述当前项目整体可用，但不虚构未提供的逐项实验记录。涉及隐藏、显示、卸载、文件删除、缓存实际清理、重启异常注入或长期运行时，应单独保存测试记录；没有记录的项目写“未单独验证”，不能借整体可用结论代替。

## 3. 静态和主机命令

全目标构建：

```sh
cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
./build.sh
```

单目标开发构建：

```sh
./build.sh --target xiaomi-band-10-pro-3.101.043
```

单目标模式只覆盖所选 profile。共享源码、profile schema、生成器、verifier、链接器、部署、资源打包或 Lua 发生变化时，必须回到无参数全目标构建。

Lua 语法：

```sh
luac -p /Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer/_Lua/main.lua
```

043 target host tests：

```sh
cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
sh tests/run_native_fs_host_tests.sh
sh tests/run_native_ui_host_tests.sh
```

这两个脚本从共享源码重放 043 的全部 target patch。文件系统测试覆盖路径、分页、遍历、链接、应用目录、缓存、CPU/内存解析与失败边界；UI 测试覆盖 page 1/page 8 导航、标题、注册表直读和缺省空列表语义。

三仓库文本检查：

```sh
git -C /Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii diff --check
git -C /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build diff --check
git -C /Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer diff --check
```

如果第三方或既有文件本身含 CRLF 或 trailing whitespace，应精确区分既有问题和本次引入问题，不擅自格式化整个 manifest 或资源树。

## 4. profile 与镜像矩阵

每个 target 必须记录：

| 检查 | 证据 | 失败处理 |
| --- | --- | --- |
| profile 解析 | 生成器退出 0 | 修复缺失、未知、重复或非法字段 |
| 版本代码 | `major*1000000 + minor*1000 + patch` | 不修改公式，不容忍近似版本 |
| 镜像路径 | 文件存在且为分析所用精确文件 | 找回正确镜像 |
| 镜像大小 | 与 profile 完全一致 | 查明截断、补齐或 variant |
| 镜像 SHA-256 | 与 profile 完全一致 | 重新建立 ABI 与镜像证据关系 |
| ABI 完整性 | 字段、原型、布局、时序均有证据 | 返回 ABI 恢复阶段，不填占位值 |

生成器通过只证明 profile 可表达。ABI 地址的身份和原型必须由完整 ABI 材料、固定镜像反汇编和交叉证据证明。

## 5. 构建与 ELF gate

一次全目标发布构建必须同时满足：

1. 所有 profile 均被发现且 target ID、版本无冲突。
2. 固件镜像大小和 SHA-256 匹配。
3. 目标补丁按排序顺序干净应用到临时源码。
4. 五个输入以 ARM Cortex-M33、soft-float、`-Wall -Wextra -Werror` 编译。
5. relocatable link 成功且不含未定义 import。
6. 每个 bin 通过[验证契约](reference/verification-contract.md)。
7. SHF_ALLOC footprint 严格小于 profile 上限。
8. `.bss` 小于或等于 profile 上限。
9. 所选目标全部成功后才进入部署。
10. 构建日志保存退出码、文件大小、footprint、BSS、symbols、relocations、sections 与 SHA-256。

只检查文件存在或 mtime 不足，因为它可能是旧构建残留。必须以本次命令退出码和完整输出为证据。

## 6. 部署与打包验证

对每个 `<target>/<version>` 执行等价比较：

```sh
cmp -s \
  /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build/out/<target>/shellpp_ii-<version>.bin \
  /Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer/_Lua/shellpp_ii-<version>.bin
cmp -s \
  /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build/out/<target>/shellpp_ii-<version>.bin \
  /Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer/resources/_lua/_Lua/shellpp_ii-<version>.bin
```

还必须验证：

- editing tree 与 packaged tree 的 `main.lua` 相同；
- 两处图标相同；
- 两处版本化 bin 集合与受支持 profile 集合精确对应；
- `resource.bin` 从 packaged tree 重建；
- 反向解析出的四类 payload 与 packaged tree 逐字节相同；
- `hashCode` 在最终 `resource.bin` 之后重算。

只比较顶层 `_Lua` 无法证明设备包内容。只比较 `resource.bin` 哈希也无法定位其内部是否混入旧 payload。

## 7. Lua 与 Supervisor 协议测试

至少覆盖：

| 场景 | 预期 |
| --- | --- |
| 支持版本 | 精确选择同名 bin |
| 未知版本 | 在 `insmod` 前拒绝，不 fallback |
| bin 缺失、过小或过大 | 拒绝加载并显示明确错误 |
| ELF 头不符 | Lua 预检拒绝 |
| 无驻留 driver | 加载后 `/dev/shellpp` 出现 |
| ABI 非 3 | 判为旧 Supervisor，要求重启 |
| word 12 不匹配 | 判为错误固件 target，要求重启 |
| 有效控制失败 | write 返回 16，状态 word 7/9 报错 |
| 第二次 Run | 在重复 `insmod` 或注册前被阻止 |
| 完整 Run | 五个 stage 按序完成且状态一致 |

协议 word、命令与错误路径见[状态与控制 ABI](reference/status-control-abi.md)和[错误码](reference/error-codes.md)。

## 8. 真机功能矩阵

每个固件单独记录，不得把 036 结果复制为 043 结果：

| 类别 | 检查项 | 通过标准 |
| --- | --- | --- |
| 安装 | 版本、加载、driver、firmware code | 精确 target，无旧驻留冲突 |
| Run | 通知、restore、install 0/1/2 | 每步完成，错误和诊断值可解释 |
| 注册 | App ID、包名、Launcher | `0x00cd`、`com.shellpp.ii`、入口唯一 |
| 生命周期 | create/resume/pause/destroy | 打开、切换、返回、重复进入无崩溃 |
| 页面数 | 036 八页、043 九页 | 目标页面集合与 bin 一致 |
| 文件只读 | 列表、分页、文本、Hex | 内容正确，边界和链接安全 |
| 文件写入 | 编辑、复制、移动、删除 | 只操作测试数据，失败可恢复 |
| 应用只读 | 列表、标题、注册表 | 043 新开“应用管理”，无错误读取提示 |
| 应用写入 | 隐藏、显示、卸载 | JSON 可解析，无关条目不变，数据范围正确 |
| 缓存 | 统计、刷新、实际清理 | 计数一致，不跟随链接，不越界删除 |
| CPU/内存 | 手动刷新 | 百分比与原始文本合理 |
| 定时监控 | timer create/update/delete | 页面退出后无 callback 残留 |
| 悬浮监控 | top layer、align、hidden | 显示/隐藏正确，无对象泄漏 |
| 重启 | soft/hard | 行为符合选择且设备可恢复 |
| 移除 | Clear Env + reboot | staging 与驻留注册按设计清除 |
| 重复安装 | 第二次 Run、旧 module | 被安全拒绝，不重复注册 |

破坏性测试前备份注册表和测试数据。一步出现崩溃后停止扩大测试范围，记录精确时间并立即采集新日志。

## 9. 结果记录模板

```text
设备型号:
系统版本:
固件镜像 SHA-256:
target ID:
bin 文件名/大小/SHA-256:
resource.bin SHA-256:
测试日期与时区:
重启后首次安装: 通过/失败
Run 五步: 通过/失败（附 words 6,7,9,12-20）
页面与功能:
  <场景>: 通过/失败/跳过/未验证
日志路径与操作时间:
恢复操作:
剩余限制:
```

“跳过”必须写原因；“未验证”不得写成“预计可用”。

## 10. 变更影响与最低回归

| 变更 | 最低验证范围 |
| --- | --- |
| 纯文档 | 相对链接、事实检索、三仓库 diff 检查 |
| 单 target profile | 生成器、镜像、全目标构建、新 target 受影响真机路径 |
| 单 target patch | patch 重放、host tests、全目标构建、旧目标不变、新目标真机路径 |
| 共享 native 源 | 全目标构建、所有固件受影响功能真机回归 |
| schema/生成器/verifier/linker | 正例、相关负例、全目标构建 |
| Lua 选择/协议 | `luac -p`、支持/拒绝/旧驻留场景、全部固件 Run |
| 部署/打包 | 全失败不部署、全成功完整替换、resource 反向比较 |
| descriptor/page 数/callback | 重启后每个固件完整注册和生命周期测试 |
| 文件/注册表写入 | 失败注入、备份恢复、每个受影响固件真机测试 |

## 11. 文档自身验证

文档交付至少满足：

- `docs/README.md` 中所有入口存在；
- 全部 Markdown 相对链接可解析；
- 完整 ABI 准入条件位于入口开头，并在适配指南和 AI 交接重复为硬 gate；
- 不把 legacy 文件描述为编译输入；
- 不把 packaged tree 写成顶层 `_Lua`；
- 不出现旧 patch 名或错误注册表路径；
- 当前 043 状态写为用户确认可用，而历史崩溃只在排障文档中作为历史案例；
- 资源上限语义准确：loaded footprint 为 `<`，BSS 为 `<=`；
- 地址、常量、哈希与当前 profile/构建产物一致。

## 12. 完成判定

适配完成要求 profile、完整 ABI、全目标构建、打包一致性、版本选择、真机页面与受影响功能都具备证据。测试或证据缺失时，应明确保留较低证据等级，不能因为构建退出 0、通知出现或 Launcher 可见而提前关闭适配。
