# 构建系统

## 1. 范围

本篇说明 `/Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build/build.sh` 的当前行为，包括工具链、profile 解析、固件身份校验、target 补丁、编译链接、ELF gate、部署、资源重打包和失败恢复。

构建器是新增固件的正常修改入口。固件版本命名、Lua/Supervisor 协议和共享功能不变时，新 target 应只增加构建器 profile，以及确有语义差异时增加最小 target patch。

## 2. 固定路径

当前 `build.sh` 显式配置：

| 角色 | 路径 |
| --- | --- |
| 构建器 | `/Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build` |
| nativeApp | `/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii` |
| 安装器输出根 | `/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer` |
| editing Lua tree | `<installer>/_Lua` |
| packaged Lua tree | `<installer>/resources/_lua/_Lua` |

安装器输出根必须保持为上述 `Shellpp-ii-installer`。移动仓库时要修改并审查 `build.sh`，不要通过大小写近似路径、当前 shell 目录或未记录符号链接掩盖配置错误。

## 3. 工具链

默认依赖：

| 工具 | 默认位置/来源 |
| --- | --- |
| POSIX shell | 脚本 shebang `/bin/sh` |
| Clang | `/usr/bin/clang` |
| rust-lld | stable x86_64 Apple Rust toolchain 内的 `rust-lld` |
| Python 3 | `/usr/local/bin/python3` |
| patch | 系统 `patch` |
| hash | `shasum -a 256` |
| 基础文件工具 | `find`、`cp`、`cmp`、`wc`、`awk`、`tr`、`mkdir`、`rm` |

可覆盖环境变量：`CLANG`、`RUST_LLD`、`RUST_LLD_LIBRARY_PATH`、`PYTHON`。脚本会检查 compiler、linker、Python 是否可执行，并在使用默认 Rust linker 时设置对应 `DYLD_LIBRARY_PATH`。

当前主流程不调用 Node.js、`sips`、`make_icon_bin.js` 或旧 `verify_elf.py`。仓库中存在旧工具不表示它们参与发布。

## 4. 命令行

全目标构建：

```sh
cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
./build.sh
```

无参数时遍历 `targets/*.env`。构建顺序由 shell glob 文件名顺序决定。

单目标构建：

```sh
./build.sh --target xiaomi-band-10-pro-3.101.043
```

`--target` 只接受字母、数字、点、下划线和连字符组成的 ID，并要求同名 `targets/<id>.env` 存在。未知参数、缺失 ID 或非法 ID 以退出码 2 失败。

单目标模式适用于已隔离的目标专用迭代。共享源码、profile schema、生成器、验证器、链接脚本、部署或打包逻辑发生变化时必须执行全目标构建。

## 5. 构建前输入 gate

脚本在读取 profile 前检查：

- nativeApp `module/src` 和 `module/include` 存在；
- 构建器 `targets` 存在；
- 两个安装器 Lua tree 均存在；
- 两处均含 `main.lua` 和 `shellpp_ii_icon.bin`；
- `repack_resource.py` 存在；
- 两处 `main.lua` 逐字节一致；
- 两处图标逐字节一致；
- 至少选中一个 profile。

共享资源不一致时脚本失败，不任选一份覆盖另一份。这样能防止 editing tree 和实际 packaged tree 的修改来源被隐藏。

## 6. profile 严格解析

对每个 target，构建器先执行：

```text
generate_target_abi.py <profile> <staged-header>
```

只有生成器成功后 shell 才用 `.` source profile。生成器的顺序很重要：它在 shell 解析以前拒绝危险或不完整内容。

生成器要求：

- 每个有效行是无引号 `KEY=VALUE` 或注释；
- key 以大写字母开头，只含大写字母、数字、下划线；
- value 非空，不含 whitespace 或 shell meta characters；
- 不允许重复、未知或缺失 key；
- target ID 只含小写字母、数字、点和连字符；
- firmware version 完整匹配 `major.minor.patch`；
- firmware code 与公式一致；
- image SHA-256 是 64 位小写十六进制；
- 数值在 uint32 范围；
- 函数地址非零且为奇数 Thumb 地址；
- MiSans style data address 字对齐；
- descriptor size 非零且为四字节倍数。

生成器通过只证明 schema 和形式约束，不证明 ABI 地址身份、原型或结构正确。

## 7. 固件镜像身份 gate

profile source 后，`build.sh` 要求 `FIRMWARE_IMAGE` 存在，并逐项比较：

```text
actual byte count == FIRMWARE_IMAGE_SIZE
actual SHA-256    == FIRMWARE_IMAGE_SHA256
```

任一不匹配立即失败。不能因为版本目录或文件名相同而继续使用旧 ABI。镜像大小和 SHA-256 是 build gate，不是文档备注。

## 8. 生成 ABI 头

通过验证的 staged header 被复制到：

```text
out/<TARGET_ID>/generated/shellpp_target_abi.h
```

复制后 `cmp -s`。生成头包含 target ID、firmware version、firmware code、全部 ABI address/constant macro 和 `SHELLPP_TARGET_ABI_GENERATED`。

生成目录位于 compiler include path 前部。不得直接编辑生成头；下一次构建会覆盖它，且不会更新 profile。

## 9. 实际源码选择

默认 `TARGET_SOURCE_DIR` 指向 nativeApp `module/src`。如果存在：

```text
targets/<TARGET_ID>/patches/
```

脚本则：

1. 删除旧 `out/<target>/target-src`；
2. 新建目录；
3. 复制 nativeApp `src/*.c` 与 `src/*.S`；
4. 按文件名顺序对临时副本执行 `patch -s -p1`；
5. 输出应用补丁数量；
6. 编译临时副本。

只有以下五个文件被编译：

- `module_prelude.S`；
- `supervisor.c`；
- `native_app.c`；
- `native_fs.c`；
- `native_ui.c`。

复制全部 `src` 文件是为了 patch 上下文和可检查性，不表示 `module.c` 或 `app_payload.c` 进入 link。真正输入仍由明确的 compile/link 调用决定。

## 10. Target patch 规则

target patch 只用于 profile 数值无法表达的、有证据支持的固件语义差异。

必须满足：

- patch 路径相对 nativeApp `module/src`；
- 使用 `a/<file>`、`b/<file>`，由 `-p1` 应用；
- 文件名使用零填充序号，顺序有意义；
- 从当前共享源码干净应用；
- 不修改 canonical nativeApp；
- 不包含 ABI 地址差异，地址仍进入 profile；
- 不复制完整源码树成为第二套实现；
- 注释说明证据、所有权、生命周期和验证边界；
- 失败时 target 构建失败，不手工修 `target-src`；
- 共享源码变化后重新验证全部 patch。

没有 patch 目录的 036 始终直接编译共享源码。043 修复不得通过修改 036 profile、创建 036 补丁或共享版本判断进入 036。

## 11. 当前 043 补丁链

| 顺序 | 文件 | 语义 |
| ---: | --- | --- |
| 1 | `0001-disable-legacy-misans-style.patch` | 043 不调用旧 MiSans style apply |
| 2 | `0002-reduce-filesystem-stack.patch` | 目录页、内存和应用路径复用 target scratch |
| 3 | `0003-reduce-interactive-stack.patch` | JSON、row spec、CPU/内存和遍历降低栈；部分递归改显式帧 |
| 4 | `0004-eliminate-delete-recursion.patch` | 应用数据删除改固定深度显式 cursor stack |
| 5 | `0005-prefer-standard-memory-fields.patch` | 标准内存字段存在时不被通用三数字 fallback 覆盖 |
| 6 | `0006-separate-app-manager-page.patch` | 增加 page 8 和独立应用页面；只保留前台 binding 以满足 BSS |
| 7 | `0007-match-shell-plus-plus-lua-app-paths.patch` | 对齐 043 registry 直读、缺省归一化和五个应用数据 root |

这些补丁描述目标行为，不是旧故障记录。043 已由用户确认当前产物可用，但后续修改仍需重新绑定新的 bin 哈希和真机结果。

## 12. 编译参数

每个输入使用相同参数：

```text
--target=arm-none-eabi
-mcpu=<profile CPU>
-mthumb
-mfloat-abi=<profile FLOAT_ABI>
-Oz
-ffreestanding
-fno-builtin
-fno-common
-fno-stack-protector
-fno-unwind-tables
-fno-asynchronous-unwind-tables
-fno-exceptions
-fomit-frame-pointer
-mlong-calls
-Wall -Wextra -Werror
```

include 顺序为 target generated directory，再到 nativeApp headers。警告即错误。`-mlong-calls` 支持对固件绝对入口的远调用。

正式栈分析必须使用等价参数加 `-fstack-usage`，并按完整 callback 调用链分析；单个函数帧不包括固件 ABI 内部消耗。

## 13. 链接

`rust-lld` 以 GNU flavor、`armelf`、`-r` 运行，并使用 `shellpp_ii.ld`。`-u module_initialize` 强制入口保留。

link inputs 顺序：

1. `module_prelude.o`；
2. `supervisor.o`；
3. `native_app.o`；
4. `native_fs.o`；
5. `native_ui.o`。

输出：

```text
out/<TARGET_ID>/shellpp_ii-<FIRMWARE_VERSION>.bin
```

输出仍是 ET_REL module，不是 flat firmware binary。

## 14. 静态 ELF gate

`verify_shellpp_elf.py` 输入 generated ABI header、profile loaded/BSS limits 和 module。它验证：

- ELF32 little-endian、System V、ARM、ET_REL、EABI5；
- 无 program header、e_entry 为 0；
- 唯一的 defined function `module_initialize`；
- 无 undefined import；
- required section 和允许的 alloc section 集；
- 无 `.preinit_array`；
- REL 格式和允许 relocation type；
- relocation 不指向 undefined symbol 或非-alloc target；
- 直接出现的分析地址 `0x2c...` 被拒绝；
- 直接出现的 XIP function 和 RAM data 地址属于 generated whitelist；
- SHF_ALLOC footprint 严格小于 `MAX_LOADED_SIZE`；
- `.bss` 严格小于 `MAX_BSS_SIZE`。

校验器会扫描编译器标记的 text literal data ranges 和 `.data`。Clang 可能把相近 target 折成 base+immediate，因此“direct target ABI literals”不等于实际调用函数总数。白名单扫描是 fail-closed 的直接 literal gate，不是完整反汇编证明。

详细契约见[验证契约](reference/verification-contract.md)。

## 15. 部署 staging

通过 ELF gate 的 bin 先复制到：

- 全目标：`out/.installer-stage`；
- 单目标：`out/.installer-stage-<TARGET_ID>`。

每个复制立即 `cmp -s`。全部 selected profile 构建后，stage 中 `shellpp_ii-*.bin` 数量必须等于 selected profile 数量。只有此时才进入安装器部署。

因此编译/链接/验证失败不会部署部分 selected target。旧安装器仍保持上一次状态。

## 16. 全目标和单目标部署

### 16.1 全目标

对 editing 和 packaged tree：

1. 删除构建器管理的 `shellpp_ii-*.bin`；
2. 删除历史无版本名 `shellpp_ii.bin`；
3. 复制完整 staged bin 集。

全目标模式使安装器支持集合等于当前 `targets/*.env` 集合。

### 16.2 单目标

只复制本次 staged bin，不删除其他版本 bin。因此已有 036/043 文件应逐字节保持不变。单目标运行后仍重打包全部 packaged resources。

单目标模式不是共享变更的验证替代品。

## 17. 部署后比较

对每个 selected module，构建器比较：

```text
stage bin == installer/_Lua/bin
stage bin == installer/resources/_lua/_Lua/bin
```

随后再次确认两处 `main.lua` 和图标未在部署中改变。构建器不修改它们。

多文件复制不是跨文件事务。磁盘、权限或进程中断可能造成部分目录更新；修复环境后应重新运行相同构建命令，而不是手工推断哪份有效。

## 18. resource.bin 重打包

部署完成后执行：

```text
repack_resource.py --project /Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer
```

重打包器：

1. 解析既有 `resource.bin` magic、theme 和 record table；
2. 只处理 File record；
3. 验证 embedded name 安全且不能逃出 `resources/`；
4. 用 `uidmap.map` 验证 name/UID；
5. 从 packaged tree 读取每个 payload；
6. 要求 File payload 在 `resource.bin` 尾部连续且无 trailing data；
7. 重建地址/长度和 payload；
8. 反向比较每个重建 payload；
9. 用同目录临时文件和 `os.replace` 原子替换 `resource.bin`；
10. 计算 capability、manifest、resource 三个 SHA-256，并原子替换 `hashCode`。

`hashCode` 内容是三个摘要的逗号连接，不是其文本文件的自哈希。重打包器不修改 capability、manifest 或 `uidmap.map`。

## 19. 输出和日志

成功时每个 target 输出：

- module 路径；
- file size；
- SHF_ALLOC footprint；
- BSS；
- entry 和 `module_initialize` offset；
- defined symbol 数；
- direct ABI literal 数；
- relocation type 集；
- section 列表。

最终输出 deployed bin、`resource.bin` 和 `hashCode` 的 SHA-256。发布记录应保存完整命令、退出码和摘要，而不是只保存 `Built` 行。

## 20. 当前全目标命令

```sh
cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
./build.sh
```

当前一次成功全构建生成两个版本，精确结果见[当前目标](current-targets.md)。

## 21. 失败类别与恢复

| 阶段 | 常见失败 | 安装器是否已修改 | 恢复 |
| --- | --- | --- | --- |
| 输入 gate | 工具/目录/共享资源缺失或不一致 | 否 | 修复路径或明确同步资源 |
| profile | key、地址、版本、schema 错误 | 否 | 返回 ABI/profile 维护 |
| image | 大小或 SHA 不同 | 否 | 找到精确镜像，不绕过 gate |
| patch | patch 无法应用 | 否 | 更新 patch，不改 target-src |
| compile/link | warning、error、undefined | 否 | 修源码或 ABI，重新全构建 |
| ELF gate | section、relocation、address、limit | 否 | 修根因，不放宽 gate猜测通过 |
| stage count | 输出集合不完整 | 否 | 检查 profile/命名 |
| deployment | copy/cmp/磁盘失败 | 可能部分 | 修环境，重跑相同命令 |
| repack | resource format、uidmap、source 缺失 | bin 目录已更新 | 修 packed tree/metadata，重跑构建 |

不要用手工复制或手工编辑 `resource.bin` 跳过失败阶段。

## 22. 构建器不负责的内容

- 不恢复 ABI；
- 不验证函数原型和结构布局；
- 不将静态检查提升为真机验证；
- 不修改 Lua、图标、manifest、capability 或编辑器配置；
- 不清理三仓库其他用户修改；
- 不部署“最接近”的版本；
- 不实现设备回滚或 live unload。

## 23. 常见陷阱

- 在错误大小写路径运行另一份构建器；
- 只改 nativeApp 中的旧 target/profile 副本；
- profile 在生成器校验前含 shell syntax；
- 直接编辑 generated header；
- 修改 `target-src` 而不是 patch；
- 认为复制到 target-src 的所有 `.c` 都被链接；
- 用旧 `verify_elf.py` 替换专用 verifier；
- 放宽 BSS/loaded limit 只为让构建通过；
- 单目标构建用于验证共享源码变化；
- 全目标失败后手工部署其中一个成功 bin；
- 只更新 editing tree；
- 只看 `resource.bin` mtime 而不反向解析；
- 修改 `hashCode` 却未重建 resource；
- 忽略单文件 atomic 与多文件非事务的区别；
- 构建成功后不重启设备，继续使用旧驻留 module；
- 把不同 bin 哈希本身当作 ABI 正确证明。
