# native 模块架构

## 1. 范围

本篇说明最终 bin 的 module 格式、loader 入口、constructor、Supervisor 控制层和内部子系统边界。App/Page descriptor 的字段和注册时序见[native App 注册](native-app-registration.md)，字段级设备协议见[状态与控制 ABI](reference/status-control-abi.md)。

## 2. 实际构建单元

当前 module 由五个输入组成：

```text
module_prelude.S
supervisor.c
native_app.c
native_fs.c
native_ui.c
```

`module.c` 和 `app_payload.c` 是 legacy 文件，不参与编译或链接。解释当前行为时不得引用其中的地址、入口或注册逻辑。

## 3. ELF 和 loader 契约

构建产物是 ARM 32 位、小端、System V ABI、ARM EABI5、`ET_REL` ELF：

- 没有 program header；
- ELF `e_entry` 必须为 0；
- `module_initialize` 必须恰好定义一次并且是函数符号；
- 所有固件依赖以绝对地址宏进入 module，不能留下 undefined import；
- loader 需要的可分配 section 为 `.text`、`.rodata`、`.data`、`.bss`，并允许 `.init_array` 和 `.ARM.exidx`；
- 当前 target 接受 REL relocation，不接受 RELA；
- 链接由 `rust-lld -r` 完成，因此 bin 是可重定位 module，不是裸机镜像或普通可执行程序。

`module_prelude.S` 在标准 `.text` 开头放置一个最小 Thumb 函数，使 `module_initialize` 位于非零 Thumb offset。它不执行初始化业务。真正的 module 元数据由 `module_initialize()` 写入 loader 传入的 `shellpp_ii_mod_info`。

`shellpp_ii.ld` 将编译器产生的细分 section 归一化到 NuttX modlib 预期名称，保留 constructor 的 `.init_array`，丢弃不需要的 unwind extension、comment、LLVM address-significance 和 GNU stack note。

## 4. module 初始化顺序

运行顺序分为两部分：

1. loader 重定位 module，并执行 `.init_array` constructor；
2. loader 调用 `module_initialize()` 获取 uninitializer、参数和 exports。

Supervisor constructor `shellpp_supervisor_ctor()`：

1. 清零静态 `file_operations_prefix`；
2. 只填写 open、close、read、write 四个回调；
3. 调用 profile 指定的 `register_driver`，注册 `/dev/shellpp`，mode 为 `0666`；
4. 保存原始返回值、driver registered flag 和 completed/failed 状态。

`module_initialize()` 设置：

| 字段 | 值 |
| --- | --- |
| `uninitializer` | `shellpp_supervisor_uninit` |
| `arg` | 0 |
| `exports` | 0 |
| `nexports` | 0 |

module 不通过 loader exports 提供符号。Lua 只通过 `/dev/shellpp` 控制它。

## 5. 固件 ABI 调用模型

所有固件地址来自构建器生成的 `shellpp_target_abi.h`。nativeApp 的稳定 include `shellpp_firmware_abi.h` 要求 `SHELLPP_TARGET_ABI_GENERATED` 存在；直接脱离构建器编译应失败。

源码用 target 宏构造函数指针，例如 driver、文件、App registry、LVX/LVGL、Activity 和 restart API。函数原型仍由 C typedef 表达，因此 profile 中地址正确并不自动证明参数、返回值、线程、所有权或结构布局正确。

`__aeabi_unwind_cpp_pr0` 是一个只执行 `bx lr` 的 stub。当前代码不进行 C++ 异常或栈展开，但 Clang 可能生成 ARM unwind index 引用；该 stub 仅满足 module 自包含要求，不能用于真正展开。

## 6. Supervisor 状态

Supervisor 保留以下静态状态：

| 状态 | 含义 |
| --- | --- |
| `g_command` | 最近一次合法控制写入的命令 |
| `g_stage` | 命令 arg0 / install stage |
| `g_state` | `RESULT_COMPLETED = 5` 或 `RESULT_FAILED = 15` |
| `g_registered` | control driver 是否注册成功 |
| `g_error` | 最近一次命令的业务结果或 constructor 结果 |
| native status | App ID、注册、发布、通知和三个原始结果 |

`g_status` 固定为 384 字节。每次 read 都先完全清零，再按固定 word 写入状态，避免未初始化或旧快照数据泄漏。完整 word 表见[状态与控制 ABI](reference/status-control-abi.md)。

源码包含一个位于标准 writable `.data` 的非零 anchor，确保 module 具有目标 modlib 预期的初始化数据 section。

## 7. control file_operations

### 7.1 open 和 close

open/close 不保存 per-file state，当前总是返回 0。协议状态为全局 module 状态，不按文件描述符隔离。

### 7.2 read

`control_read(file, buffer, count)` 的规则：

- `buffer == NULL` 或 `count < 384` 返回 `-22`；
- 成功时写入完整 384 字节并返回 384；
- 不支持短状态读取；
- 状态 word 使用设备小端表示；
- word 10 和 11 当前都写入 stage，保持与参考 Supervisor 的稳定快照对形，但当前 Shell++ II write 同步完成，不依赖 sequence-pair 重试。

### 7.3 write

控制写入必须至少 16 字节，并包含四个 `uint32_t`：magic、command、arg0、arg1。当前 arg1 保留且不使用。

校验失败条件：

- buffer 为空；
- count 小于 16；
- command magic 不是 `0x53505331`。

以上条件直接返回 `-22`。合法框架的命令无论业务成功或失败都返回 16，业务结果写入状态的 error 和 result state。这与 Lua 的调用方式配套：Lua 先确认写完成，再读 384 字节状态。

支持的命令：

| 命令 | 处理 |
| --- | --- |
| notify loaded | 调用 `shellpp_native_notify_loaded()` |
| restore | 当前作为同步点返回 0 |
| install stage 0 | 当前作为同步点返回 0 |
| install stage 1 | 注册 App 与页面 |
| install stage 2 | 发布 Launcher |
| uninstall | 调用受保护的 native uninstall |

未知命令或 install stage 返回 `-22` 或 native 层的具体错误，并将 state 设为 failed。

## 8. 为什么分阶段

App 安装和 Launcher 发布不是一个不可分割的固件调用：

- stage 1 创建固件 registry 项；
- registry publication 可能由固件 worker 异步提交；
- stage 2 必须在 lookup 能观察到已注册 App 后才能发布 Launcher；
- Lua 用独立 LVGL timer callback 在阶段之间给固件 event loop 运行机会。

将所有阶段放进同一个 Lua click callback，或在 C 中立即连续调用并假定 registry 同步可见，都可能产生 App missing、页面入口未发布或固件崩溃。

## 9. 内部子系统边界

`supervisor.c` 只协调协议，不直接构造 UI 或读写文件：

```text
Lua
  │ /dev/shellpp
  ▼
Supervisor
  ├── native_app.c ── App/Page registry, Launcher, notification
  │                      │ page callbacks
  │                      ▼
  │                 native_ui.c
  │                      │ filesystem/system metrics
  │                      ▼
  └──────────────── native_fs.c
```

这种边界使 Lua 不需要知道固件 ABI 地址，文件系统也不需要知道 Supervisor command magic。

## 10. 卸载保护

`shellpp_supervisor_uninit()` 首先询问 `shellpp_native_can_unload()`：

- App 尚未注册：允许注销 `/dev/shellpp`，返回 0；
- App 已注册：返回 `-16`，不注销 driver，也不允许 loader 回收 module text。

原因不是 driver 本身，而是固件 registry 已保留 module 内 descriptor、字符串和回调地址。只移除设备节点会留下悬空 App/Page/Launcher 指针，下一次点击图标或页面事件可能跳入已释放 module。

native uninstall 在已注册时返回 `-95`。当前安全移除路径是删除 Shell++ II 环境目录并重启。未经完整逆向验证，不得把 uninitializer 改成无条件成功，也不得调用未知 registry remove API。

## 11. 目标专用差异

Supervisor 共享实现当前没有 043 专用补丁。native App、UI 和文件系统的 043 差异由构建器补丁进入临时源码。若未来固件改变 file_operations 前缀、constructor 时序、状态 driver ABI 或 unload 规则，应：

1. 先恢复完整新 ABI 和结构证据；
2. 能数值表达的内容扩展 profile；
3. 只能语义表达的内容建立新 target patch；
4. 不修改已真机确认的 036 路径来迁就新固件；
5. 同步升级 Lua/Supervisor 协议时必须明确兼容策略。

## 12. 不变量和禁止项

- 不得把 bin 当作 `ET_EXEC`、裸 `.bin` 或固定加载地址镜像。
- 不得添加未解析 undefined symbol 依赖。
- 不得在源码中硬编码新固件地址。
- 不得扩大 file_operations 布局并假定保留字段和参考实现一致。
- 不得将合法 write 的返回值当成业务结果；业务结果来自随后 read。
- 不得在 stage 1 后卸载 module。
- 不得在旧 Supervisor 驻留时加载另一 target；必须重启。
- 不得以 constructor 成功推断 App 注册或 UI 功能成功。

## 13. 验证点

构建器静态验证应证明：

- ELF32 little-endian ARM ET_REL、EABI5、无 program header；
- `module_initialize` 唯一定义且 e_entry 为零；
- required/allowed sections 和 relocation 集合成立；
- 无 undefined import；
- 直接固件地址在 profile 白名单中；
- loaded footprint 与 BSS 未达到 target 上限。

真机还必须单独证明：constructor 能注册 driver、状态可读写、五步命令顺序成立、App 能打开全部页面、生命周期正常、注册后卸载被阻止。
