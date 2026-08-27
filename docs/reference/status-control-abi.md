# 状态与控制 ABI

本页是 Lua 安装器与 native Supervisor 之间的二进制协议规范。实现端分别是 nativeApp `module/src/supervisor.c` 和安装器 `_Lua/main.lua`。协议修改必须同步修改两端、提升兼容版本并验证旧驻留 Supervisor 的拒绝路径。

## 1. 传输端点与编码

| 项目 | 当前值 |
| --- | ---: |
| 设备路径 | `/dev/shellpp` |
| 状态/控制 magic | `0x53505331` |
| 状态 ABI version | `3` |
| 状态记录 | `384` B，96 个 word |
| 控制记录 | 最少 `16` B，4 个 word |
| 完成 state | `5` |
| 失败 state | `15` |

设备目标是 little-endian ARM。协议中每个 word 都按 little-endian 32 位编码；带符号错误需按 int32 解释。Lua 将大于或等于 `0x80000000` 的 word 减去 `0x100000000`。

Supervisor 注册的 file-operations 只提供 open、close、read、write 四个前缀 callback。open/close 返回 0，不保存每个文件句柄的私有状态；状态由 module 级全局变量维护。

## 2. 状态读取

`read(buffer, count)` 的规则：

- 空 buffer 或 `count < 384`：直接返回 `-22`；
- 合法请求：先清零内部 384 B，再编码当前快照，复制恰好 384 B 并返回 `384`；
- 请求长度大于 384 B 时也只返回 384 B；
- reserved word 当前为零，但消费者不能依赖其永久为零。

下表使用协议的 1-based word 编号。Lua table index 与 word 编号相同；C byte offset 是 `(word - 1) * 4`。

| Word | C byte offset | 类型 | 字段 | 含义 |
| ---: | ---: | --- | --- | --- |
| 1 | `0x00` | uint32 | magic | `0x53505331` |
| 2 | `0x04` | uint32 | status ABI | 当前为 `3` |
| 3-5 | `0x08..0x10` | uint32 | reserved | 当前清零 |
| 6 | `0x14` | uint32 | command | 最近一次格式有效的控制命令 |
| 7 | `0x18` | uint32 | state | 最近结果，`5` 或 `15` |
| 8 | `0x1c` | uint32 | reserved | 当前清零 |
| 9 | `0x20` | int32 | error | 最近 driver/命令的项目错误 |
| 10 | `0x24` | uint32 | stage snapshot A | 最近命令的 arg0/stage |
| 11 | `0x28` | uint32 | stage snapshot B | 与 word 10 相同 |
| 12 | `0x2c` | uint32 | firmware code | 编译 target 的版本整数 |
| 13 | `0x30` | uint32 | driver registered | `/dev/shellpp` 注册成功为 1 |
| 14 | `0x34` | uint32 | App ID | 当前为 `0x00cd` |
| 15 | `0x38` | uint32 | App registered | 固件表中已确认本 App 为 1 |
| 16 | `0x3c` | uint32 | Launcher published | Launcher 调用路径已执行为 1 |
| 17 | `0x40` | uint32 | loaded notified | 通知提交路径已执行为 1 |
| 18 | `0x44` | int32 | install result | 固件 `APP_INSTALL` 原始返回 |
| 19 | `0x48` | int32 | launcher result | 固件 `LAUNCHER_ADD` 原始返回 |
| 20 | `0x4c` | int32 | notification result | 固件通知提交原始返回 |
| 21-96 | `0x50..0x17c` | uint32 | reserved | 当前清零 |

word 10/11 保留 Canopus 的双快照形状，但当前 Shell++ 命令同步完成于 device `write()` 内，不用这对字段进行异步 sequence 一致性判断。Lua 当前读取 word 1、2、6、7、9、12 至 17；word 18 至 20主要用于 native 诊断。

constructor 注册 driver 后，在任何控制命令之前，word 6 和 word 10/11 为 0；word 7/9 已反映 `register_driver` 的结果，word 13 表示是否成功。

## 3. 控制写入

控制 payload 是四个 little-endian word：

| Word | offset | 字段 | 规则 |
| ---: | ---: | --- | --- |
| 1 | `0x00` | magic | 必须为 `0x53505331` |
| 2 | `0x04` | command | 命令表中的值 |
| 3 | `0x08` | arg0/stage | install 使用 0、1、2 |
| 4 | `0x0c` | reserved | Lua 写零；C 当前忽略 |

空 buffer、`count < 16` 或 magic 错误时，write 直接返回 `-22`，且不会按正常命令路径更新 command/state。只要格式有效，Supervisor 就执行或拒绝命令、更新 word 6/7/9/10/11，并返回 `16`。因此：

- write 返回 16 只表示控制记录已被消费；
- native 操作是否成功必须继续 read status；
- 未知命令和无效 install stage 的 write 仍返回 16，但 word 7 为 15、word 9 为 -22。

Lua 使用独立的 LuaLVGL timer callback 依次发送各 stage，让固件 event loop 在 native 注册阶段之间获得调度机会。

## 4. 命令表

| 命令 | 值 | arg0 | 当前 native 行为 |
| --- | ---: | ---: | --- |
| `CMD_NOTIFY_LOADED` | `0x53510004` | 0 | 一次性提交 foreground loaded notification |
| `CMD_RESTORE_AFTER_BOOT` | `0x5351000a` | 0 | 接受并完成；当前无额外 native 恢复动作 |
| `CMD_INSTALL` | `0x53510002` | 0 | 接受并完成；作为注册序列的兼容阶段 |
| `CMD_INSTALL` | `0x53510002` | 1 | 注册或确认 App/Page 描述符 |
| `CMD_INSTALL` | `0x53510002` | 2 | 确认 App 后发布 Launcher |
| `CMD_UNINSTALL` | `0x53510003` | 当前忽略 | 未注册时成功；已注册时返回 `-95` 要求重启 |

`CMD_NOTIFY_LOADED`、restore 和 uninstall 当前不校验 arg0。未知 command，或 install 的 arg0 不是 0/1/2，进入状态错误 `-22`。

安装器 Run 的实际五步顺序是：notification、restore、install 0、install 1、install 2。改变顺序需要重新验证固件 worker 时序、通知行为和 Launcher 发布条件。

## 5. 状态更新与原始诊断

每次格式有效的 write 先保存 command/stage 并清零项目错误，操作结束后：

- native 返回 0：word 7 = 5，word 9 = 0；
- native 返回非零：word 7 = 15，word 9 = 对应 int32；
- write 无论上述结果都返回 16。

word 18 至 20 不参与这个成功判定。当前 native App 对某些固件入口采用“记录原始值，再用可观察状态确认”的策略：

- `APP_INSTALL` 后以 bounded lookup 和包名匹配确认注册；
- `LAUNCHER_ADD` 的不透明返回存入 word 19，调用路径完成后 published 置 1；
- 通知提交结果存入 word 20，提交路径执行后 notified 置 1。

所以不能看到 word 18/19/20 非零就直接套用 errno，也不能只看 flag 而丢弃原始诊断。

## 6. 固件 target 校验

word 12 来自生成宏 `SHELLPP_ABI_FIRMWARE_CODE`。Lua 根据 `ro.build.version` 独立计算：

`major * 1,000,000 + minor * 1,000 + patch`

Lua 在读状态时同时校验 magic、ABI version 与 firmware code。code 不匹配意味着错误版本 bin 或旧 module 驻留，必须重启并使用精确目标。这个检查不能证明 profile 中每个 ABI 地址正确。

## 7. 兼容性规则

下列变更必须提升 status ABI，并同步 C、Lua、reference、全部 target 构建和真机协议测试：

- word 的位置、类型或语义改变；
- magic、记录大小或状态编码改变；
- 控制 command/arg 含义改变；
- 同步 write 改为异步完成；
- consumer 开始依赖 reserved word。

旧 ABI module 已驻留时不能 live 替换。Lua 必须拒绝并要求重启。
