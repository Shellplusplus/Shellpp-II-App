# 应用管理实现

## 1. 范围

本篇说明 native UI 如何读取 Xiaomi Band 10 Pro 的应用注册表、建立列表、隐藏/显示/卸载应用、计算和删除应用数据，以及 036 与 043 的差异。应用管理包含不可逆文件删除和注册表改写；只读列表成功不能作为写操作安全的证据。

## 2. 页面入口差异

共享 036 基线中，page 1 `文件与应用管理` 在 menu 与 app list 两种模式之间切换。进入应用管理会清空与文件浏览器互斥的 workspace，并在同一 Activity 页面重新渲染应用列表。

043 通过专用补丁注册 page 8 `shellpp-apps`：

- page 1 只显示文件、应用、缓存三个入口；
- 点击应用管理调用 Activity navigate 到 page key `(0x00cd << 16) | 8`；
- page 8 标题为“应用管理”；
- create page 8 时进入 app list mode；
- back 通过 Activity finish 关闭独立页面。

这一差异不能通过只改标题字符串实现，必须同步 App descriptor page count、page name、UI count、导航 key 和 BSS 分配。该差异只存在于 043 target，不修改 036 共享基线。

## 3. 注册表路径

| 类型 | 路径 |
| --- | --- |
| 可见应用 | `/data/apps.json` |
| 隐藏应用 | `/data/apps.json_hide` |

043 的路径和读取逻辑来自 `0007-match-shell-plus-plus-lua-app-paths.patch`，以同机型 Shell++ Lua 实现为只读参考。`/data/quickapp/` 可能包含应用数据，但不是当前 043 注册表文件路径；此前把它当成注册表根会导致“无法读取固件应用注册表”。

## 4. 固定容量

| 限制 | 当前值 |
| --- | ---: |
| 最大列表项 | 256 |
| 每页应用 | 16 |
| display name 缓冲 | 80 字节 |
| package 缓冲 | 96 字节 |
| JSON key 缓冲 | 128 字节 |
| registry text workspace | 7808 字节 |
| 单个 captured object | 3996 字节 |
| selection bitmap | 32 字节 |

每个列表项只保存 name/package 在 workspace 中的 offset，以及 hidden、locked、valid、source registry。原始 JSON object 不常驻；改写时重新流式解析磁盘文件。

达到项目数、workspace 或 object 容量上限时，列表标为 truncated。truncated 列表仍可显示已成功解析的项目，但禁止 hide-all/show-all 等 bulk 操作，因为未显示条目不能安全纳入选择。

## 5. 流式 JSON parser

实现不依赖动态内存或通用 JSON 库，而是包含固定容量的流式 parser：

- 支持 whitespace、object、array、string、boolean 和一般 JSON value 的复制或跳过；
- 处理字符串 escape，并对 display/package 做容量检查；
- 可以将一个应用 object 原样 capture 到固定 workspace；
- 改写时复制不认识的顶层 key/value 和未操作 object；
- 对被移动对象只重建 `hideFlag`，保留其他字段；
- 输入可以是固件文件 reader 或内存 captured object。

无法解析、不是 object、缺少必需身份或超过容量的记录不会暴露给批量操作；磁盘改写时尽量原样保留，并将列表标为 truncated。显示名缺失时使用 package 作为 fallback。

## 6. 受保护应用

代码只强制保护 resident native App package `com.shellpp.ii`。该项不能选择、隐藏、显示或卸载。注册表 object 自身的 locked 标志也会使条目不可选择。

这不是完整的系统关键应用白名单。旧 Xiaomi Vela Shell++ 没有被额外硬编码保护，可能作为普通可管理项目出现。

## 7. 036 读取语义

共享 036 基线：

1. 先通过父目录枚举判断文件是否存在且为普通文件；
2. visible registry 是必需输入；
3. hidden registry 可选；
4. JSON 必须是 object；
5. 必须恰好存在一个有效 `InstalledApps` array；
6. 缺失、空、无效或缺少 `InstalledApps` 会报告读取失败。

这是已验证 036 的既有逻辑。不要为匹配 043 的宽松读取行为修改共享源码或 036 target。

## 8. 043 读取语义

043 直接 `open()` 两个注册表，不先枚举父目录。列表读取与 Shell++ Lua 对齐：

- 文件缺失：该 registry 视为空；
- 文件为空或只有空白：视为空；
- JSON 非 object、重复关键字段或内部解析失败：回滚本 registry 已追加的条目，并将本 registry 视为空；
- 没有 `InstalledApps`：视为空；
- 一个 registry 无效不会丢弃另一个已成功读取的列表项。

**宽松归一化只用于列表读取。** 它不表示无效 JSON 可以安全写回，也不表示所有缺失 registry 都能由写操作自动创建。

## 9. Reload、选择和分页

reload 先清空 metadata、text workspace、selection 和状态，再读取 visible 与 hidden registry，标记 source/hidden，最后设置 loaded。043 的 reader 可能成功返回空列表，因此“页面无报错且显示 0 项”仍需核对真实文件内容，不能直接断言设备没有应用。

选择规则：

- 每页最多 16 项；
- selection 使用 256 位 bitmap；
- locked/protected 项不会进入 selection；
- 全选只影响可选择项；
- hide 只针对 visible source；
- show 只针对 hidden source；
- delete 可针对两个 source；
- truncated 时禁止 all 操作，但允许明确选择已知条目。

每项还显示估算数据大小。大小读取失败显示未知，不阻止只读列表展示。

## 10. 单文件原子改写

每次改写一个 registry 时使用 `<path>.shellpp.tmp`：

1. 打开 source reader；
2. begin 临时 writer；
3. 流式复制所有顶层 key/value；
4. 对 `InstalledApps` 选择性删除、保留或追加 object；
5. 关闭完整 JSON object；
6. 关闭 reader；
7. commit 通过 rename 替换目标；
8. 任一步失败 abort 并删除临时文件。

未知顶层字段和未操作 object 被复制。hide/show 的 object 会移除旧 `hideFlag`，再按目标状态决定是否写入 `\"hideFlag\":true`。

原子性仅针对单个 registry。hide/show 需要依次改写两个文件，因此不是跨文件事务；中间失败可能留下目标已追加、source 尚未删除的重复状态。

## 11. 隐藏应用

hide 使用 target-first 顺序：

1. 先改写 hidden registry，将选中的 visible object 追加进去并设置 `hideFlag:true`；
2. target 成功后，再改写 visible registry，删除这些 object；
3. reload 并提示重启后生效。

如果第 2 步失败，hidden 已保存 object，visible 仍有原 object。这通常形成重复而不是丢失。恢复时应先备份两个文件，再按 package 处理重复。

hidden registry 不存在时，hide 允许创建最小结构：

```json
{"InstalledApps":[...]}
```

## 12. 显示应用

show 同样 target-first：

1. 将 hidden 选中 object 追加到 visible registry，并移除 `hideFlag`；
2. 再从 hidden registry 删除这些 object；
3. reload 并提示重启后生效。

visible registry 的缺失策略与可选 hidden registry 不同。实际创建权限以 `app_rewrite_registry()` 的 allow-missing 参数为准，不能从 043 宽松读取语义推导。

## 13. 卸载应用

卸载需要选中项目并连续点击两次确认：

1. 从 visible registry 删除目标 object；
2. 从 hidden registry 删除目标 object；
3. 对每个目标 package 删除受限数据目录；
4. reload；
5. 建议重启。

两个 registry 独立原子替换。若第二个 registry 失败，第一个可能已经改变。若 registry 删除成功而数据目录删除失败，应用身份已移除但部分数据仍残留，不能报告完全卸载。

这里的应用卸载不是 Supervisor/native App 的 live uninstall。`com.shellpp.ii` 被保护，Shell++ II 自身仍通过安装器清理环境后重启移除。

## 14. Package 路径校验

应用目录只接受单个 package component：长度 1..71 字节，字符限 ASCII 字母、数字、`.`、`_`、`-`。`/`、空字符串和超长名称被拒绝。

该校验防止注册表 package 构造任意路径或 `..` 穿越。未经校验的字符串不能传入递归删除。

## 15. 应用大小路径

036 共享基线统计：

- `/data/app`；
- `/data/quickapp/system`；
- `/data/quickapp/files`。

043 通过 Shell++ Lua 参考逻辑对齐为：

- `/data/app`；
- `/data/quickapp/system`；
- `/data/cache`；
- `/data/files`；
- `/data/mass`。

代码统计每个 `<root>/<validated-package>` 目录内普通文件，深度最大 12，不跟随链接，并对 32 位总量做饱和累加。

## 16. 应用删除路径

`shellpp_fs_delete_app_package()` 对以下 roots 删除 `<root>/<package>`：

- `/data/app`；
- `/data/quickapp/system`；
- `/data/cache`；
- `/data/files`；
- `/data/mass`。

删除规则：root 和 package 路径必须是目录；普通文件 unlink；链接只 unlink 本身；未知/device 类型失败；超过深度上限失败；子树完成后删除目录。043 用固定深度显式 cursor stack 替代递归 C 调用。

这是不可恢复操作，只能用专用测试 package 或有完整备份的设备验证。

## 17. 生效与重启

hide/show 的状态文案明确要求重启后生效。应用管理页提供二次确认的 soft restart。卸载后也建议重启，使固件内存状态与磁盘 registry 一致。

registry 写入后当前 Launcher 未立即变化不等于写入失败；固件可能只在启动或特定刷新阶段重新读取 registry。

## 18. 失败恢复

写操作前保存：

```text
/data/apps.json
/data/apps.json_hide（如果存在）
目标 package 在全部数据 root 下的目录清单
```

发生失败后：

1. 不继续执行另一轮 hide/show/delete；
2. 保存两个当前 registry；
3. 检查 `.shellpp.tmp` 残留；
4. 按 package 比较两个 `InstalledApps` 的重复、缺失和 `hideFlag`；
5. 只有结构可解析时才人工恢复；
6. 数据目录已删除时，registry 备份不能恢复数据；
7. 恢复后重启并重新验证。

target-first hide/show 的部分失败通常产生重复。卸载部分失败可能已造成不可逆数据删除，不能自动重试整个操作。

## 19. 主机测试边界

043 UI host test 使用实际重放七个补丁后的 `native_ui.c`，mock Activity/UI/file ABI，覆盖 page 8、标题、导航、两个 registry 的直接 open，以及正常、缺失、空、无效 JSON 的读取归一化。

文件系统 host test 使用 ASan/UBSan 覆盖目标补丁后的路径、遍历和文件操作逻辑。两者都不能证明真实固件地址、dirent、Activity object、线程栈、registry 字段或断电行为。

## 20. 常见陷阱

- 把 `/data/quickapp/` 当作注册表文件；
- 通过目录枚举判断 043 平面 registry 是否存在；
- 把空列表直接解释为设备无应用；
- 将宽松读取语义套用到写入无效 JSON；
- 使用字符串替换代替结构化 parser；
- 认为两个 rename 构成跨文件事务；
- truncated 列表仍允许 bulk operation；
- 未校验 package 就拼接删除路径；
- 跟随符号链接；
- 只改标题而未注册 page 8；
- 增加第九页后超出 BSS；
- 将 043 页面或路径修复合并进 036；
- registry 已移除但目录失败时仍报告完整卸载；
- 无备份重复执行失败操作。

## 21. 新固件适配检查

至少确认 registry 精确路径、直接 open 语义、`InstalledApps` 和 object 字段、缺失/空/无效行为、大小与删除 roots、registry reload 时机、rename 语义、dirent、链接类型、受保护 package、独立页面需求以及 hide/show/delete 的部分失败恢复。

这些都是完整 ABI 和运行时数据契约的一部分；只有文件函数地址不足以完成应用管理适配。
