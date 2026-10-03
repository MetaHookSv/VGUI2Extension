# VGUI2Extension

[English README](README.md)

VGUI2Extension 是 MetaHook 的 UI 扩展插件，为其他插件提供修改 GoldSrc VGUI2 组件的能力，
并提供字体与 HiDPI 支持、游戏语言覆盖和输入法处理。

* 需要 Windows x86 和 MetaHook API 115 或更新版本。宿主还须满足编译插件时使用的 API 版本。
* BugFixedHL 使用不同的 VGUI2 对象布局，与本插件不兼容。

## 兼容性

| 引擎 | 构建号范围 |
| --- | --- |
| GoldSrc_blob | 3248–4554 |
| GoldSrc_legacy | 4554–6153 |
| GoldSrc_new | 8684 及以后 |
| SvEngine | 8832 及以后 |
| GoldSrc_HL25 | 9884 及以后 |

这些范围沿用原插件文档。符号要求见 [gamedata](docs/zh-CN/gamedata.md)，
游戏运行验证状态见[功能说明](docs/zh-CN/features.md)。

## 快速开始

从 [GitHub Releases](https://github.com/MetaHookSv/VGUI2Extension/releases) 获取
`VGUI2Extension-windows-x86.7z`，或在本地[构建插件](docs/zh-CN/build-instruction.md)。

将解压出的 `svencoop/` 合并到目标 mod 目录，将 `platform/` 合并到游戏的 platform 目录。
在 MetaHook 的 `metahook/configs/plugins.lst` 中启用 `VGUI2Extension.dll`，放在依赖它的插件之前，
然后通过 MetaHook 启动游戏。目录布局见[安装说明](docs/zh-CN/installation.md)。

## 文档

- [构建说明：构建、依赖、gamedata 与回归测试](docs/zh-CN/build-instruction.md)
- [安装说明：安装目录布局、加载顺序与公共接口](docs/zh-CN/installation.md)
- [功能说明：兼容性、UI 扩展、HiDPI 与启动参数](docs/zh-CN/features.md)
- [gamedata：catalog 要求与插件使用的 GameSymbols](docs/zh-CN/gamedata.md)
- [自动化构建：CI 工作流与发布归档](docs/zh-CN/ci.md)

## 许可证

项目采用 [MIT License](LICENSE)；各依赖保留自己的许可证。
