# native App 注册

## 1. 范围

本篇说明 `native_app.c` 如何构造固件 App/Page descriptor、处理异步 registry、发布 Launcher、提交通知以及为什么必须保持 module 驻留。地址数值不在本文复制；它们以各 target profile 为准。

## 2. 固定身份

| 属性 | 当前值 |
| --- | --- |
| App ID | `0x00cd` |
| package | `com.shellpp.ii` |
| display name | `Shell++ II` |
| Launcher icon | `/data/shellpp-ii/shellpp_ii_icon.bin` |
| notification title/source | `Shell++ II` |
| notification body | `Shell++ 已成功加载` |

App ID 和 package 共同用于冲突检测。固件 lookup 只按 App ID 返回对象，Shell++ II 随后读取已安装 descriptor 的 package pointer 并逐字节比较，不能只因为 ID 存在就认为是自己的 App。

## 3. 页面集合

### 3.1 共享 036 基线

共享源码注册八页：

| index | descriptor name |
| ---: | --- |
| 0 | `shellpp-home` |
| 1 | `shellpp-files` |
| 2 | `shellpp-viewer` |
| 3 | `shellpp-cache` |
| 4 | `shellpp-about` |
| 5 | `shellpp-display` |
| 6 | `shellpp-cpu` |
| 7 | `shellpp-restart` |

### 3.2 043 target

043 的 `0006-separate-app-manager-page.patch` 将 page count 改为 9，并增加：

| index | descriptor name |
| ---: | --- |
| 8 | `shellpp-apps` |

该页使应用管理通过 Activity navigation 打开独立页面和“应用管理”标题。它不是共享源码的一部分，不改变 036 的 descriptor 数量。

## 4. descriptor 存储

App descriptor、全部 Page descriptor 和 page pointer 数组都是静态全局存储：

```c
static uint32_t g_app_descriptor[APP_DESCRIPTOR_SIZE / 4];
static uint32_t g_page_descriptors[PAGE_COUNT][PAGE_DESCRIPTOR_SIZE / 4];
static void *g_pages[PAGE_COUNT];
```

尺寸来自 profile 的 `ABI_APP_DESCRIPTOR_SIZE` 和 `ABI_PAGE_DESCRIPTOR_SIZE`，并由 `_Static_assert` 检查。

不能把 descriptor 或字符串改为栈变量。固件在 `APP_INSTALL` 返回后仍会保存 descriptor、字符串和 callback 指针；栈内存离开函数后会变成悬空引用。

## 5. 当前共享布局

### 5.1 App descriptor

| 偏移 | 写入内容 |
| ---: | --- |
| `+0x08` | package name pointer |
| `+0x0c` | icon path pointer |
| `+0x10` | 16 位 App ID |
| `+0x1c` | display-name callback pointer |

其余字节先清零。display-name callback 返回静态 `Shell++ II` 字符串。

### 5.2 Page descriptor

| 偏移 | 写入内容 |
| ---: | --- |
| `+0x10` | page name pointer |
| `+0x14` | 完整 page key |
| `+0x34` | signal callback |
| `+0x4c` | create callback |
| `+0x50` | resume callback |
| `+0x58` | pause callback |
| `+0x5c` | destroy callback |

page key 计算为：

```text
(0x00cd << 16) | page_index
```

这些偏移目前由 036/043 的静态恢复和已验证行为共同支持，但尚未进入 profile schema。若新固件改变布局，不能只增加 descriptor size；必须扩展 schema/生成器或建立有明确证据的 target 语义差异。

## 6. lifecycle callback 转发

固件向 callback 传入 Page descriptor pointer。`page_index_of()` 只接受与静态 descriptor 数组元素精确相同的地址，然后转发：

| 固件 callback | UI 接口 |
| --- | --- |
| signal | 当前返回 0，不消费 payload |
| create | `shellpp_ui_page_create(index, page, root)` |
| resume | `shellpp_ui_page_resume(index, page)` |
| pause | `shellpp_ui_page_pause(index)` |
| destroy | `shellpp_ui_page_destroy(index)` |

create 要求 root 非空。未知 descriptor 或非法 index 返回错误，不尝试从 page key 猜测页面。

## 7. Stage 1：注册 App 和页面

### 7.1 已注册 fast path

如果 module 内 `g_registered` 已为 1，stage 1 再次执行 lookup：

- lookup 存在且 package 匹配：返回成功；
- lookup 缺失或 package 不匹配：返回 App missing。

这不是跨重启持久状态，只是当前 module 驻留期内的幂等保护。

### 7.2 注册前冲突检查

首次注册先用 App ID lookup。lookup 带有限重试，因为固件 registry 由 worker 异步发布：

- 最多 8 次 lookup；
- 每次未命中后执行最多 180,000 次 Thumb-safe `nop` bounded spin；
- 命中后立即停止。

如果已有对象：

- package 相同：视为当前 App 的有效既有项，设置 registered，允许 stage 2；
- package 不同：返回 `-101`，不得覆盖其他 App。

bounded spin 是当前已验证时序补偿，不是通用调度 API。新固件若需要不同 worker 同步机制，应先恢复该机制，不能无限循环或删除校验。

### 7.3 descriptor 初始化

只有 lookup 无现有项时才：

1. 清零 App 和 Page descriptor；
2. 调用 `shellpp_ui_reset()` 清理旧 UI 全局状态；
3. 写入 App descriptor；
4. 为每页写 name、key 和 callback；
5. 填充 page pointer 数组。

### 7.4 APP_INSTALL 和成功判定

调用：

```c
APP_INSTALL(g_app_descriptor, g_pages, PAGE_COUNT)
```

原始返回值保存到 `g_install_result`，但最终注册成功不依赖将该值解释为 POSIX errno。代码随后再次执行带重试 lookup，并要求：

1. App ID 可查到；
2. installed object 的 `+0x08` package pointer 非空；
3. package 与 `com.shellpp.ii` 完全一致。

查不到返回 `-100`，package 冲突返回 `-101`。只有验证通过后才设置 `g_registered = 1`。

## 8. Stage 2：发布 Launcher

stage 2 前置条件：

- module 内 registered flag 为 1；
- lookup 仍可看到 App；
- package 仍匹配。

不满足时分别返回 `-102` 或 `-100`。首次发布调用 profile 中的 `LAUNCHER_ADD(app_id)`，保存原始结果到 `g_launcher_result`，随后设置 published flag。

参考实现将 Launcher 返回值视为 opaque 诊断值，不按 errno 判断失败。当前发布成功的业务前置证据是已注册且 package 匹配的 App。后续重复 stage 2 因 published flag 不再重复调用。

## 9. 加载通知

通知使用一个精确 0x58 字节的 Canopus 兼容静态 record：

| 字段 | 当前值或含义 |
| --- | --- |
| first/class word | `0x50555302`，module notification class |
| second word | `0x43414e4f` |
| title/source/body | 静态 Shell++ II 字符串 |
| icon/small icon | staged icon 路径 |
| `+0x50` flags | 1 |

`_Static_assert` 固定 record size。Shell++ II 每次 module 驻留期只提交一次通知：首次 `CMD_NOTIFY_LOADED` 调用固件 notification submit，保存原始结果并设置 notified flag；后续调用直接返回成功，不产生重复条目。

class word 而不是 flags 用于选择参考固件的 foreground/module 通知类别。不得仅修改 flags 猜测震动或前台行为。

## 10. 诊断状态

`shellpp_native_get_status()` 将以下值复制到 caller-owned 结构：

- App ID；
- registered；
- published；
- loaded_notified；
- APP_INSTALL 原始结果；
- LAUNCHER_ADD 原始结果；
- notification submit 原始结果。

Supervisor 把它们写入状态 words 14 至 20。标志表示 module 内已经完成对应逻辑，不代表所有 opaque 固件返回值都可按 0/errno 解释。

## 11. Stage 错误

`shellpp_native_install_stage()` 只接受 stage 1 和 2。stage 0 由 Supervisor 作为同步点截获，不会到达该函数。其他值返回 `-103`。

私有错误：

| 错误 | 含义 |
| ---: | --- |
| `-100` | 安装或重新检查后 App 缺失 |
| `-101` | App ID 被不同 package 占用 |
| `-102` | 未注册就请求 Launcher 发布 |
| `-103` | native install stage 无效 |

## 12. 为什么不能 live uninstall

stage 1 后固件会持有：

- App/Page descriptor 地址；
- package、display name、page name、icon 字符串地址；
- display-name 和全部 page lifecycle callback；
- 由页面对象进一步注册的事件 callback。

当前没有已确认的 App registry remove、Page remove、Launcher remove、事件排空和 callback 失效顺序。`shellpp_native_uninstall()` 因此：

- 未注册时返回 0；
- 已注册时返回 `-95`，要求重启。

`shellpp_native_can_unload()` 只有 registered 为 0 才允许 module uninitializer。这个保护不能用“先 unregister driver”替代。

## 13. 常见错误实现

- descriptor 使用局部数组，APP_INSTALL 返回后失效；
- 只检查 App ID，不检查 package；
- APP_INSTALL 后立即一次 lookup，忽略异步 worker；
- 把 Launcher 原始返回值强制解释为 errno；
- stage 1 与 stage 2 在同一固件事件循环片段内连续执行；
- App ID 冲突时覆盖已有对象；
- 每次 Run 或每个 stage 重复发通知；
- 修改 043 page count 的同时改共享 036 源码；
- 只注销 control driver 后卸载 module；
- 猜测未验证的 reverse ABI 实现卸载。

## 14. 新固件审计清单

新增 target 时，除 profile 中 App/Launcher/notification 函数地址外，还必须确认：

1. App lookup 参数和返回对象布局；
2. package pointer 偏移；
3. APP_INSTALL 参数、消费时序和 descriptor 所有权；
4. App/Page descriptor 尺寸与所有写入偏移；
5. Page callback 原型和调用线程；
6. page key 公式；
7. worker publication 可见时序；
8. Launcher add 的参数和返回语义；
9. 0x58 notification record 布局和同步消费保证；
10. App/Page/Launcher 的完整逆向注销流程是否已证实。

任一布局或原型缺失都意味着 ABI 不完整，不能只替换地址后发布。
