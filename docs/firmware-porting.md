# 新固件适配指南

## 0. 硬性准入条件

适配同一机型上的新系统版本前，必须先准备该固件的完整 ABI。这是开始 target 适配工作的前置条件，不是构建完成后再补充的材料。

完整 ABI 不等于拥有固件镜像，也不等于找到一批看起来相近的函数地址。至少必须覆盖：

- profile schema 要求的全部函数地址、数据地址、常量、枚举和 descriptor 尺寸；
- 每个函数的独立身份、运行时地址、Thumb 形式、C 原型和调用约定；
- App/Page descriptor、notification、dirent 和固件对象的关键布局；
- worker、timer、Activity、页面回调的时序、线程和指针保存期限；
- 文件系统根路径、注册表格式、链接、rename 和删除语义；
- 页面生命周期、栈、BSS、重入和 unload 约束；
- 精确固件镜像路径、大小、SHA-256，以及分析地址到运行时地址的映射证据。

缺少任一必需项时，只能继续做 ABI 分析并记录缺口。不得创建可发布 target，不得用旧固件值、固定地址差值、最近候选或零值占位，不得让 Lua fallback 到相邻版本，也不得修改已验证的 036 代码来掩盖新固件缺口。

## 1. 阶段与出口

| 阶段 | 输入 | 必须产出 | 出口状态 |
| --- | --- | --- | --- |
| A. 镜像固定 | 原始固件包、设备版本 | 路径、版本、大小、SHA-256 | 镜像可唯一复现 |
| B. ABI 恢复 | 固件、反汇编、参考实现 | 完整 ABI 证据表 | 无必需缺口 |
| C. profile | ABI 证据、镜像身份 | 新的 targets/*.env | 生成器通过 |
| D. 语义差异 | 新旧行为对比 | 最小 target patch，可选 | 可从共享源码重放 |
| E. 构建 | 全部 profiles、工具链 | 每目标独立 bin | 全部 ELF gate 通过 |
| F. 打包 | staged bin、packaged tree | 两个 Lua tree、resource、hash | 逐字节一致 |
| G. 真机 | 精确固件和 bin | 分阶段测试记录 | 可声明对应验证等级 |

后一个阶段的成功不能覆盖前一个阶段的缺失。例如 bin 能链接不代表 ABI 完整，Run 能完成不代表全部页面和写操作已通过。

## 2. 必须准备的材料

开始前至少准备：

1. 设备真实返回的 ro.build.version；
2. 与版本一一对应的 vela_ap.bin；
3. 固件文件字节数和 SHA-256；
4. 固件包来源、提取方式、日期和工具版本；
5. 完整 ABI 字段表及逐项证据；
6. 参考固件或参考 native module 的版本和哈希；
7. 新固件与 036 共享基线的行为差异记录；
8. 能重启、恢复和采集新日志的真机环境；
9. 应用注册表、测试文件和缓存目录的备份；
10. 测试结果、失败日志和未测试项的记录模板。

目录名、版本字符串和镜像内容必须由哈希绑定，不能只准备一个名为新版本的目录。

## 3. 固定固件身份

在构建器 profile 中记录精确镜像路径。用下列命令固定身份：

    wc -c /path/to/vela_ap.bin
    shasum -a 256 /path/to/vela_ap.bin

同时记录设备属性原始输出、固件包来源文件名、绝对路径、计算日期和任何同名但不同哈希的候选。FIRMWARE_IMAGE_SIZE 和 FIRMWARE_IMAGE_SHA256 是 build gate，不是备注。

同名镜像但 SHA-256 不同必须视为不同输入，不能复用旧 ABI。

## 4. 恢复完整 ABI

### 4.1 函数和数据

逐项确认：

- driver register/unregister；
- open/read/write/close/lseek、unlink/rename、opendir/readdir/closedir/rmdir；
- App lookup/install、Launcher add、notification submit；
- LVX/LVGL object、label、style、list row、event、timer；
- Activity navigate/finish；
- spawn、file-actions、spawn attributes、waitpid、soft restart；
- MiSans style 数据地址。

每个函数记录入口反汇编、调用点、参数来源、返回值使用和候选排除理由。数据地址记录对齐、读写方式和生命周期。

### 4.2 非地址 ABI

还必须确认：

- O_*、SEEK_*、DT_* 数值；
- align、event、trailing 枚举；
- App/Page descriptor size 和内部偏移；
- 0x58 notification record；
- dirent 类型字节和 name 起始偏移；
- App registry 路径、JSON 字段和重新加载时机；
- callback 同步消费还是长期保存；
- worker、timer、Activity 和 UI callback 的调用线程。

POSIX 或 LVGL 中存在同名常量不能作为证据。

### 4.3 地址映射

先用目标镜像启动头证明文件偏移到运行时 XIP 的映射，再转换候选地址。当前两份已固定哈希的 10 Pro 镜像关系是：

    analysis = 0x2c000000 + file_offset
    runtime  = 0x0c0c0000 + file_offset

该结论不能无条件迁移到后续镜像。禁止把 runtime 地址低 24 位直接当文件偏移；这会系统性错开 0x0c0000。转换后仍须确认函数边界、Thumb bit、调用图和原型。

### 4.4 证据表

| 字段 | 必填内容 |
| --- | --- |
| profile key | 与 schema 完全一致 |
| image | 路径、大小、SHA-256 |
| analysis address | 分析工具地址 |
| runtime address | module 实际调用地址 |
| identity evidence | 入口、调用点、字符串、机器码、数据流 |
| prototype | 参数、返回值、寄存器约定 |
| layout | 结构偏移、尺寸、对齐 |
| timing | 同步/异步、线程、指针保存 |
| status | 静态恢复、主机验证、真机确认或未验证 |
| rejected candidates | 被排除地址及原因 |

只有必填项没有未处理缺口，才能进入 profile 阶段。

## 5. 创建 target profile

在构建器创建：

    /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build/targets/
      xiaomi-band-10-pro-<version>.env

可以复制现有文件作为字段模板，但必须逐项替换固件身份和 ABI 值。profile 只能使用无引号、无空白、无 shell 元字符的 KEY=VALUE 行。

先独立运行生成器：

    cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
    python3 generate_target_abi.py targets/xiaomi-band-10-pro-<version>.env /tmp/shellpp_target_abi.h

生成器通过只证明字段存在、格式和形式约束，不证明地址身份或原型正确。

检查 target ID、精确版本、firmware code、镜像身份、函数 Thumb 地址、数据对齐、descriptor size 和 loaded/BSS 预算。不能为让构建通过而随意放大资源上限。

## 6. 判断是否需要 target patch

先用 profile 表达地址、常量、枚举、尺寸和资源限制。只有新旧固件的语义或资源行为无法由 profile 表达，并且差异已有证据时，才创建：

    targets/<TARGET_ID>/patches/0001-short-description.patch

patch 必须：

- 从 nativeApp 当前 module/src 可干净应用；
- 使用 a/<file>、b/<file> 路径，供 patch -p1 使用；
- 只改变对应 target 的最小语义；
- 说明固件证据、栈/BSS 预算、buffer 所有权和验证范围；
- 不包含 ABI 地址差异，不复制完整源码；
- 不修改 canonical nativeApp，也不进入 036；
- 共享源码变化后重新 dry-run 和全目标构建。

043 的七个 patch 可作为组织形式参考，不能未经新固件证据直接复制。

## 7. 修改位置

| 需求 | 正常修改位置 | 禁止或例外 |
| --- | --- | --- |
| 新固件地址、常量 | 构建器新 .env | 不写 nativeApp 或 Lua 地址表 |
| 新固件语义差异 | 该 target 的 patches | 不改 036 共享行为 |
| 新 ABI 字段 | schema、生成器、稳定 include、源码、verifier | 不只改生成头 |
| Lua/Supervisor 协议改变 | Lua、supervisor.c、参考文档、测试 | 不只改一端 |
| 共享功能改变 | nativeApp 五个真实输入 | 不以 legacy 文件替代 |
| 安装器 bin | build.sh 管理的两个 Lua tree | 不手工只放一处 |
| 文档 | nativeApp/docs | 不复制多份 ABI 地址表 |

固件命名和协议不变时，新增固件只需修改构建器。只有 profile 模式无法表达的跨固件公共契约变化，才扩展 nativeApp 或 Lua。

## 8. 构建

先目标级快速检查，再全目标：

    cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
    ./build.sh --target xiaomi-band-10-pro-<version>
    ./build.sh

单目标输出用于定位；全目标才证明共享源、生成器、链接器、部署和既有 target 未回归。

核对 generated header 来源、target-src patch 顺序、ELF gate、输出命名、两个 installer tree 和旧 target 冻结哈希。不同版本 bin 若意外完全相同，必须调查是否误用了同一 generated header，不能直接接受。

## 9. 安装器检查

命名契约不变时不修改 Lua，但仍运行：

    luac -p /Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer/_Lua/main.lua

确认版本字符串与 bin 文件名精确一致，firmware code 与 profile 相同，两个 Lua tree 都有新 bin，main.lua 和图标一致，resource.bin 从 packaged tree 重建，hashCode 已重算。

版本格式、command magic、状态 ABI、图标格式、device path 或五步顺序变化时，属于跨仓库协议升级，不能只新增 profile。

## 10. 真机验证顺序

开始前保存 bin/resource/hash 摘要，重启清除旧 module 和 App/Page registry，准备新日志采集和测试数据备份。

按风险逐步扩大：

1. 安装器识别精确版本，错误版本不提供操作；
2. insmod 成功，/dev/shellpp 出现；
3. 状态 magic、ABI、firmware code、driver flag 正确；
4. notification、restore、install 0 完成；
5. install 1 完成，App ID 与 package 匹配；
6. install 2 完成，Launcher 出现；
7. 打开 App，逐页 create/resume/pause/destroy；
8. 只读文件根目录、分页、文本和 Hex；
9. CPU/内存单次刷新，再测 timer 和 overlay；
10. cache 只读统计；
11. 专用测试数据上的 copy、move、delete；
12. 应用管理只读和列表分页；
13. 备份后分别验证 hide、show、delete；
14. 通知、restart、重启后状态；
15. 清理环境、再次重启和重复 Run 拒绝。

任一步崩溃或数据异常都停止扩大范围，保存新日志和精确操作时间。历史日志不能替代本次产物证据。

## 11. 完成记录

记录机型和版本、固件路径/大小/哈希、profile/patch/Git revision、bin 文件名/大小/哈希、resource/hash 摘要、构建与测试命令/退出码、每个真机检查的结果、日志、恢复动作、已知限制和未测试路径。

已构建验证只表示静态 gate 通过；真机确认必须绑定实际设备和精确 bin 哈希。没有逐项证据时保持较低验证等级。

## 12. 失败与恢复

- ABI 缺口：停止 profile，回到分析；
- 镜像哈希错误：找到正确镜像，不绕过 gate；
- patch 冲突：修 patch，不改 out/target-src；
- ELF 超限：减少真实资源，不任意放宽限制；
- 部署部分完成：保存当前哈希，修环境后重跑构建；
- 旧 module 驻留：清理环境并重启；
- registry 部分改写：停止重复操作，备份并比较两个 JSON；
- native callback 已注册：不得 live unload，必须重启。

不要用 git reset、git checkout 或全目录清理来恢复适配现场；先保存日志、profile、产物和工作树状态。

## 13. 禁止清单

- 无完整 ABI 就发布 target；
- 复制 036 profile 只改版本号；
- 将 0x2c 分析地址直接写入 profile；
- 忽略 Thumb 位或混淆数据/函数地址；
- 只验证一个页面就声明 App 可用；
- 在 Lua 中增加近似版本 fallback；
- 修改共享源码修复只属于新 target 的问题；
- 用 target-src 手工修改代替 patch；
- 只更新 editing tree；
- 把 host test、构建成功或旧日志当作真机证据；
- 未备份 registry 就执行破坏性应用操作；
- 未确认 reverse ABI 就卸载 module。
