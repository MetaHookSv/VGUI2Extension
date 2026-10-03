# AGENTS.md

- 本仓库是从 MetaHookSv 迁出的独立 VGUI2Extension 插件。先读 `memory/project_overview.md`，构建与验证读 `memory/build_and_verification.md`。
- Basic Memory 只有在项目绑定到本仓库 memory 目录时才用于写入；metahooksv 项目属于源仓库。无对应 MCP 项目时直接读写本地 Markdown。
- 插件源码在 `src/`，公共接口在 `include/Interface/`，资源在 `assets/`。源码命名、缩进和注释遵循原文件。
- 保持 MetaHook API、插件导出、接口版本和原行为。私有符号通过宿主 gamedata API 解析，不为已有符号加入扫描 fallback。
- CMake 显式编译清单位于 `cmake/Sources.cmake`。SDK 从 `METAHOOK_SOURCE_PATH` 读取；未指定时获取固定提交，只消费 SDK。
- SDL2 与 SDL3 include 参数均必填，使用依赖函数规范化后的路径，不构建 SDL。不要修改外部 SDK 或第三方源码。
- VC-LTL 使用校验哈希的二进制缓存；本工程没有第三方 submodule。输出只到 `build/` 与 `install/`，不自动部署游戏。
- 本仓库五个公共接口头优先于 MetaHook 历史副本，并随 install 安装。
- 修改 gamedata 使用时同步 `scripts/manifests/vgui2extension.json`；包括 module、Windows 版本条件及连续编号补丁。不得只按符号名称跨模块判断覆盖。
- 原有 C++ 回归测试使用 CTest，gamedata 同步器行为测试使用 Python unittest。Release 测试保留断言；配置与文档文本不作为单元测试断言对象。
- 验证区分编译、模拟测试与游戏运行。未经实际游戏验证不得声称输入法或各引擎兼容性已验证。
- 项目级 skills 如有，位于 `.claude/skills`。
