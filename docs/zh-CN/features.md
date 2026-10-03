[返回 README](../../README.zh-CN.md) | [English](../en/features.md)

# 功能说明

VGUI2Extension 扩展游戏的 VGUI2 接口，并为其他 MetaHook 插件提供VGUI2相关回调。
构建与启用方式见[构建说明](build-instruction.md)和[安装说明](installation.md)。

## 兼容性

| 引擎 | 构建号范围 |
| --- | --- |
| GoldSrc_blob | 3248–4554 |
| GoldSrc_legacy | 4554–6153 |
| GoldSrc_new | 8684 及以后 |
| SvEngine | 8832 及以后 |
| GoldSrc_HL25 | 9884 及以后 |

引擎范围沿用原插件文档。本独立仓库已有本地构建与模拟回归测试记录；游戏内加载、
各引擎兼容性、HiDPI 和实际 IME 输入仍需游戏运行验证。

* 需要 Windows x86 和 MetaHook API 115 或更新版本。运行时 API 版本还须满足编译插件时使用的 SDK 版本。
* BugFixedHL 使用不同的 VGUI2 对象布局，与本插件不兼容。
* 请随插件安装匹配的 [gamedata](gamedata.md)；仅凭引擎类型和构建号无法保证私有符号可解析。

## VGUI2 mod 框架

插件以回调方式提供修改主菜单、选项对话框和客户端 UI 等 VGUI2 组件的能力。
其他插件可以插入按钮、添加选项页，以及修改现有控件的大小和布局。

公共接口涵盖 VGUI2 回调、DPI 管理、surface 与字体、scheme 和输入处理。
头文件位置见[安装说明](installation.md#公共接口)。

## 游戏语言覆盖

可以使用 Steam 客户端语言或启动参数，覆盖引擎和 VGUI2 使用的语言。
`-forcelang <language>` 优先于 `-steamlang`。Sven Co-op 即使未添加 `-steamlang`，
也会使用 Steam 语言处理路径。

## HiDPI 支持

HiDPI 支持为 VGUI2 控件应用 HDProportional 缩放，并通过文件系统的 `SKIN` 搜索路径加载替代控件资源。

非 HL25 引擎在系统 DPI 缩放比例高于 100% 时默认启用。
`-high_dpi` 和 `-no_high_dpi` 覆盖默认值；同时提供时，`-high_dpi` 优先。
视频模式小于 HDProportional 基准尺寸时会禁用此功能。

对于 HL25，当前实现会在初始化时无条件启用 HiDPI，因此 `-no_high_dpi` 无法禁用它。

启用后，以下资源目录会以 `SKIN` 标签加入搜索路径：

1. `(GameDirectory)\(ModDirectory)_dpi(DpiScalingPercentage)`
   例如：`\Sven Co-op\svencoop_dpi150` 或 `\Half-Life\valve_dpi200`。
2. `(GameDirectory)\(ModDirectory)_hidpi`
   例如：`\Sven Co-op\svencoop_hidpi` 或 `\Half-Life\valve_hidpi`。

## 启动参数

| 参数 | 作用 |
| --- | --- |
| `-steamlang` | 使用 Steam 客户端语言作为引擎和 VGUI2 的语言；Sven Co-op 无需此参数也会使用该路径 |
| `-forcelang <language>` | 强制引擎和 VGUI2 使用指定语言，覆盖 Steam 中的游戏语言设置 |
| `-high_dpi` | 启用 HiDPI，受上述视频模式限制 |
| `-no_high_dpi` | 在非 HL25 引擎中禁用 HiDPI |
| `clientui_use_hdp` | 为 ClientUI 的 VGUI2 控件启用 HDProportional |
| `clientui_no_hdp` | 为 ClientUI 的 VGUI2 控件禁用 HDProportional |

当前实现检查的两个 ClientUI 参数**没有前导 `-`**。
效果取决于客户端的 VGUI2 控件；同时提供时，`clientui_use_hdp` 优先。
