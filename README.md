# Shell++ II

Xiaomi Band 10 Pro 的共享 nativeApp 源码。固件私有 ABI 由相邻构建器的 target profile 生成并注入。

完整技术文档统一位于 [`docs/README.md`](docs/README.md)，包括架构、固件 ABI、构建、安装器协议、固件适配、验证、故障排查和 AI 交接规范。

同机型新系统的适配必须在取得该固件完整 ABI 后开始。ABI 不完整时只能继续分析并记录缺口，不能创建可发布 target。

构建入口：

```sh
cd /Users/ikun_cxkpro/Projects/Shell++/Shellpp-ii-build
./build.sh
```
