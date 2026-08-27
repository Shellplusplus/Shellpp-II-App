# 仓库结构与文件所有权

## 1. 范围

本篇列出三个仓库中与当前实现有关的文件，并区分手工维护、target 配置、临时生成、构建输出和 legacy 文件。判断一个源码是否影响 bin 时，以构建器 `build.sh` 的实际输入列表为准。

## 2. nativeApp 仓库

根目录：

```text
/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii
```

### 2.1 顶层结构

```text
Shellpp-ii/
├── README.md
├── assets/
│   └── shellpp-ii-icon.png
├── docs/
│   ├── README.md
│   ├── *.md
│   └── reference/*.md
└── module/
    ├── include/
    │   ├── shellpp_firmware_abi.h
    │   ├── shellpp_ii_module.h
    │   ├── shellpp_native_app.h
    │   ├── shellpp_native_fs.h
    │   └── shellpp_native_ui.h
    └── src/
        ├── module_prelude.S
        ├── supervisor.c
        ├── native_app.c
        ├── native_fs.c
        ├── native_ui.c
        ├── module.c
        └── app_payload.c
```

### 2.2 当前真实编译输入

构建器只编译以下五个文件：

| 文件 | 职责 |
| --- | --- |
| `module/src/module_prelude.S` | 提供 NuttX module prelude 并保留 `module_initialize` 引用 |
| `module/src/supervisor.c` | module 入口、constructor、`/dev/shellpp`、状态/控制协议、卸载保护 |
| `module/src/native_app.c` | App/Page descriptor、registry、Launcher、通知、安装状态 |
| `module/src/native_fs.c` | 固件文件 ABI 包装、路径、分页、读写、缓存、CPU/内存、应用目录 |
| `module/src/native_ui.c` | 页面生命周期、UI 渲染、文件浏览、应用管理、监控、缓存与重启 |

新增 `.c` 文件不会自动进入构建。必须显式更新 `build.sh` 的 compile 和 link 列表，再同步更新本篇、构建文档和验证契约。

### 2.3 稳定头文件

| 文件 | 职责和约束 |
| --- | --- |
| `shellpp_firmware_abi.h` | 只包含生成的 `shellpp_target_abi.h` 并要求 `SHELLPP_TARGET_ABI_GENERATED`；不得添加默认固件地址 |
| `shellpp_ii_module.h` | loader 提供的 module info 结构和入口声明 |
| `shellpp_native_app.h` | 分阶段注册、通知、状态、unload/uninstall 接口 |
| `shellpp_native_fs.h` | 路径/文件限制、结果码、数据结构和文件系统公共 API |
| `shellpp_native_ui.h` | reset/create/resume/pause/destroy 生命周期接口 |

### 2.4 Legacy 文件

| 文件 | 当前状态 |
| --- | --- |
| `module/src/module.c` | 旧 module 实现；不编译、不链接 |
| `module/src/app_payload.c` | 旧 App payload；包含历史地址或注册逻辑，不编译、不链接 |

这两个文件不能作为当前 App ID、ABI 地址、页面结构、Supervisor 协议或运行行为的证据。修改它们不会改变当前 bin。自动化代理在阅读 symbol 或搜索绝对地址时必须排除这两个 legacy 来源，除非任务明确是清理历史代码。

### 2.5 资产和文档

- `assets/shellpp-ii-icon.png` 是源图像资产；当前 `build.sh` 不调用图标转换工具，也不读取它。
- 当前安装器使用既有 `shellpp_ii_icon.bin`。若需重生成图标，必须单独确认格式、尺寸、两个资源目录和最终 `resource.bin`，不能假定修改 PNG 会影响安装包。
- 所有跨仓库技术文档统一在本 `docs/`。其他 README 只保留入口链接和最小说明。

## 3. 构建器仓库

根目录：

```text
/Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
```

### 3.1 手工维护文件

| 路径 | 所有者 | 职责 |
| --- | --- | --- |
| `build.sh` | 构建系统 | 选择目标、校验工具/输入、编译、链接、验证、部署和打包 |
| `targets/*.env` | firmware target | 固件身份、ABI 数值、编译参数和尺寸限制的规范来源 |
| `targets/<TARGET_ID>/patches/*.patch` | firmware target | 无法用 profile 表达的目标专用语义差异 |
| `generate_target_abi.py` | ABI schema | 严格解析 profile 并生成 C 头 |
| `verify_shellpp_elf.py` | module gate | 验证 ET_REL、sections、symbols、relocations、地址和尺寸 |
| `repack_resource.py` | installer packaging | 从 packaged tree 重建文件 record、`resource.bin` 和 `hashCode` |
| `shellpp_ii.ld` | linker | 合并 module sections，形成 relocatable module |
| `tests/*.c`、`tests/*.sh` | host verification | 构建 043 补丁源码和 mock 固件 ABI 回归测试 |
| `tools/firmware_xrefs.py` | analysis support | 固件交叉引用分析辅助；不是构建主流程 |

`verify_elf.py`、`make_icon_bin.js` 等旧工具不属于当前 `build.sh` 主流程。是否存在于仓库不代表它们具有发布权威性。

### 3.2 Target 结构

```text
targets/
├── xiaomi-band-10-pro-3.101.036.env
├── xiaomi-band-10-pro-3.101.043.env
└── xiaomi-band-10-pro-3.101.043/
    └── patches/
        ├── 0001-disable-legacy-misans-style.patch
        ├── 0002-reduce-filesystem-stack.patch
        ├── 0003-reduce-interactive-stack.patch
        ├── 0004-eliminate-delete-recursion.patch
        ├── 0005-prefer-standard-memory-fields.patch
        ├── 0006-separate-app-manager-page.patch
        └── 0007-match-shell-plus-plus-lua-app-paths.patch
```

036 没有补丁目录，因此直接编译共享源码。不要为 043 问题修改 036 profile、创建 036 补丁或在共享源码加入版本判断，除非有独立的 036 需求和重新真机验证授权。

### 3.3 生成目录

```text
out/<TARGET_ID>/
├── generated/shellpp_target_abi.h
├── target-src/                 # 仅目标存在补丁时生成
├── module_prelude.o
├── supervisor.o
├── native_app.o
├── native_fs.o
├── native_ui.o
└── shellpp_ii-<version>.bin
```

以下内容均可重建，不应手工编辑：

- `out/<target>/generated/shellpp_target_abi.h`；
- `out/<target>/target-src/*`；
- `out/<target>/*.o`；
- `out/<target>/shellpp_ii-*.bin`；
- `out/.installer-stage*`；
- `tests/.build*`。

若临时源码看起来正确而 patch 文件错误，应修 patch 并从共享源码重新生成，不能只改 `target-src`。

## 4. Lua 安装器仓库

根目录和固定构建输出根：

```text
/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer
```

### 4.1 资源树

```text
Shellpp-ii-installer/
├── _Lua/
│   ├── main.lua
│   ├── shellpp_ii_icon.bin
│   ├── shellpp_ii-3.101.036.bin
│   └── shellpp_ii-3.101.043.bin
├── resources/_lua/_Lua/
│   ├── main.lua
│   ├── shellpp_ii_icon.bin
│   ├── shellpp_ii-3.101.036.bin
│   └── shellpp_ii-3.101.043.bin
├── capability.json
├── resources/manifest.xml
├── uidmap.map
├── resource.bin
└── hashCode
```

### 4.2 所有权

| 路径 | 修改者 | 说明 |
| --- | --- | --- |
| `_Lua/main.lua` | 安装器实现 | 手工维护的唯一 Lua 入口 |
| `resources/_lua/_Lua/main.lua` | 安装器资源 | 必须与 editing tree 逐字节一致；构建器校验但不覆盖 |
| 两处 `shellpp_ii_icon.bin` | 安装器资源流程 | 必须逐字节一致；构建器校验但不生成 |
| 两处 `shellpp_ii-*.bin` | 构建器 | 由选定 target 成功后部署 |
| `resource.bin` | 构建器重打包器 | 从 packaged tree 的 File records 重建 |
| `hashCode` | 构建器重打包器 | capability、manifest、resource 三个 SHA-256 的逗号连接 |
| manifest、capability、`uidmap.map` | 安装器工程 | 构建器读取或校验，不应无关改写 |

真正进入 `resource.bin` 的源是 `resources/_lua/_Lua`，不是顶层 `_Lua`。只更新 editing tree 会造成界面上看到新文件但安装包仍携带旧资源。

### 4.3 全目标与单目标部署

无参数全目标模式：

- 构建全部 `targets/*.env`；
- 所有目标成功后，删除两个资源目录内构建器管理的旧 `shellpp_ii-*.bin` 和旧名 `shellpp_ii.bin`；
- 写入完整 staged bin 集；
- 重建 `resource.bin` 与 `hashCode`。

`--target` 单目标模式：

- 只构建和替换指定 target bin；
- 保留两个资源目录内其他版本 bin；
- 仍重建 `resource.bin` 与 `hashCode`。

不要通过手工复制一个 bin 后忘记重打包来模拟单目标部署。

## 5. 固件资料目录

当前已知输入：

| 固件 | 资料路径 |
| --- | --- |
| 3.101.036 固件 | `/Users/ikun_cxkpro/Projects/固件修改/10p/3.101.036` |
| 3.101.036 完整 ABI | `/Users/ikun_cxkpro/Projects/固件修改/10p/3.101.036_ABI` |
| 3.101.043 固件 | `/Users/ikun_cxkpro/Projects/固件修改/10p/3.101.043` |
| 分析脚本和其他 10 Pro 资料 | `/Users/ikun_cxkpro/Projects/固件修改/10p/` |

具体 `vela_ap.bin` 路径、大小和 SHA-256 以 target profile 为准。目录名称相同不构成固件身份。

## 6. 手工文件与生成文件矩阵

| 类型 | 示例 | 正确修改入口 | 禁止操作 |
| --- | --- | --- | --- |
| 共享源码 | `module/src/native_app.c` | 修改 canonical 源并全目标构建 | 只改某个 `out/target-src` |
| ABI 配置 | `targets/*.env` | 修改经证据确认的 profile | 直接编辑生成头 |
| target 语义差异 | `targets/<id>/patches/*.patch` | 从共享源码制作最小补丁 | 复制完整源码树成为第二实现 |
| 生成 ABI 头 | `out/.../shellpp_target_abi.h` | 运行生成器或构建器 | 手工保存修改 |
| module bin | `out/.../*.bin` | 运行 `build.sh` | 二进制 patch 后直接发布 |
| 安装器 bin 副本 | 两个 Lua 目录 | 由构建器部署 | 只更新其中一个目录 |
| 安装包 | `resource.bin`、`hashCode` | 由重打包器生成 | 手工拼接或只更新 hashCode |
| Lua | `_Lua/main.lua` | 手工修改、同步 packaged 副本、语法验证、重打包 | 为每个固件复制一个 Lua |
| 文档 | 本 `docs/` | 随实现同步更新 | 在其他仓库维护冲突版本 |

## 7. 大小写和近似路径陷阱

当前环境曾同时出现大小写近似名称，例如 `Shellpp-ii` 与 `shellpp-ii`。macOS 文件系统是否区分大小写取决于卷配置；Git 和构建脚本仍可能把它们视为不同文本路径。

维护规则：

- nativeApp 的构建器路径以 `build.sh` 当前值为准；
- 构建器目录当前是小写 `shellpp-ii-build`；
- 安装器固定输出根是大写开头的 `Shellpp-ii-installer`；
- 命令和文档使用绝对路径；
- 迁移时用 `pwd -P`、`git rev-parse --show-toplevel` 和 profile 中的镜像路径分别确认；
- 不根据 Finder 显示或 shell tab completion 推断正在修改的仓库。

## 8. 脏工作树规则

三个仓库可能同时存在未提交的用户修改。执行文档或适配工作时：

- 先记录 `git status --short`；
- 不执行 `git reset --hard`、`git checkout --` 或全目录清理；
- 不回退与当前任务无关的修改；
- 修改重叠文件前先阅读完整 diff；
- 构建输出变化不表示源修改可以被回滚；
- 发布记录应说明使用的工作树状态或 commit，而不是假定 clean tree。
