# 目标 profile 模式

本页逐项描述构建器 `generate_target_abi.py` 当前接受的 profile。它是机器可读 ABI 数值的 schema，不是完整 ABI 的全部内容；函数原型、结构布局、线程、所有权和时序仍必须在[固件 ABI](../firmware-abi.md)所述证据中恢复。

## 1. 文件位置与语法

profile 位于构建器：

`targets/<TARGET_ID>.env`

解析器逐行接受：空行、去除首尾空白后以 `#` 开头的注释、或一个无引号的 `KEY=VALUE`。规则如下：

- key 必须匹配 `[A-Z][A-Z0-9_]*`；
- value 不能为空；
- value 中不能含任何空白；
- value 中禁止双/单引号、反斜杠、反引号、美元、分号、`&|<>`、括号、花括号、方括号、星号、问号和感叹号；
- 同一 key 不得重复；
- `METADATA_KEYS + ABI_KEYS` 中每项都必须存在；
- schema 之外的未知 key 在 validate 阶段被拒绝。

生成器先严格解析，成功后 `build.sh` 才以 shell assignment 读取同一文件。不要使用变量展开、引号、命令替换或相对当前目录的隐式值。

## 2. 元数据字段

`METADATA_KEYS` 的实际顺序和约束：

| Key | 格式 | 生成器校验 | 后续构建使用 |
| --- | --- | --- | --- |
| `TARGET_ID` | `[a-z0-9][a-z0-9.-]*` | 严格正则 | profile 选择、out 目录、patch 目录 |
| `FIRMWARE_VERSION` | `major.minor.patch` | 十进制三段；minor/patch <= 999 | bin 文件名和 target 元数据 |
| `FIRMWARE_CODE` | uint32 | 必须等于 `major*1000000+minor*1000+patch` | status word 12 |
| `FIRMWARE_IMAGE` | 无空白字符串 | 生成器不检查存在/绝对性 | `build.sh` 检查存在并读取 |
| `FIRMWARE_IMAGE_SIZE` | uint32 | 范围 `0..0xffffffff` | 必须与实际字节数完全一致 |
| `FIRMWARE_IMAGE_SHA256` | 64 个小写 hex | 格式严格 | 必须与实际 SHA-256 完全一致 |
| `CPU` | 无空白字符串 | 不验证 clang CPU 列表 | 传给 `-mcpu`；当前为 `cortex-m33` |
| `FLOAT_ABI` | 无空白字符串 | 不验证允许集合 | 传给 `-mfloat-abi`；当前为 `soft` |
| `MAX_LOADED_SIZE` | uint32 | 仅数值范围 | verifier 要求 SHF_ALLOC footprint 严格小于 |
| `MAX_BSS_SIZE` | uint32 | 仅数值范围 | verifier 允许 `.bss <= limit` |

“路径必须绝对”“CPU/float ABI 必须是当前工具链支持值”“上限必须来自 loader/设备证据”是项目政策和构建约束，不是生成器自身全部能够证明的条件。文档和错误信息必须区分这两层。

## 3. 函数地址字段

下列 `ABI_KEYS` 以 `_ADDR` 结尾且名称不含 `STYLE_`，生成器把它们全部视为函数地址：值必须是非零 uint32 且最低位为 1。

| 分组 | Keys |
| --- | --- |
| driver | `ABI_REGISTER_DRIVER_ADDR`, `ABI_UNREGISTER_DRIVER_ADDR` |
| file | `ABI_OPEN_ADDR`, `ABI_READ_ADDR`, `ABI_WRITE_ADDR`, `ABI_CLOSE_ADDR`, `ABI_LSEEK_ADDR`, `ABI_UNLINK_ADDR`, `ABI_RENAME_ADDR` |
| directory | `ABI_OPENDIR_ADDR`, `ABI_CLOSEDIR_ADDR`, `ABI_READDIR_ADDR`, `ABI_RMDIR_ADDR` |
| App/Launcher | `ABI_APP_LOOKUP_ADDR`, `ABI_APP_INSTALL_ADDR`, `ABI_LAUNCHER_ADD_ADDR`, `ABI_NOTIFICATION_SUBMIT_ADDR` |
| content/text | `ABI_LVX_CONTENT_CREATE_ADDR`, `ABI_LVX_PAGE_TITLE_CREATE_ADDR`, `ABI_LVX_LABEL_CREATE_ADDR`, `ABI_LVX_LABEL_SET_TEXT_ADDR` |
| display/timer | `ABI_LV_DISPLAY_GET_LAYER_TOP_ADDR`, `ABI_LV_TIMER_CREATE_ADDR`, `ABI_LV_TIMER_DELETE_ADDR` |
| object layout | `ABI_LVX_OBJECT_SET_SIZE_ADDR`, `ABI_LVX_OBJECT_ALIGN_ADDR`, `ABI_LVX_ALIGN_TO_ADDR`, `ABI_LVX_SET_HIDDEN_ADDR`, `ABI_LVX_STYLE_APPLY_ADDR` |
| list/event | `ABI_LVX_LIST_ROW_CREATE_ADDR`, `ABI_LVX_LIST_ROW_UPDATE_ADDR`, `ABI_LVX_LIST_ROW_TRAILING_ADDR`, `ABI_LVX_EVENT_ADD_ADDR`, `ABI_LVX_EVENT_GET_USER_DATA_ADDR`, `ABI_LVX_EVENT_GET_CODE_ADDR` |
| Activity | `ABI_ACTIVITY_NAVIGATE_ADDR`, `ABI_ACTIVITY_FINISH_ADDR` |
| spawn/restart | `ABI_POSIX_SPAWN_ADDR`, `ABI_FILE_ACTIONS_INIT_ADDR`, `ABI_FILE_ACTIONS_ADDOPEN_ADDR`, `ABI_FILE_ACTIONS_DESTROY_ADDR`, `ABI_SPAWNATTR_INIT_ADDR`, `ABI_SPAWNATTR_DESTROY_ADDR`, `ABI_WAITPID_ADDR`, `ABI_SOFT_RESTART_ADDR` |

最低位置 1 只证明 Thumb 形式。它不证明地址位于函数入口、对应正确符号、原型一致或调用线程安全。profile 必须填写设备运行时地址；逆向工具的 `0x2c...` analysis mapping 不能直接使用。

## 4. 固件数据地址

| Key | 生成器约束 | 当前用途 |
| --- | --- | --- |
| `ABI_STYLE_MISANS_DEMIBOLD_32_ADDR` | uint32，4 B 对齐；允许数值 0 | 固件 MiSans Demibold 32 style 对象 |

生成器通过字段名含 `STYLE_` 将其排除在函数地址检查之外；专用 ELF verifier 则通过明确的 `DATA_ADDRESS_MACROS` 集合把它归入数据地址白名单。新增其他数据地址不能仅仿照命名，必须同步生成器分类、verifier 集合、所有 profile 和文档。

项目的完整 ABI 政策要求实际使用的数据对象有非占位证据。生成器当前只检查对齐，不应把“允许 0”解释为发布 profile 可以使用空占位值。

## 5. 固件常量、枚举与结构尺寸

| Key | 含义 | 生成器额外约束 |
| --- | --- | --- |
| `ABI_O_RDONLY` | 只读 open flag | uint32 |
| `ABI_O_WRONLY` | 只写 open flag | uint32 |
| `ABI_O_CREAT` | create flag | uint32 |
| `ABI_O_TRUNC` | truncate flag | uint32 |
| `ABI_SEEK_SET` | seek-start | uint32 |
| `ABI_SEEK_END` | seek-end | uint32 |
| `ABI_DT_DIR` | dirent 目录类型 | uint32 |
| `ABI_DT_REG` | dirent 普通文件类型 | uint32 |
| `ABI_DT_LNK` | dirent 链接类型 | uint32 |
| `ABI_APP_DESCRIPTOR_SIZE` | App descriptor bytes | 非零、4 B 倍数 |
| `ABI_PAGE_DESCRIPTOR_SIZE` | Page descriptor bytes | 非零、4 B 倍数 |
| `ABI_ALIGN_TOP_MID` | LVX align enum | uint32 |
| `ABI_ALIGN_TOP_LEFT` | LVX align enum | uint32 |
| `ABI_ALIGN_OUT_BOTTOM_MID` | LVX align enum | uint32 |
| `ABI_EVENT_CLICKED` | click event code | uint32 |
| `ABI_TRAILING_NONE` | list trailing enum | uint32 |

除两个 descriptor size 外，生成器不验证枚举之间的关系、是否非零或是否符合通用 POSIX/LVGL 值。这些值即使在现有两个固件中相同，也必须按新固件完整 ABI 独立确认。

## 6. 生成输出

每个 profile 生成 `out/<target>/generated/shellpp_target_abi.h`，包含：

- include guard `SHELLPP_TARGET_ABI_H`；
- `SHELLPP_TARGET_ABI_GENERATED 1`；
- 字符串 `SHELLPP_TARGET_ID`；
- 字符串 `SHELLPP_TARGET_FIRMWARE_VERSION`；
- 十进制 `SHELLPP_ABI_FIRMWARE_CODE`；
- 每个 `ABI_*` key 对应的十六进制 `SHELLPP_ABI_*` 宏。

生成头是可重建产物，不能手工编辑或提交为独立事实来源。共享 `shellpp_firmware_abi.h` 只接受带 generated marker 的头，直接绕过构建器编译应失败。

## 7. schema 扩展

新增字段时按以下顺序：

1. 在 `METADATA_KEYS` 或 `ABI_KEYS` 明确加入字段。
2. 定义类型、范围、函数/数据分类和完整 ABI 证据要求。
3. 为所有现有 profile 提供经验证值，不引入按旧固件默认值。
4. 更新生成宏消费者和共享 typedef/结构。
5. 对绝对地址同步 `verify_shellpp_elf.py` 的分类和白名单行为。
6. 增加合法/非法 profile 测试或手工负例。
7. 更新本页、适配指南和 ABI 文档。
8. 执行全目标构建和相应真机回归。

不能通过可选字段或 `#ifndef old-value` 隐藏 ABI 缺口；否则新增 target 可能悄然继承错误旧值。

## 8. 新 profile 审计

```text
[ ] 固件镜像路径、大小和 SHA-256 与 ABI 分析来源相同
[ ] METADATA_KEYS 全部存在且无未知/重复项
[ ] ABI_KEYS 全部存在，且每项有证据
[ ] 函数地址是非零 Thumb 运行时入口
[ ] 数据地址分类和对齐正确，不使用占位值
[ ] 函数原型、descriptor offset、dirent、时序另有完整记录
[ ] target ID 与文件名、patch 目录、版本化 bin 一致
[ ] loaded/BSS 上限来自 loader 或已验证基线
[ ] 生成器通过后仍执行镜像、ELF、打包和真机 gate
```
