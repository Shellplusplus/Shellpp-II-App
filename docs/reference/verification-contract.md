# Shell++ ELF 验证契约

本页精确描述构建器 `verify_shellpp_elf.py` 的当前检查范围。通用 `verify_elf.py` 不属于 Shell++ II 发布 gate，不能替代本脚本。

## 1. 调用方式

```sh
python3 verify_shellpp_elf.py \
  --abi-header out/<target>/generated/shellpp_target_abi.h \
  --max-loaded-size <MAX_LOADED_SIZE> \
  --max-bss-size <MAX_BSS_SIZE> \
  out/<target>/shellpp_ii-<version>.bin
```

ABI header 中匹配 `SHELLPP_ABI_*_ADDR` 的宏形成绝对地址白名单。`SHELLPP_ABI_STYLE_MISANS_DEMIBOLD_32_ADDR` 是当前唯一显式数据地址；其他匹配项都按函数地址处理。header 必须至少产生一个函数地址和一个数据地址，所有函数地址必须为奇数。

## 2. ELF header gate

验证器按 little-endian struct 读取 ELF32 header，并要求：

- magic 为 `0x7fELF`；
- class = ELFCLASS32；
- data = little-endian；
- ident version = 1；
- OS ABI = System V；
- `e_type = ET_REL (1)`；
- `e_machine = EM_ARM (40)`；
- ELF version = 1；
- `e_flags` 高字节表示 ARM EABI5；
- `e_flags & 0x00ffffff` 只允许 0 或 `0x200`；
- ELF header size 等于脚本结构大小；
- `e_phoff = 0` 且 program header count = 0；
- section header entry size、数量、string-table index 与文件范围有效；
- `e_entry = 0`。

`.bin` 只是文件命名。产物本质是 ARM EABI5 ELF32 ET_REL module，不是裸 bin 或 ET_EXEC。

## 3. section table 与符号

所有非-NOBITS section 的 offset/size 必须落在文件内；section-name table 必须为字符串表且字符串以 NUL 结束。

符号条件：

- 恰好存在一个 SHT_SYMTAB，名称必须是 `.symtab`；
- entry size、关联 string table 和表大小必须合法；
- 除 null symbol 外不允许任何 SHN_UNDEF symbol；
- 名为 `module_initialize` 的 symbol 恰好一个；
- 该 symbol 必须已定义且类型为 STT_FUNC。

“没有 undefined import”只证明 module 自包含所有符号引用或使用绝对地址，不证明绝对目标 ABI 正确。

## 4. allocated section 白名单

必须存在：

| Section | flags/type |
| --- | --- |
| `.text` | ALLOC + EXEC，非 NOBITS |
| `.rodata` | ALLOC，非 NOBITS |
| `.data` | ALLOC + WRITE，非 NOBITS |
| `.bss` | ALLOC + WRITE，NOBITS |

允许的其他 SHF_ALLOC section 只有 `.ARM.exidx` 和 `.init_array`。任何额外 allocated section 失败；只要存在名为 `.preinit_array` 的 section 就失败，无论其 flags。

脚本没有要求 `.ARM.exidx` 或 `.init_array` 必须存在，也没有禁止任意非 allocated 调试/字符串/relocation section；这些仍要满足结构与 relocation 规则。

## 5. footprint 与 BSS

SHF_ALLOC footprint 按 section table 顺序，对每个 allocated section先按 `max(sh_addralign,1)` 对齐累计值，再加 `sh_size`。它不是简单文件大小，也不是各 section size 无对齐相加。

- `loaded_size >= MAX_LOADED_SIZE` 失败，即必须严格 `loaded_size < limit`；
- `.bss size > MAX_BSS_SIZE` 失败，即允许 `.bss == limit`。

两种比较不能写成同一个 `<=` 或 `<` 规则。提高 profile 上限必须有 loader/设备内存证据，不能用于绕过增长回归。

## 6. relocation gate

脚本先收集所有 SHT_REL 的目标 offset，供 literal 扫描跳过 relocation 覆盖 word；随后验证每条 relocation：

- 任意 SHT_RELA 立即失败；
- SHT_REL entry size/总大小合法；
- `sh_info` 指向有效且非零的目标 section；
- `sh_link` 指向有效 SHT_SYMTAB；
- 目标 section 必须 SHF_ALLOC；
- relocation 写入的 4 B 不越出目标；
- symbol index 有效，引用 symbol 不得 undefined；
- 默认只允许 `R_ARM_ABS32 = 2`；
- `.rel.ARM.exidx` 额外允许 type 0 和 42；
- `.rel.init_array` 额外允许 `R_ARM_TARGET1 = 38`。

白名单按 relocation section 名称区分。编译器产生新类型不是放宽理由，必须先证明目标 NuttX module loader 支持。

## 7. 绝对地址扫描

扫描范围不是整个文件：

1. 从 `.text` 中 ARM mapping symbol `$d` / `$d.*` 标记的 data range 扫描 4 B 对齐 word；
2. 扫描整个 `.data` 的 4 B 对齐 word；
3. 跳过有 relocation 覆盖的 word；
4. 不扫描 `.rodata`、普通 Thumb instruction bytes、`.bss` 或非 allocated section。

对被扫描的直接 32 位值：

- `0x2c000000 <= value < 0x2d000000`：作为 analysis-only firmware address 失败；
- `0x0c000000 <= value < 0x0d000000`：必须为奇数且在函数白名单；
- `0x20000000 <= value < 0x21000000`：除 `0x20200000..0x202fffff` 外必须在数据白名单；
- 值若正好等于任一白名单地址，会计入“direct target ABI literals”集合。

已知限制：Clang 可把相邻调用目标折叠为一个 literal base 加立即数，所以被使用的函数地址未必以完整 word 出现；直接 literal 计数不等于源码使用的 ABI 数量。扫描也不进行完整 ARM/Thumb 数据流分析，未覆盖范围不能被解释为“绝对不存在硬编码地址”。源码审查和 generated-only ABI 设计仍是必要 gate。

## 8. 成功输出

退出 0 时输出：

- 模块路径和 OK；
- file size；
- SHF_ALLOC total；
- `.bss`；
- zero ELF entry；
- `module_initialize` symbol value；
- 除 null symbol 外的 symbol 数；
- direct target ABI literal 数；
- 实际出现的 relocation type 集合；
- 按 section table 顺序的 section 名称。

发布记录应保存完整输出和退出码。只保留 `OK` 行会丢失尺寸、符号和 section 回归证据。

## 9. 不在证明范围内

验证器不能证明：

- profile 地址对应预期函数或数据；
- 地址来自正确哈希的固件镜像；
- typedef 与真实固件原型一致；
- App/Page/notification/dirent offset 正确；
- constructor、worker、Activity、timer 和 callback 时序正确；
- 调用链在设备线程栈内安全；
- 文件与注册表写入可恢复；
- Lua 选择了该 bin；
- 两个资源目录或 `resource.bin` 包含该字节；
- 真机在任何页面或功能路径上可用。

这些分别由固件 ABI 证据、镜像身份、host tests、打包比较和真机矩阵承担。ELF gate 通过只构成构建验证。

## 10. 维护负例

修改 verifier 后至少覆盖相关负例：

- 偶数函数地址或空函数集合；
- 空数据地址集合；
- 直接 `0x2c...` literal；
- 非白名单 `0x0c...` 或 data literal；
- ELF class/endian/type/machine/EABI/entry/program header 错误；
- undefined symbol、缺失或重复 entry；
- 缺失必需 section或额外 allocated section；
- `.preinit_array`；
- RELA、越界 relocation、无效 symbol/target、未支持 type；
- footprint 恰好等于 loaded limit；
- BSS 恰好等于 limit 应通过，超过 1 B 应失败。

负例在临时 fixture 上执行，不修改或破坏已部署 bin。
