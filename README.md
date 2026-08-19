# Shell++ II

Shell++ II 的原生模块源码，当前目标为 Xiaomi Band 10 Pro 固件 `3.101.036`。

## 快速开始

在同一个父目录中拉取三个仓库。构建脚本依赖它们的相对位置：

```sh
git clone https://github.com/Shellplusplus/Shellpp-II-App.git shellpp-ii
git clone https://github.com/Shellplusplus/Shellpp-II-Build.git shellpp-ii-build
git clone https://github.com/Shellplusplus/Shellpp-II-install-Lua.git shellpp-ii-installer
```

构建环境需要 macOS、Apple Clang、Rust 工具链（提供 `rust-lld`）、Python 3、Node.js 和 `sips`。然后执行：

```sh
cd shellpp-ii-build
./build.sh
```

构建产物为 `out/xiaomi-band-10-pro-3.101.036/shellpp_ii.bin`。脚本会同时更新相邻 `shellpp-ii-installer` 仓库中的原生模块、图标资源、`resource.bin` 和 `hashCode`。
