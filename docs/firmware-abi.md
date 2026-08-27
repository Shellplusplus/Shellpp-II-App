# 固件 ABI 与证据规范

## 1. 首要准入条件

**在同一机型上适配一个新系统，必须先取得该系统的完整固件 ABI。** 这是开始 target 适配工作的前置条件，不是构建完成后的补充材料。

完整固件镜像不等于完整 ABI。只有镜像而没有以下信息时，工作仍处于 ABI 恢复阶段：函数身份、运行时地址、C 原型、调用约定、结构布局、枚举/常量、数据地址、线程和生命周期约束。此时不得创建可发布 profile，不得填零值或复制旧值，不得用版本间地址差值猜测，不得将可链接 bin 称为适配完成。

## 2. ABI 的定义

本项目把 ABI 定义为 native module 能够安全调用固件并被固件安全回调的全部约束：

| 类别 | 必须确认的内容 | 当前消费者 |
| --- | --- | --- |
| 镜像身份 | 精确版本、镜像路径、字节数、SHA-256、加载映射 | 构建器 |
| 函数入口 | 运行时地址、函数边界、Thumb 位、入口身份 | 全部 C 源码 |
| 原型 | 参数顺序/宽度、返回值、指针目标、寄存器约定 | typedef 和调用点 |
| 数据地址 | 字体 style、全局对象或其他 XIP/RAM 数据 | UI 和 descriptor |
| 常量 | open/seek/dirent、align、event、trailing 等值 | 文件/UI 调用 |
| 结构 | App/Page descriptor、notification、dirent、固件对象偏移 | App/UI/FS |
| 时序 | 同步或异步、worker publication、事件循环机会 | Supervisor/App/Lua |
| 资源 | 栈、BSS、对象所有权、重入和回调存活时间 | UI/FS/App |
| 逆向生命周期 | unregister、registry remove、Launcher remove、callback 排空 | unload/uninstall |

profile 目前能直接表达镜像元数据、地址、常量、枚举、descriptor 尺寸和资源限制；原型、结构内部偏移、时序和证据仍必须在 ABI 记录中独立维护。若新固件改变了这些内容，应先扩展 schema/生成器或建立经过审查的 target patch。

## 3. 注入链

nativeApp 不包含固件默认地址，只包含稳定边界：

```c
#include "shellpp_firmware_abi.h"
```

实际构建链：

```text
targets/<TARGET_ID>.env
  -> generate_target_abi.py
  -> out/<TARGET_ID>/generated/shellpp_target_abi.h
  -> clang include path
  -> nativeApp C/汇编
```

`shellpp_firmware_abi.h` 再包含生成头，并要求 `SHELLPP_TARGET_ABI_GENERATED` 已定义。脱离构建器直接编译应失败，不能回退到 036 或 043 的地址。生成头带 profile 来源和“do not edit”标记；修改它不会改变规范来源。

## 4. profile 字段类别

当前生成器要求每个 profile 提供：

- `TARGET_ID`、`FIRMWARE_VERSION`、`FIRMWARE_CODE`；
- `FIRMWARE_IMAGE`、`FIRMWARE_IMAGE_SIZE`、`FIRMWARE_IMAGE_SHA256`；
- `CPU`、`FLOAT_ABI`、`MAX_LOADED_SIZE`、`MAX_BSS_SIZE`；
- driver 注册/注销地址；
- open/read/write/close/lseek/unlink/rename/opendir/closedir/readdir/rmdir 地址；
- App lookup/install、Launcher add、notification submit 地址；
- LVX/LVGL object、label、style、list row、event、timer 地址；
- Activity navigate/finish、spawn/file-actions、spawn attributes、waitpid、soft restart 地址；
- MiSans style 数据地址；
- open/seek/dirent 常量、descriptor 尺寸、align/event/trailing 枚举。

完整字段和形式约束见[目标 profile 模式](reference/target-profile-schema.md)。本文不复制地址表，避免文档和 profile 漂移。

## 5. 适配分析地址与运行时地址

固件分析工具可能把 AP 镜像映射到 `0x2c000000`，但设备执行 native module 时使用 XIP 运行时映射。当前两份精确记录的 10 Pro AP 镜像通过启动头证明：文件偏移 `0` 对应运行时 `0x0c0c0000`。对这两份哈希固定的镜像，分析和运行时地址关系为：

```text
analysis_address = 0x2c000000 + file_offset
runtime_address  = 0x0c0c0000 + file_offset
```

这条关系不是所有后续镜像的默认事实。新增固件必须重新从启动头或等价加载证据确认。不要将运行时地址的低 24 位直接当成文件偏移；这种错误会系统性错开 `0x0c0000`，且错误结果仍可能落在可解码指令中。

地址转换只说明映射，不说明符号身份。每个函数仍需通过反汇编入口、调用点、字符串、调用图、数据流、机器码签名或可靠参考实现独立确认。

## 6. Thumb 与数据地址

当前 CPU 为 Cortex-M33 Thumb 执行环境。profile 中函数地址必须：

- 非零；
- 最低位为 1；
- 使用设备运行时映射；
- 与 typedef 的原型和 ARM EABI 调用约定一致。

最低位为 1 只证明编码形式，不证明函数身份、原型或线程语义。生成器只检查形式，ELF verifier 检查白名单，真机测试才检查行为。

MiSans style 是数据地址，不能加 Thumb 位；当前要求字对齐。把数据地址填入函数字段或把函数地址写进 data 字段都属于 ABI 错误。

## 7. 原型和调用约定

profile 保存地址，不保存 C 类型。typedef 位于 `supervisor.c`、`native_app.c`、`native_fs.c`、`native_ui.c`。恢复 ABI 时逐个证明：

1. 参数数量、顺序和宽度；
2. 返回值宽度、成功/失败语义和是否为 opaque result；
3. 指针指向的结构、字段偏移和对齐；
4. caller/callee 保存寄存器与 Thumb 调用约定；
5. 调用是否同步、是否排队 worker、何时消费指针；
6. 所在线程是否允许 LVGL、文件 I/O、Activity navigation 或重启；
7. callback 返回后固件是否继续保存 descriptor、字符串或回调地址。

“找到了相似函数”不足以完成适配。原型不确定时，必须把该字段标为缺口，不能只替换地址继续编译。

## 8. 结构和枚举

当前共享源码还依赖以下非地址 ABI：

- App descriptor 内 package `+0x08`、icon `+0x0c`、App ID `+0x10`、display callback `+0x1c`；
- Page descriptor name `+0x10`、key `+0x14`、signal `+0x34`、create `+0x4c`、resume `+0x50`、pause `+0x58`、destroy `+0x5c`；
- 0x58 字节 notification record 及 class/flags 位置；
- `readdir()` 记录类型在第 1 字节、名称从 `raw + 1` 开始；
- App/Page descriptor 尺寸；
- open/seek/dirent、LVX align、clicked event 和 trailing 值。

当前两个 profile 的 descriptor size 为非零四字节倍数，并由 `_Static_assert` 固定数组大小。新固件若改变内部偏移，不能只改 size；需要生成字段、专用 patch 或重新设计共享布局。

## 9. 栈、BSS 和生命周期

ABI 还包括调用方可提供的栈和对象存活时间。应对 event callback、Page create/resume、timer、文件遍历、JSON parser、App install 计算完整调用链，记录：

- 编译器可见每帧大小；
- 最大递归深度或显式 walk frame 数；
- 固件函数内部未知栈消耗；
- 中断/框架余量；
- 静态 workspace 所有者、阶段和重入条件；
- descriptor、字符串、row、timer、overlay 的释放顺序。

043 的专用补丁把部分大局部数组放到可证明串行使用的静态 scratch，并用显式深度栈代替递归。这不是“ABI 已证实安全”的替代品；主机 stack report 不能测量固件入口内部栈。

## 10. 固件代码交叉检查

版本代码为：

```text
major * 1,000,000 + minor * 1,000 + patch
```

minor、patch 小于 1000。当前：

| version | code |
| --- | ---: |
| `3.101.036` | `3101036` |
| `3.101.043` | `3101043` |

生成器校验 profile，Lua 独立计算，Supervisor 将生成值写入状态 word 12。三处不一致时，优先拒绝加载并重启，不尝试猜测版本。

## 11. ABI 证据记录模板

每个函数/数据/结构至少记录：

| 字段 | 内容 |
| --- | --- |
| key | profile 中的精确名称 |
| image | 固件路径、大小、SHA-256 |
| analysis address | 反汇编工具使用的地址 |
| runtime address | module 实际调用地址 |
| identity evidence | 入口、调用点、字符串、数据流、签名等 |
| prototype | C 原型和参数解释 |
| layout | 相关结构偏移、尺寸、对齐 |
| timing | 同步/异步、线程和保存时间 |
| status | 静态恢复、主机验证、真机确认或未验证 |
| negative evidence | 被排除的候选和原因 |

独立证据比“与旧版本相差固定字节”更重要。多候选、函数中段、短序言或未知返回语义都必须保留为未决问题。

## 12. 禁止的 ABI 维护方式

- 在 nativeApp 源码中硬编码新固件绝对地址；
- 在 Lua 中维护 ABI 地址表；
- 直接编辑 generated header；
- 用版本间固定差值生成新地址；
- 复制旧 profile 只改版本号；
- 用零值、最近候选或偶数地址占位；
- 因 POSIX/LVGL 同名而假设枚举相同；
- 用 bin 能链接、`insmod` 能返回或一个页面能打开证明 ABI 完整；
- 在未确认 reverse unregister ABI 前实现 live unload；
- 把历史日志中的 PC/LR 当作新产物的当前证据。

## 13. ABI 完整性的出口条件

只有同时满足以下条件，才能从“ABI 恢复”进入“target 适配”：

1. 精确镜像身份已固定；
2. profile 所有必填字段都有独立证据；
3. 所有调用点的原型、结构和枚举没有未处理缺口；
4. XIP 映射和 Thumb 形式已证明；
5. worker、线程、栈、BSS 和 callback 生命周期已审查；
6. App/Page/notification/dirent 的关键布局已确认；
7. reverse unregister 的未知部分已明确建模为“只能重启移除”，而不是假设可卸载；
8. 证据表绑定了镜像 SHA-256 和分析工具版本。

出口后仍需要构建验证和真机验证；ABI 完整不等于功能已经在设备上通过。
