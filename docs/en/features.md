[Back to README](../../README.md) | [中文](../zh-CN/features.md)

# Features

VGUI2Extension extends the game's VGUI2 interfaces and provides callbacks for other
MetaHook plugins. For building and enabling it, see [Build instruction](build-instruction.md)
and [Installation](installation.md).

## Compatibility

| Engine | Build range |
| --- | --- |
| GoldSrc_blob | 3248–4554 |
| GoldSrc_legacy | 4554–6153 |
| GoldSrc_new | 8684 and later |
| SvEngine | 8832 and later |
| GoldSrc_HL25 | 9884 and later |

The engine ranges follow the original plugin documentation. This standalone repository
has local build and simulated regression test records; in-game loading, engine
compatibility, HiDPI and actual IME input still require game testing.

* Windows x86 and MetaHook API 115 or newer are required. The runtime API version must
  also meet the SDK version used to build the plugin.
* BugFixedHL uses a different VGUI2 object layout and is not compatible.
* Install matching [gamedata](gamedata.md) with the plugin. Engine family and build number
  alone do not guarantee that private symbols can be resolved.

## VGUI2 modding framework

The plugin provides callbacks for customizing VGUI2 components, including the main menu,
options dialog and client UI. Other plugins can insert buttons, add options pages and
change the size and layout of existing controls.

The public interfaces cover VGUI2 callbacks, DPI management, surface and fonts, schemes,
and input handling. Header locations are listed in [Installation](installation.md#install-layout).

## Game language overrides

Use Steam's language or a launch parameter to override the language used by the engine
and VGUI2. `-forcelang <language>` takes precedence over `-steamlang`. Sven Co-op uses
the Steam language path even without `-steamlang`.

## HiDPI support

HiDPI support applies HDProportional scaling to VGUI2 controls and loads alternative
control resources through the filesystem's `SKIN` search paths.

On non-HL25 engines, it is enabled by default when system DPI scaling is above 100%.
`-high_dpi` and `-no_high_dpi` override that default; when both are supplied, `-high_dpi`
takes precedence. Video modes smaller than the HDProportional base size disable it.

On HL25, the current implementation enables HiDPI unconditionally during initialization,
so `-no_high_dpi` does not disable it.

When enabled, the following resource directories are added with the `SKIN` tag:

1. `(GameDirectory)\(ModDirectory)_dpi(DpiScalingPercentage)`
   For example: `\Sven Co-op\svencoop_dpi150` or `\Half-Life\valve_dpi200`.
2. `(GameDirectory)\(ModDirectory)_hidpi`
   For example: `\Sven Co-op\svencoop_hidpi` or `\Half-Life\valve_hidpi`.

## Launch parameters

| Parameter | Effect |
| --- | --- |
| `-steamlang` | Use Steam's language for the engine and VGUI2; Sven Co-op uses this path without the parameter |
| `-forcelang <language>` | Force the engine and VGUI2 language, overriding the game's Steam language setting |
| `-high_dpi` | Enable HiDPI, subject to the video mode restriction above |
| `-no_high_dpi` | Disable HiDPI on non-HL25 engines |
| `clientui_use_hdp` | Enable HDProportional for ClientUI VGUI2 controls |
| `clientui_no_hdp` | Disable HDProportional for ClientUI VGUI2 controls |

The current implementation checks the two ClientUI parameters **without a leading `-`**.
Their effect depends on the client's VGUI2 controls; when both are supplied,
`clientui_use_hdp` takes precedence.
