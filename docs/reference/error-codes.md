# 错误码与返回通道

本页区分五类容易混淆的结果：device read/write 的直接返回、Supervisor status 错误、native App 私有错误、文件系统内部错误、固件 API 原始诊断。相同整数出现在不同通道时不一定具有相同语义。

## 1. 返回通道

| 通道 | 消费者 | 成功形状 | 失败形状 |
| --- | --- | --- | --- |
| device read | Lua/诊断工具 | 返回 384 B | 参数错误直接 `-22` |
| device write | Lua | 格式有效返回 16 | 格式无效直接 `-22` |
| status word 7/9 | Lua | state 5、error 0 | state 15、项目 error |
| status word 18-20 | 诊断 | 固件原始 int32 | 不映射为项目 errno |
| native UI/FS 函数 | native UI | `SHELLPP_FS_OK` | `-200..-215` |
| module uninitializer | NuttX loader | 0 | 注册后保护性 `-16` |
| shell/`insmod` | Lua 的 `os.execute` | shell 成功状态 | 只转换为安装器文本，不进入 status |

格式有效但命令未知是一个关键例外：device write 返回 16，随后 status word 7/9 为 15/-22。调用者不能把 write 成功等同于命令成功。

## 2. Supervisor 和 native App 错误

| 值 | 来源 | 含义 | 恢复 |
| ---: | --- | --- | --- |
| `0` | 通用 | 操作成功 | 继续下一 stage |
| `-16` | module uninitializer | 固件仍保存 App/Page callback，拒绝 unload | 清理 staging 后重启 |
| `-22` | read/write/Supervisor | buffer/长度/magic/command/install stage 无效 | 修正协议；旧驻留则重启 |
| `-95` | `ERR_UNINSTALL_REQUIRES_REBOOT` | 已注册 native App 无安全 live unregister | 重启移除驻留状态 |
| `-100` | `ERR_APP_MISSING` | 注册后 lookup 不到 App，或已标记注册但固件表不匹配 | 核对 App ABI、worker 时序和旧状态 |
| `-101` | `ERR_APP_CONFLICT` | App ID `0x00cd` 被不同 package 占用 | 不覆盖冲突项，分析/恢复注册表 |
| `-102` | `ERR_APP_NOT_REGISTERED` | install stage 2 在有效 stage 1 前执行 | 恢复正确 Run 顺序 |
| `-103` | `ERR_BAD_STAGE` | native install 函数收到 1/2 以外 stage | 修正调用者；Supervisor 通常先以 -22 拒绝 |

`-19` 只存在于 Lua `describe_error` 的展示映射，当前 C 实现没有显式生成它。不能把安装器显示的“device unavailable”自动视为 Supervisor word 9 的实际来源。

constructor 的 `register_driver` 原始返回会先写入 word 9；首次命令后 word 9 被该命令结果覆盖。诊断 driver 注册失败应在执行其他控制命令前保存初始快照。

## 3. 文件系统结果码

这些枚举定义在 `module/include/shellpp_native_fs.h`，通常由 native UI 转换为页面状态文案，不保证出现在 Supervisor word 9。

| 值 | 符号 | 语义 |
| ---: | --- | --- |
| 0 | `SHELLPP_FS_OK` | 成功 |
| -200 | `SHELLPP_FS_ERR_ARGUMENT` | 空指针、容量、格式或一般参数无效 |
| -201 | `SHELLPP_FS_ERR_PATH` | 路径无效、越界、组件或深度不安全 |
| -202 | `SHELLPP_FS_ERR_OPEN` | 文件/目录打开失败或必需目标缺失 |
| -203 | `SHELLPP_FS_ERR_READ` | 读取失败 |
| -204 | `SHELLPP_FS_ERR_WRITE` | 写入失败或 short write |
| -205 | `SHELLPP_FS_ERR_CLOSE` | close 失败 |
| -206 | `SHELLPP_FS_ERR_SEEK` | seek 失败或 offset 无效 |
| -207 | `SHELLPP_FS_ERR_TOO_LARGE` | 文件或中间表示超出支持上限 |
| -208 | `SHELLPP_FS_ERR_RENAME` | rename、move 或原子提交失败 |
| -209 | `SHELLPP_FS_ERR_DELETE` | unlink、rmdir 或删除流程失败 |
| -210 | `SHELLPP_FS_ERR_DIRECTORY` | 目录读取、遍历或完整性失败 |
| -211 | `SHELLPP_FS_ERR_NOT_EDITABLE` | 目标不满足文本编辑条件 |
| -212 | `SHELLPP_FS_ERR_SAME_PATH` | 源和目标相同 |
| -213 | `SHELLPP_FS_ERR_TRUNCATED` | 输入/解析被截断，拒绝有损写回 |
| -214 | `SHELLPP_FS_ERR_UNSAFE_TYPE` | 链接、device 或未知类型不允许该操作 |
| -215 | `SHELLPP_FS_ERR_EXISTS` | 目标已存在且操作不允许覆盖 |

一个高层操作可能返回最后或首个代表性错误，同时报告结构保存更细的 deleted/failed/skipped 计数。排障时保留页面文案、路径、计数和底层错误，不只保存一个整数。

## 4. 固件原始诊断值

| Status word | 固件调用 | 项目如何使用 |
| ---: | --- | --- |
| 18 | `APP_INSTALL` | 保存原始结果；以 lookup 与 package 匹配确认注册 |
| 19 | `LAUNCHER_ADD` | 保存原始结果；发布调用只执行一次 |
| 20 | `NOTIFICATION_SUBMIT` | 保存原始结果；通知提交只执行一次 |

这些入口的返回约定没有被项目统一映射为 errno。记录时必须同时保存固件版本、bin 哈希、command/stage、words 13 至 17 和原始值。禁止用 `-100..-215` 表格解释它们。

## 5. Lua 安装器错误

下列错误是 Lua 自己生成的文本，不是 native errno：

- 无法读取 `ro.build.version`；
- 精确版本 bin 不存在；
- bin 不是 ELF32 little-endian ARM ET_REL，或大小越界；
- 图标资源格式、尺寸、写入或回读校验失败；
- `insmod` shell 命令失败；
- `/dev/shellpp` 不存在；
- status magic/ABI/firmware code 不匹配；
- LuaLVGL timer 创建或 callback 出错；
- staging 环境删除失败。

这些文本需要结合是否已执行 `insmod`、是否能读取 status 和设备日志定位，不能反向推断某个固件 ABI 函数返回了同名错误。

## 6. 恢复边界

- `-95` 与 `-16` 是悬空 callback 防护，不应通过强制 unload 绕过。
- 文件系统写入错误后先保留临时文件和原注册表证据，不重复操作扩大损坏。
- firmware raw 结果异常但项目 flag 已置位时，保存完整快照并验证实际固件状态，不擅自改成 errno 判断。
- 驻留 target、页面数或 status ABI 不匹配时重启；覆盖磁盘 bin 无法修复 RAM 中旧 module。
