# VGUI2Extension

[English README](README.md)

VGUI2Extension 是 MetaHook 的 UI 扩展插件，为其他插件提供修改 GoldSrc VGUI2 组件的能力，
并提供字体与 HiDPI 支持、多语言支持和输入法处理。

* BugFixedHL 使用不同的 VGUI2 对象布局，因此与本插件不兼容。

## 兼容性

|        Engine               |      |
|        ----                 | ---- |
| GoldSrc_blob   (3248~4554)  | √    |
| GoldSrc_legacy (4554~6153)  | √    |
| GoldSrc_new    (8684 ~)     | √    |
| SvEngine       (8832 ~)     | √    |
| GoldSrc_HL25   (>= 9884)    | √    |
| GoldSrc_CoF    (5936)       | √    |

## 快速开始

从 [GitHub Releases](https://github.com/MetaHookSv/VGUI2Extension/releases) 获取
`VGUI2Extension-windows-x86.7z`，或在[本地构建插件](docs/zh-CN/build-instruction.md)。

将解压出的 `svencoop/` 合并到目标 mod 目录，将 `platform/` 合并到游戏的 platform 目录。
在 MetaHook 的 `metahook/configs/plugins.lst` 中启用 `VGUI2Extension.dll`，然后通过 MetaHook 启动游戏。

## 文档

- [构建说明](docs/zh-CN/build-instruction.md)
- [安装说明](docs/zh-CN/installation.md)
- [功能说明](docs/zh-CN/features.md)
- [gamedata](docs/zh-CN/gamedata.md)
- [自动化构建与发布](docs/zh-CN/ci-cd.md)
- [F5 调试（可选）](docs/zh-CN/debugging.md)

## 许可证

项目采用 [MIT License](LICENSE)；各依赖保留自己的许可证。

## C/C++ 格式化

使用 [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
共享工具及固定版本 **clang-format 23.1.3**，采用 DiligentCore 风格（4 空格，保留
include 顺序）。为 CMake 使用的 Python 解释器安装格式工具：

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

格式专用配置需要 CMake 3.21+、Git、Python 3.9+（CI 使用 3.12）及构建生成器；
使用 `-G Ninja` 可无需 Visual Studio。它不准备原生 SDK 或游戏依赖。
格式目标需显式执行，不加入普通 DLL 构建。使用 Visual Studio 生成器时，执行目标
需追加 `--config Debug` 或 `--config Release`。

聚合仓库注入 `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`。
独立组件支持该 CMake 参数及同名环境变量；为空时通过 FetchContent 获取固定工具
提交。相对路径应加引号，例如
`"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`。
配置时在仓库根目录生成被 gitignore 的 `.clang-format` 供编辑器使用；格式规则应
在共享仓库修改，不修改生成副本。可通过 `FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE`
指定工具路径，但版本仍须与固定版本一致。

检查覆盖 `src/`、`include/`、`tests/` 中维护的 C/C++ 文件，包括未被 Git 忽略的新文件。
相对仓库根目录的排除规则位于 `.clang-format-ignore`；第三方源和构建产物不纳入检查。
`clang-format` workflow 在 push、pull request 和手动运行时执行全量检查。
