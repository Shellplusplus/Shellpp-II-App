# 系统概览

## 1. 范围和前置条件

本篇说明 Shell++ II 从固件 ABI 到最终设备运行的整体结构。阅读前应先接受一个硬性边界：新增同机型固件之前必须具备该固件的完整 ABI；仅有固件镜像、旧版本 ABI 或一组候选地址不足以进入 target 发布流程。

## 2. 三层架构

| 层 | 稳定输入 | 主要输出 | 不承担的职责 |
| --- | --- | --- | --- |
| nativeApp | 生成的 target ABI 头、共享 C/汇编、target 临时补丁结果 | ARM ET_REL module 对象 | 固件版本检测、profile 维护、资源打包 |
| 构建器 | 固件镜像、profile、共享源码、target 补丁、安装器资源树 | 每固件一个 bin、同步资源目录、`resource.bin`、`hashCode` | 设备运行时版本检测、Lua UI、ABI 逆向恢复 |
| Lua 安装器 | `ro.build.version`、同目录版本化 bin、图标、Supervisor 状态 | 正确 module 的加载和分阶段 App 安装 | C 编译、固件地址维护、自动选择近似固件 |

架构的核心不变量是：**一套长期维护的 nativeApp 共享源码，一个 Lua 入口，每个精确固件版本一个由构建器生成的 bin。**

## 3. 变化如何表达

固件差异分为两类：

1. 地址、常量、枚举、描述符尺寸和资源限制写入 `targets/<TARGET_ID>.env`，由生成器变成 C 宏。
2. 无法由纯数值 profile 表达的已证明语义差异，写入 `targets/<TARGET_ID>/patches/*.patch`，只应用到该目标的临时源码副本。

036 直接编译共享源码。043 将同一共享源码复制到 `out/xiaomi-band-10-pro-3.101.043/target-src`，按文件名顺序应用七个补丁后编译。这个机制保证 043 修复不需要修改 036 专用代码，因为 036 没有专用源码副本或补丁通道。

target 补丁不是第二套源码。canonical 源仍在 nativeApp；补丁必须保持最小、可重放、可审计，并在共享源码变化后重新验证能否干净应用。

## 4. 构建时数据流

```text
固定的 vela_ap.bin
       │  路径、大小、SHA-256
       ▼
targets/<TARGET_ID>.env
       │  严格 schema 与形式检查
       ▼
generate_target_abi.py
       │
       ▼
out/<target>/generated/shellpp_target_abi.h
       │
       ├───────────────┐
       │               │
共享 module/src       target patches（可选）
       │               │
       └──────┬────────┘
              ▼
    实际 target 编译源码
              │ clang --target=arm-none-eabi
              ▼
        五个目标对象文件
              │ rust-lld -r
              ▼
 shellpp_ii-<firmware>.bin
              │ verify_shellpp_elf.py
              ▼
        deployment stage
              │ 所有所选 target 均成功
              ▼
 installer/_Lua + resources/_lua/_Lua
              │ repack_resource.py
              ▼
       resource.bin + hashCode
```

每个 target 都有自己的生成头、对象和输出目录。生成头通过 include path 注入，不复制进 nativeApp。任一所选目标在 profile、镜像、编译、链接或 ELF gate 阶段失败，部署阶段不会启动。

## 5. 运行时数据流

```text
安装器读取 ro.build.version
             │ 精确 major.minor.patch
             ▼
 SCRIPT_PATH/shellpp_ii-<version>.bin
             │ Lua ELF32/ET_REL/ARM/大小预检
             ▼
      insmod <bin> shellpp_ii
             │ constructor
             ▼
 Supervisor 注册 /dev/shellpp
             │ 状态 magic / ABI / firmware code / driver flag
             ▼
    LuaLVGL timer 分五次发送命令
             │
             ├─ notify loaded
             ├─ restore
             ├─ install stage 0
             ├─ install stage 1: App + Page registry
             └─ install stage 2: Launcher publication
                         │
                         ▼
             Launcher 打开 native App
```

每个命令在独立 LuaLVGL timer callback 中执行，使固件事件循环在注册阶段之间获得一次运行机会。C 端 `write()` 同步完成命令，Lua 随后读取状态确认 command 和 result。

## 6. 稳定跨仓库契约

| 契约 | 当前值 |
| --- | --- |
| bin 命名 | `shellpp_ii-<major.minor.patch>.bin` |
| 固件代码 | `major * 1,000,000 + minor * 1,000 + patch` |
| module loader 名称 | `shellpp_ii` |
| module 入口符号 | `module_initialize` |
| Supervisor 设备 | `/dev/shellpp` |
| 状态长度 | 384 字节 |
| 状态 ABI | 3 |
| 控制写入 | 四个小端 `uint32_t`，16 字节 |
| native App ID | `0x00cd` |
| 包名 | `com.shellpp.ii` |
| 显示名 | `Shell++ II` |
| 图标资源名 | `shellpp_ii_icon.bin` |
| 图标设备路径 | `/data/shellpp-ii/shellpp_ii_icon.bin` |

更改任一跨仓库契约都必须同时审查 C、Lua、构建部署、安装包和文档，不能只改一端。

## 7. Supervisor 与 native App 边界

Supervisor 的职责是：

- module constructor 中注册只含 open/close/read/write 前缀的驱动；
- 暴露稳定状态快照；
- 校验控制命令；
- 调用通知、App 注册和 Launcher 发布；
- 在固件仍保存 native 回调时拒绝卸载。

native App 子系统的职责是：

- 用静态存储创建 App/Page descriptor；
- 处理 App ID 冲突和异步 registry publication；
- 将 page lifecycle callback 转发给 UI；
- 在 App 存在并且包名匹配后发布 Launcher；
- 只提交一次加载通知。

Supervisor 不拥有页面对象，Lua 也不直接调用固件 App registry ABI。

## 8. 页面模型

共享基线注册八页：

| index | 内部名称 | 主要用途 |
| ---: | --- | --- |
| 0 | `shellpp-home` | 首页 |
| 1 | `shellpp-files` | 文件与应用管理入口 |
| 2 | `shellpp-viewer` | 文件浏览和查看 |
| 3 | `shellpp-cache` | 缓存统计和清理 |
| 4 | `shellpp-about` | 版本和固件信息 |
| 5 | `shellpp-display` | CPU/内存入口 |
| 6 | `shellpp-cpu` | CPU 或内存监控页面 |
| 7 | `shellpp-restart` | 系统重载 |

043 的专用补丁增加 index 8 `shellpp-apps`，应用管理通过 Activity 导航打开独立页面。036 保持共享八页行为，不接受为 043 页面修复而产生的共享源码修改。

每个页面 key 为：

```text
(app_id << 16) | page_index
```

UI 支持 create、resume、pause 和 destroy。事件 cookie 携带 generation、page 和 row slot，销毁或重建后产生的旧 callback 会因为 generation 不匹配被忽略。

## 9. 文件与应用数据边界

文件浏览从 `/` 开始，路径必须是绝对规范路径，不允许空组件、`.`、`..` 或超长组件。普通文件可以查看、复制、移动和删除；符号链接不跟随，只允许作为链接本身删除。目录递归操作有固定深度上限。

043 应用管理读取：

- `/data/apps.json`
- `/data/apps.json_hide`

它与 Shell++ Lua 的 10 Pro 逻辑一致：直接打开文件，不以父目录枚举作为存在性前置；缺失、空、无效或缺少有效 `InstalledApps` 的注册表在列表读取阶段归一化为空列表。写操作仍要求可解析、可原子改写的结构，不能把宽松读取误解为宽松写入。

应用大小和删除目录必须按 target 的实际实现审查，详见[应用管理](application-management.md)。

## 10. 生命周期和驻留约束

stage 1 后，固件 App/Page registry 会保存以下 module 内地址：

- App descriptor 和 Page descriptor；
- 包名、图标和页面名称字符串；
- display-name callback；
- signal/create/resume/pause/destroy callback；
- UI 事件和定时器回调间接到达的 module text。

因此只注销 `/dev/shellpp` driver 并不能安全卸载 module。当前行为是：

- 注册前可以由 uninitializer 注销 driver；
- 注册后 uninitializer 返回 `-16`；
- native uninstall 返回 `-95`；
- 安装器删除 `/data/shellpp-ii` 后要求重启；
- 重启负责清除 RAM 中的 module 和 registry 状态。

在逆向注销 ABI、回调失效顺序和 Launcher 移除流程全部确认以前，不得实现 live unload。

## 11. 资源与栈约束

两个当前 target 的 profile 都限制：

- loaded SHF_ALLOC footprint 小于 65,536 字节；
- BSS 小于 24,576 字节。

043 固件的 UI event 路径表现出更小的有效栈余量，因此其补丁将大数组迁移到可证明串行使用的静态工作区，并将递归遍历改为固定容量显式游标栈。这种复用依赖 LVGL 事件串行、无重入和明确所有权，不能作为通用优化无条件合并到共享基线。

## 12. 验证边界

系统需要四类互补证据：

1. profile 和固件镜像身份验证；
2. 编译、链接和静态 ELF 验证；
3. host mock 逻辑测试与安装包反向核对；
4. 指定固件、指定 bin 哈希上的真机验证。

静态 ELF 验证可拒绝格式、section、relocation、undefined symbol、地址白名单和尺寸错误，但不能证明函数原型、dirent 布局、栈余量、异步时序或 LVGL 生命周期正确。

## 13. 失败隔离

- profile 或 ABI 错误在生成头以前失败；
- 固件镜像身份错误在编译以前失败；
- target patch 冲突只使对应构建失败；
- 任一所选 target 失败时不部署任何 staged bin；
- 单 target 模式保留其他固件 bin；
- 全目标模式替换构建器管理的完整版本化 bin 集；
- resource 重打包失败后应修复原因并重复同一构建，不手工拼接 `resource.bin`；
- 设备加载或注册失败后必须重启，不在同一运行期替换驻留 module。
