# 当前目标与产物基线

本页固定当前支持固件、构建产物和证据边界。ABI 数值的机器可读权威来源是构建器 `targets/*.env`，本页不复制完整地址表。任何重新编译都会使这里记录的文件哈希成为历史基线；发布新产物时必须同步更新本页。

## 1. 支持状态

| 固件 | target ID | 固件代码 | 共享/专用实现 | 当前状态 |
| --- | --- | ---: | --- | --- |
| `3.101.036` | `xiaomi-band-10-pro-3.101.036` | `3101036` | 共享 nativeApp 基线，无 target patch | 已构建、已打包、用户确认可用 |
| `3.101.043` | `xiaomi-band-10-pro-3.101.043` | `3101043` | 共享基线加 7 个隔离 target patch | 已构建、已打包、用户确认可用 |

这里的“用户确认可用”是当前项目状态结论，表示相应固件上的实际安装和主要使用路径已经通过用户实验。它不是一份逐测试项、逐时间戳的实验室认证记录；没有单独记录的破坏性边界、异常注入和长期运行场景仍应按[验证矩阵](validation.md)重新测试。

## 2. 共同构建基线

最近一次全目标构建使用：

```sh
cd /Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build
./build.sh
```

构建退出码为 `0`。同一次执行完成 profile 校验、固件镜像身份校验、两个目标编译和链接、Shell++ 专用 ELF 验证、双目录部署、`resource.bin` 重打包与 `hashCode` 重算。

| 项目 | 3.101.036 | 3.101.043 |
| --- | ---: | ---: |
| bin 文件 | `shellpp_ii-3.101.036.bin` | `shellpp_ii-3.101.043.bin` |
| 文件大小 | `62040` B | `62620` B |
| SHF_ALLOC footprint | `60644` B | `60276` B |
| `.bss` | `24492 / 24576` B | `23872 / 24576` B |
| `module_initialize` | `0x000001b5` | `0x000001b5` |
| 定义符号数 | `556` | `562` |
| 直接 ABI literal 数 | `38` | `37` |
| relocation 类型 | `2, 38, 42` | `2, 38, 42` |
| SHA-256 | `0e91cd93125a5f4c992a41bc0f46e73d070af0149e9079a83076317ff915fb6e` | `c86f0abf002be8957a4ca026e91f189db0dda5b7b2078c5416a83bff3b926ad6` |

两个 bin 内容和大小不同，证明 target 处理确实生成了不同模块；差异本身不证明 ABI 正确，ABI 正确性仍来自固定镜像证据和真机行为。

## 3. 3.101.036 基线

固件镜像：

```text
/Users/ikun_cxkpro/Projects/固件修改/10p/3.101.036/
  miwear.watch.p67tc_v3.101.036_full_17a1ea48/vela_ap.bin
```

| 属性 | 值 |
| --- | --- |
| 镜像大小 | `13616208` B |
| 镜像 SHA-256 | `662d67f5e247e31e194d3161024890ba93b9d29d70b290fadb9aac8ce8ec3c81` |
| profile | `targets/xiaomi-band-10-pro-3.101.036.env` |
| target patch | 无 |
| native 页面 | 8 页 |

036 是 canonical C/汇编实现的行为基线。构建器直接复制共享源码到该目标的临时工作区，不对源码应用补丁。维护上的硬约束是：针对其他固件的修正不得改变 036 的运行代码；如果行为差异无法由 profile 表达，必须给新固件建立 target 专用通道。

036 应用管理仍属于 page 1 “文件与应用管理”的内部模式；其文件路径和页面形状不应被 043 的独立应用管理页反向覆盖。

## 4. 3.101.043 目标

固件镜像：

```text
/Users/ikun_cxkpro/Projects/固件修改/10p/3.101.043/
  miwear.watch.p67tc_v3.101.043_full_a4ce8564/vela_ap.bin
```

| 属性 | 值 |
| --- | --- |
| 镜像大小 | `13795728` B |
| 镜像 SHA-256 | `519307675665e4866d722a8119a98589c397b614ac3294cb87bfc86de45756ec` |
| profile | `targets/xiaomi-band-10-pro-3.101.043.env` |
| native 页面 | 9 页，page 8 为 `shellpp-apps` |
| 应用注册表 | `/data/apps.json`、`/data/apps.json_hide` |

043 在临时 `target-src` 中按字典序应用：

1. `0001-disable-legacy-misans-style.patch`：禁用该固件不兼容的旧 MiSans style 调用。
2. `0002-reduce-filesystem-stack.patch`：把目录分页和 CPU/内存读取的大对象移出短栈帧。
3. `0003-reduce-interactive-stack.patch`：压缩页面渲染、应用 JSON 与交互工作区的栈占用。
4. `0004-eliminate-delete-recursion.patch`：目录统计和删除改为固定深度显式游标栈，避免递归调用帧累计。
5. `0005-prefer-standard-memory-fields.patch`：标准内存字段成功时不再被通用数字回退覆盖。
6. `0006-separate-app-manager-page.patch`：注册第九页并让应用管理从 page 1 导航到独立页面。
7. `0007-match-shell-plus-plus-lua-app-paths.patch`：按 Shell++ Lua 的 10 Pro 路径语义读取注册表并处理应用数据目录。

这些补丁只应用于构建输出目录中的临时源码，不修改共享 nativeApp 文件。043 当前行为包括：

- 文件管理保留 page 1 标题“文件与应用管理”；
- 点击应用管理导航到 page 8，标题为“应用管理”；
- 直接打开 `/data/apps.json` 和 `/data/apps.json_hide`，不会先枚举一个假定的父目录；
- 注册表缺失、为空、无效或没有 `InstalledApps` 时归一化为空列表；
- 应用大小和卸载范围使用 `/data/app`、`/data/quickapp/system`、`/data/cache`、`/data/files`、`/data/mass`；
- 页面绑定按当前前台页重建，以在固定 BSS 上限内支持第九页；
- 监控使用已按真实 XIP 映射恢复的 top-layer 与 timer 入口。

## 5. 安装包基线

| 产物 | SHA-256 |
| --- | --- |
| `main.lua` payload | `18776971d8aa0cf6e8245f2afcb63541ef5a84949fa3ae892eac24a26f4da303` |
| `shellpp_ii-3.101.036.bin` | `0e91cd93125a5f4c992a41bc0f46e73d070af0149e9079a83076317ff915fb6e` |
| `shellpp_ii-3.101.043.bin` | `c86f0abf002be8957a4ca026e91f189db0dda5b7b2078c5416a83bff3b926ad6` |
| `shellpp_ii_icon.bin` | `9796563b8e4396c3f8dd22dac37a6453b099cc7cdaad937bcf2f6e3ea66ff6bb` |
| `resource.bin` | `f4303a207f4c93eba71fd715b86cc34bff6e561c1e759e6831f356801301d641` |
| `hashCode` | `6ba87073d4d5c8c99340af0d6e149ab6dfffaa4ea65dfd3627af45d1eeada0b4` |

构建输出、安装器 `_Lua`、安装器 `resources/_lua/_Lua` 和反向解析的 `resource.bin` 中四个 payload 在记录本基线时逐字节一致。设备实际接收的是由 packaged tree 生成的 `resource.bin`，因此只比较顶层 `_Lua` 不能形成打包证据。

## 6. 状态结论的使用规则

- “用户确认可用”可以用于描述当前 036/043 整体项目状态。
- 涉及某个具体写操作、故障注入或长时间运行时，只能引用有对应实验记录的结论；没有记录的场景标为“未单独验证”。
- 历史崩溃日志说明曾经发生过什么，不否定当前产物，也不能证明当前产物仍存在同一问题。
- profile、源码、补丁、工具链、链接器、Lua 或资源包任一变化后，必须生成新哈希并重新界定验证范围。
- 设备内已驻留旧 module 时，覆盖安装包不会替换 RAM 中的 App/Page callback。测试新产物前必须重启。

历史故障的原因与辨别方法统一保存在[故障排查](troubleshooting.md)，不作为当前支持状态。
