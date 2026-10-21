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
