# AI 交接与执行规范

本页面向接手 Shell++ II 任务的自动化代理，定义必须读取的事实、修改位置、禁止项、证据格式、停止条件和完成审计。代理必须检查当前工作树，不得用聊天记忆或旧文档替代当前源码与工具。

## 1. 首要准入条件

> 同一机型的新系统适配必须先取得该系统的完整固件 ABI。

“完整 ABI”至少包括 profile schema 中全部函数地址、数据地址、常量、枚举和 descriptor size，也包括源码实际依赖的函数原型、结构字段 offset、dirent 布局、线程与异步时序、参数所有权和生命周期约束。只有固件文件、相邻版本 ABI、若干符号或地址差值时，只能继续固件分析和建立缺口表，不能创建发布 target、不能填写占位值、不能部署 bin，也不能声称适配完成。

## 2. 必读顺序

1. 对三个仓库分别执行 `git status --short`，识别用户已有修改。
2. 阅读 `docs/README.md`、`docs/current-targets.md` 和与当前任务有关的专题。
3. 阅读构建器 `build.sh`，确认真实路径、编译输入、target 选择、部署和打包语义。
4. 阅读相关 `targets/*.env` 和 target patch，确认 ABI 数值和固件专用差异。
5. 阅读 `generate_target_abi.py` 与 `verify_shellpp_elf.py`，确认 schema 和静态 gate。
6. 阅读当前五个编译输入：`module_prelude.S`、`supervisor.c`、`native_app.c`、`native_fs.c`、`native_ui.c`。
7. 阅读共享 headers，尤其是 firmware ABI、native status、文件系统和 UI 接口。
8. 阅读安装器 `_Lua/main.lua`，确认版本选择、状态 word、控制消息和 Run 顺序。
9. 按任务需要读取固定镜像、完整 ABI、日志或只读参考实现。

`module.c` 和 `app_payload.c` 是 legacy，不参与当前构建，不能作为运行行为来源。

## 3. 三仓库职责

| 仓库 | 当前路径 | 规范职责 | 不应放入 |
| --- | --- | --- | --- |
| nativeApp | `/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii` | 一套 canonical C/汇编、共享 headers、中央 `docs/` | 逐固件硬编码地址、永久 target 源码副本 |
| 构建器 | `/Users/ikun_cxkpro/Projects/Shell++/shellpp-ii-build` | profile、target patch、ABI 生成、编译链接、ELF gate、部署打包 | 设备运行期版本分支、第二套 canonical nativeApp |
| 安装器 | `/Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-installer` | 一个 Lua、多个版本化 bin、图标和最终资源 | 固件 ABI 地址表、相邻版本 fallback |

路径大小写必须精确。当前构建器的安装器输出根目录必须保持为上表路径。

## 4. 修改决策矩阵

| 需求 | 正常修改位置 | 必须同步检查 |
| --- | --- | --- |
| 新固件，仅 ABI 数值不同 | 新 `targets/<id>.env` | 完整 ABI、镜像身份、全目标构建、真机 |
| 固件语义不同 | `targets/<id>/patches/*.patch` | 差异证据、patch 顺序、共享源码未变、host/真机 |
| 新 ABI 字段 | 生成器、所有 profile、共享 ABI include、消费者、verifier、文档 | 全目标正例与相关负例 |
| 共享功能 | 当前五个编译输入与 headers | 全部固件回归、BSS、调用链栈 |
| Supervisor 协议 | C 端、Lua 端、reference 文档 | ABI version、旧驻留拒绝、全部固件 Run |
| Lua 版本选择 | `main.lua` 与资源同步流程 | `luac -p`、支持/未知版本、resource 重打包 |
| 部署或 resource | `build.sh`、专用工具 | 失败隔离、双目录、反向解包、hash |
| 文档 | nativeApp `docs/` | 链接、当前实现、状态和产物哈希 |

仅地址、常量、枚举或尺寸差异应进入 profile。需要改变 C 控制流、路径语义、页面数量或内存策略，且只适用于一个固件时，建立最小 target patch。不得修改已验证的 036 逻辑来修复 043，也不得创建 036 patch 去补偿本不应进入共享源码的改动。

## 5. 当前不可破坏约束

- 长期维护一套 nativeApp 共享源码。
- 036 是共享行为基线，当前无 target patch。
- 043 的七个 patch 只应用于临时 `target-src`，不得回写 canonical 源。
- 固件绝对地址只来自生成 ABI 头，不能散落在 C 或 Lua 中。
- 函数地址使用设备运行时 Thumb 地址；禁止把 `0x2c...` analysis address 写入 module。
- 无参数构建覆盖全部 profile；共享或全局工具变化后不能只用单目标结果交付。
- 两个安装器 Lua 目录都要同步；`resource.bin` 从 packaged tree 重建。
- 未知固件必须拒绝加载，禁止 fallback。
- 固件仍保存 module callback 时禁止强制卸载；正确清理边界是删除 staging 后重启。
- 不回退、不格式化、不删除与任务无关的用户工作树修改。
- 不把历史日志中的 fault 自动归因给当前产物。

## 6. 新 target 执行流程

```text
精确固件镜像是否存在，并已固定大小和 SHA-256？
  否 -> 停止发布工作，记录缺失输入
  是
  -> 完整 ABI 是否覆盖全部 schema 和隐式布局/时序？
     否 -> 只做 ABI 恢复与缺口记录
     是
     -> 创建严格 profile
     -> 判断差异能否完全由 profile 表达
        是 -> 不修改 nativeApp 或 Lua
        否 -> 建立有证据支持的最小 target patch
     -> 执行全目标构建与 ELF gate
     -> 执行 host、部署和 resource 验证
     -> 重启后执行新固件真机矩阵
     -> 冻结哈希，更新状态、限制和文档
```

新增固件时，常规工作应集中在构建器：新增 profile 和必要 target patch。只有命名协议、状态协议或资源格式真正变化时才修改 Lua；只有所有固件共享的新能力或现有 ABI 表达能力不足时才修改 canonical nativeApp。

## 7. 证据表达格式

结论使用以下标签：

- `源码确认`：当前真实编译输入直接证明；
- `静态恢复`：固定哈希镜像的反汇编、交叉引用或数据流支持；
- `主机验证`：mock ABI 下测试通过；
- `构建验证`：profile、镜像、编译链接和 ELF gate 通过；
- `打包验证`：out、双目录和反向 resource 一致；
- `真机确认`：精确固件与精确 bin 哈希的实际场景通过；
- `假设`：用于继续调查，尚不能写入确定实现；
- `限制`：当前工具或记录不能覆盖。

任何 ABI 地址应附目标固件、镜像 SHA-256、运行时地址、Thumb/data 分类、原型、主要交叉引用、与已知实现的比对和未决风险。发生证据冲突时保留双方来源并降低结论等级，不能选择更容易通过构建的值。

## 8. 验证命令选择

| 变更范围 | 最低验证 |
| --- | --- |
| 纯文档 | 相对链接、过时术语、事实核对、三仓库 `diff --check` |
| target patch、文件系统或 UI | 上述检查加两个 host tests |
| Lua 或发布 | 加 `luac -p`、双目录比较、resource 反向比较 |
| profile、源码、生成器、verifier、linker、构建部署 | 无参数 `./build.sh` 并保存完整输出 |
| 设备行为 | 重启清除旧 module 后执行与影响范围匹配的真机测试 |

没有设备结果时必须明确说明，不能用 host test 或 build 替代。

## 9. 停止写入或发布的条件

出现以下任一项时，停止创建确定 profile 或发布产物，但可以继续非破坏性分析：

- 完整 ABI 缺项或只有猜测值；
- ABI 分析镜像与 profile 镜像哈希不一致；
- 地址只有版本差值，没有独立函数身份证据；
- 原型、结构 offset、dirent 或线程语义互相冲突；
- descriptor/page 布局变化而现有 schema 或 patch 无法安全表达；
- 需要扩大 loader section、relocation 或内存上限但没有 loader 证据；
- 设备发生新崩溃、注册表损坏或恢复状态未知；
- 任务要求改变已验证 036 行为，却没有明确授权或独立 target 通道。

停止时输出缺少的精确证据和下一步可执行检查，不使用占位值推进发布。

## 10. 工作树协作规则

三个仓库可能包含用户或前序任务未提交的修改。代理必须：

1. 修改前读取相关文件和 diff。
2. 只编辑任务所需文件。
3. 不运行 `git reset --hard`、`git checkout --` 或全目录清理。
4. 遇到外部修改时重新读取并与其协作，不覆盖。
5. 在交接中列出自己修改的文件和保留的既有变化。

生成物和安装器资源可能本来就处于 modified/untracked 状态，不能仅凭 Git 状态删除。

## 11. 完成审计

结束重大任务前逐项证明：

```text
[ ] 用户的每个明确要求都有对应文件、代码或命令证据
[ ] 真实编译输入与文档一致
[ ] 新固件有完整 ABI，不含占位值或地址差值猜测
[ ] 036 基线未被目标专用修复改变
[ ] profile、patch 和 schema 的所有权正确
[ ] 全目标构建覆盖全部支持 profile
[ ] 每个 ELF 通过专用 verifier 和资源上限
[ ] Lua 精确选择，未知版本拒绝
[ ] out、editing、packaged 和 resource payload 一致
[ ] hashCode 对应最终 resource.bin
[ ] 真机结论绑定精确固件与 bin 哈希
[ ] 未执行场景没有被写成已验证
[ ] 文档链接、状态、patch 名、路径和哈希无过时项
[ ] 未回退或覆盖无关用户修改
```

只有所有适用项都有权威证据，且没有遗留的用户要求，才能宣告任务完成。

## 12. 交接输出

每次适配或重大变更至少交付：

- 修改文件和每个修改的职责；
- 固件版本、镜像路径、大小和 SHA-256；
- target ID、profile 和 patch 列表；
- bin 文件名、大小、footprint、BSS 和 SHA-256；
- 构建、host、Lua、部署、打包、链接和文档检查结果；
- 真机设备、操作、时间、结果和日志；
- 未验证项目、限制和恢复方式；
- 工作树中保留的非本任务修改。

交接应让下一代理只凭当前仓库和记录即可复现实验，不依赖聊天中未落盘的隐含知识。
