# Lua 安装器实现与协议

## 1. 范围

本篇说明安装器 `_Lua/main.lua` 的资源模型、固件检测、bin 选择、ELF 预检、Supervisor 控制协议、Run 状态机、图标 staging、清理和重启行为。字段级状态布局见[状态与控制 ABI](reference/status-control-abi.md)。

安装器只有一个 Lua 文件。新增同机型固件且命名与协议不变时，只需由构建器新增并部署对应版本 bin，不应复制或修改 Lua。

## 2. 资源模型

编辑目录与 packaged 目录必须包含同一组资源：

```text
_Lua/
├── main.lua
├── shellpp_ii_icon.bin
├── shellpp_ii-3.101.036.bin
└── shellpp_ii-3.101.043.bin

resources/_lua/_Lua/
├── main.lua
├── shellpp_ii_icon.bin
├── shellpp_ii-3.101.036.bin
└── shellpp_ii-3.101.043.bin
```

Lua 从运行环境的 `SCRIPT_PATH` 访问同目录资源。真正进入 `resource.bin` 的来源是 `resources/_lua/_Lua`；构建器要求两处 `main.lua` 与图标逐字节一致，并将两处版本化 bin 同步。

支持版本由 `shellpp_ii-<version>.bin` 文件集合决定。Lua 不维护版本白名单、地址表或版本范围。

## 3. 常量和稳定契约

| 项目 | 当前值 |
| --- | --- |
| 系统版本属性 | `ro.build.version` |
| 临时版本文件 | `/data/shellpp-installer-firmware-version.tmp` |
| module loader name | `shellpp_ii` |
| Supervisor device | `/dev/shellpp` |
| 图标资源 | `SCRIPT_PATH .. "shellpp_ii_icon.bin"` |
| 图标设备路径 | `/data/shellpp-ii/shellpp_ii_icon.bin` |
| 状态长度 | 384 字节 |
| 状态 magic | `0x53505331` |
| 状态 ABI | 3 |
| 命令 magic | `0x53505331` |
| completed state | 5 |
| module size 下限 | 512 字节 |
| module size 上限 | 262144 字节 |

Lua 和 C 对这些值必须一致。变更任何协议值需要同时修改安装器、Supervisor、参考文档和兼容性处理。

## 4. 固件版本检测

`detect_firmware_version()` 执行：

```text
getprop 'ro.build.version' > '/data/shellpp-installer-firmware-version.tmp'
```

流程：

1. shell 参数使用单引号，并将内部单引号安全转义；
2. 只有 `os.execute` 成功才读取临时文件；
3. 读取后用 `os.remove` 尝试删除；
4. 去除首尾空白；
5. 要求完整匹配三个十进制组件 `major.minor.patch`。

不接受后缀、前缀、两段版本、空组件或任意文本。检测失败时 firmware version 为 unknown。

## 5. bin 精确选择

选择公式：

```lua
MODULE_PATH = SCRIPT_PATH .. "shellpp_ii-" .. version .. ".bin"
```

不存在以下行为：

- 不选择“最近版本”；
- 不忽略 patch 号；
- 不在 036/043 之间 fallback；
- 不读取通用 `shellpp_ii.bin`；
- 不根据文件大小或哈希猜固件；
- 不从 Lua 地址表动态修补 module。

精确文件不存在或预检失败时，页面只显示 firmware not supported，并在顶层返回，不创建 Run/Uninstall/Clear/Reboot 按钮。

## 6. firmware code

Lua 独立计算：

```text
firmware_code = major * 1,000,000 + minor * 1,000 + patch
```

minor 和 patch 必须小于等于 999。当前：

| 版本 | code |
| --- | ---: |
| `3.101.036` | 3101036 |
| `3.101.043` | 3101043 |

profile 生成器也验证同一公式，Supervisor 将生成的 code 写入状态 word 12。Lua 读取 word 12 可判断当前驻留 module 是否与设备版本相符。

## 7. Lua 侧 ELF 预检

`verify_module_file()` 读取前 20 字节与文件长度，要求：

- ELF magic `0x7f ELF`；
- ELFCLASS32；
- little-endian；
- ELF ident version 1；
- `e_type = ET_REL = 1`；
- `e_machine = EM_ARM = 40`；
- 文件长度在 512..262144 字节。

此检查只是设备加载前的快速格式门槛，不验证：

- ARM EABI flags；
- sections 和 relocations；
- undefined symbols；
- profile 地址白名单；
- loaded footprint/BSS；
- bin SHA-256；
- ABI 原型或真实固件兼容性。

完整静态验证只能由构建器 `verify_shellpp_elf.py` 完成。Lua 预检成功不能作为发布证据。

## 8. 图标 staging

Launcher descriptor 使用固定设备路径，因此 Run 在注册 App 前写入图标。

`stage_manager_icon()` 验证资源格式：

- 至少 13 字节；
- 第一个字节为 `0x19`；
- width 为 bytes 5..6 的小端 16 位值；
- height 为 bytes 7..8；
- width 和 height 均大于 0；
- 总长度必须等于 `12 + width * height * 4`。

写入流程：

1. 尝试打开 `/data/shellpp-ii/shellpp_ii_icon.bin`；
2. 如果目录不存在，执行 `mkdir /data/shellpp-ii` 后重试；
3. 使用 protected call 写入并关闭；
4. 重新读取并与资源内容逐字节比较。

不能只检查 open/write 返回而不回读。App descriptor 中的图标路径在 stage 1 后会被固件保存，注册前必须完成 staging。

## 9. Supervisor 是否驻留

Lua 通过能否打开 `/dev/shellpp` 判断 Supervisor 是否存在。

如果不存在，Run 执行：

```text
insmod '<selected-version-bin>' shellpp_ii
```

随后再次确认 device 出现。`insmod` 成功返回而 device 不存在仍视为 LOAD failed。

如果 device 已存在，Lua不会再次 `insmod`。它读取当前 Supervisor 状态，避免同一运行期加载第二个 module 或替换旧 target。

## 10. 状态读取

Lua 每次打开 `/dev/shellpp` 并精确读取 384 字节。`bytes_to_words()` 按小端转换为 96 个 32 位 word；长度不等于 384 直接失败。

强制检查：

| 检查 | 失败含义 |
| --- | --- |
| word 1 = magic | 不是 Shell++ II 状态协议 |
| word 2 = ABI 3 | 旧或不兼容 Supervisor 驻留 |
| word 12 = expected firmware code | 错误 target 或其他固件的 module 驻留 |
| word 13 = 1 | control driver 未成功注册 |

word 9 用 `signed32()` 从无符号 Lua number 恢复有符号错误码。

参考 Canopus 协议有 sequence-pair consistency check；当前 Shell++ II 命令在 C `write()` 内同步完成，read 得到的 384 字节快照已完整，因此 Lua 不循环比较 words 10/11。

## 11. 控制写入

`write_command(command, arg0)` 构造 16 字节：

| byte offset | 内容 |
| ---: | --- |
| 0 | magic |
| 4 | command |
| 8 | arg0 |
| 12 | 0，保留 arg1 |

每个 word 使用纯 Lua 按小端编码。write 和 close 都用 `pcall`，任一异常或 nil 返回视为失败。

合法 C write 固定返回 16，即使业务失败；因此 `execute_step()` 必须随后 read status，并要求：

- pending operation 等于刚发送的 command；
- pending state 等于 completed 5。

否则从 error word 生成失败信息。不能把 Lua file write 成功等同于 native operation 成功。

## 12. Run 前置状态

Run 在一次 Lua 页面实例中最多尝试一次：

- 首次点击立即设置 `run_attempted = true`；
- Run button 取消 clickable；
- 后续点击提示重启后再试；
- 即使首次尝试在中途失败，也不在同一页面实例恢复按钮。

这样避免在 partial stage、旧 callback 或已加载 module 上重复注册。失败后的恢复单位是设备重启，不是重新点击 Run。

## 13. 完整 Run 顺序

Run 同步前置部分：

1. 重新预检 selected bin；
2. 如果 device 不存在则执行 `insmod`；
3. 确认 device 出现；
4. 读取状态并检查 magic、ABI、firmware code、driver registered；
5. stage 并回读校验图标；
6. 创建 LuaLVGL timer。

之后 timer 分五次执行：

| 顺序 | command | arg0 | native 行为 |
| ---: | --- | ---: | --- |
| 1 | `0x53510004` notify loaded | 0 | 提交一次 foreground/module 加载通知 |
| 2 | `0x5351000A` restore | 0 | 当前作为完成同步点 |
| 3 | `0x53510002` install | 0 | 当前作为完成同步点 |
| 4 | `0x53510002` install | 1 | 注册 App 和页面 |
| 5 | `0x53510002` install | 2 | 发布 Launcher |

timer period 为 1000 ms，创建时 paused，随后 resume 并 ready；每步完成后下一步调用 `timer:ready()`。其目的不是延迟一秒，而是让每个 native 注册阶段位于独立 LuaLVGL callback，使 miwear/固件 event loop 在阶段之间运行。

任一步失败：

- 删除 timer；
- 清除全局 run timer 引用；
- 显示错误和“Reboot before retrying”；
- 不发送自动逆向命令。

全部完成显示 `Run completed`。

## 14. 为什么不自动回滚

stage 1 可能已经将 descriptor 和 callback pointer 发布到固件 registry。当前没有确认的 reverse unregister ABI。发生后续失败时：

- 无法可靠判断 worker 已发布到哪个阶段；
- 卸载 module 会留下悬空 callback；
- 只移除 Launcher 或 driver 不完整；
- 重复 stage 1 可能遇到已有 registry 项。

因此安装器采用 fail-stop：保留 module 驻留并要求重启。不得在 Lua failure handler 中添加 `rmmod`、未知 remove command 或强制删除 registry。

## 15. Uninstall 和 Clear Env

当前两个按钮都调用：

```text
rm -rf /data/shellpp-ii
```

差异：

- Uninstall 单次执行并提示重启；
- Clear Env 需要连续两次点击确认；
- Run timer 进行时两者都不应与安装步骤并发；
- 两者都不发送 `CMD_UNINSTALL`；
- 两者都不 live remove App/Page/Launcher/module。

目录删除会移除 staged 图标和 Shell++ II 私有 cache/tmp/logs，但不会使固件 RAM 中的 callback 自动失效。重启才是移除完成边界。

`CMD_UNINSTALL = 0x53510003` 仍在协议和 C 端定义；App 已注册时返回 `-95`，用于明确拒绝不安全 live uninstall，不是当前 UI 清理路径。

## 16. Reboot

安装器 Reboot 按钮直接执行系统 `reboot` shell command。这与 native App 内的 soft restart/spawn 路径不同。

清理、旧 Supervisor 驻留、错误 target、Run partial failure 或重新安装前，使用安装器重启可以清除 RAM 中 module 和 App registry 状态。

## 17. 界面和异常隔离

按钮 callback 都用 `pcall` 包裹，Lua 异常显示在状态区域。Run timer callback 也用 `pcall`，异常触发相同 fail-stop 和重启提示。

状态区域放在固定 log panel 中，长错误文本由 LVGL label 展示。界面布局不改变协议语义；修改样式时不得改变按钮 callback 或命令顺序。

## 18. 新固件无需改 Lua 的条件

同时满足下列条件时，新增 target 只修改构建器：

1. 设备仍用 `ro.build.version` 返回精确三段版本；
2. bin 命名仍是 `shellpp_ii-<version>.bin`；
3. firmware code 公式不变；
4. module loader name 和 `insmod` 形式不变；
5. `/dev/shellpp`、状态长度、magic、ABI 和 word 布局不变；
6. 五步命令顺序和 command IDs 不变；
7. 图标格式和运行时路径不变。

只需新增 profile、可选 target patch 和版本化 bin。若任一条件变化，则属于跨仓库协议升级，必须同时修改 Lua、C、构建器验证和文档。

## 19. 常见陷阱

- 将 `getprop` 输出与 prefix 或范围匹配；
- 对未知版本 fallback 到 036/043；
- 两个固件共用无版本文件名；
- 只把新 bin 放进顶层 `_Lua`，没有同步 packaged tree 和 `resource.bin`；
- 将 Lua ELF 预检当作完整验证；
- device 已存在时仍重复 `insmod`；
- 忽略状态 ABI 或 firmware code；
- 只看 write 是否成功，不读 result state；
- 在一个 click callback 中连续执行全部注册步骤；
- failure 后允许再次 Run；
- stage 1 后自动 `rmmod`；
- 认为删除 `/data/shellpp-ii` 已 live 卸载；
- 图标写入后不回读校验；
- 修改一个 Lua 资源副本而让两处不同；
- 改 command ID 只修改 Lua 或只修改 C；
- 旧 Supervisor 驻留时用新日志/新 bin 继续测试而未重启。

## 20. 验证清单

安装器变更后至少验证：

1. `luac -p` 通过；
2. 两个 `main.lua` 逐字节一致；
3. 036 和 043 的精确 bin 均存在；
4. 未知版本不会创建操作按钮；
5. resource 反向解析内容与 packaged tree 一致；
6. 正确固件显示 Ready 和精确版本；
7. 错误/旧驻留 Supervisor 被 ABI/code 检查拒绝；
8. Run 只允许一次；
9. 五步顺序和每步 result 检查不变；
10. 清理后明确要求重启。
