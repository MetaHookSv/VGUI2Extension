[Back to README](../../README.md) | [中文](../zh-CN/installation.md)

# Installation

This page covers the install layout, plugin loading order and public interface headers.
For building from source, see [Build instruction](build-instruction.md); for runtime
requirements and launch parameters, see [Features](features.md).

## Install layout

`install/x86/<Debug|Release>/`:

```text
svencoop/
  metahook/plugins/VGUI2Extension.dll
  metahook/plugins/VGUI2Extension.pdb
  metahook/gamedata/vgui2extension/
  vgui2ext/resource/
  vgui2ext/resource_hl25/
platform/
  resource/vgui_schinese.txt
  servers/serverbrowser_schinese.txt
include/Interface/
  IVGUI2Extension.h
  IDpiManager.h
  VGUI/IInput2.h
  VGUI/IScheme2.h
  VGUI/ISurface2.h
```

The runtime archive `VGUI2Extension-windows-x86.7z` contains `svencoop/` and `platform/`.
Public interface headers are installed for development and are not included in that archive.

## Enable the plugin

1. Install MetaHook with API 115 or newer and support for merging nested gamedata catalogs.
   Its API version must also be at least the SDK version used to build the plugin.
2. Merge the contents of `svencoop/` into the target mod directory, such as `valve/`,
   `cstrike/` or `svencoop/`.
3. Merge `platform/` into the game's platform directory.
4. Add the following entry to `<mod>/metahook/configs/plugins.lst`:

   ```text
   VGUI2Extension.dll
   ```

5. Place it before plugins that depend on it, such as CaptionMod, BulletPhysics and
   SCModelDownloader, and launch the game through MetaHook.

Update the DLL, resources and `metahook/gamedata/vgui2extension/` together. Older catalogs
may lack the private symbols required by the plugin; see
[gamedata requirements](gamedata.md#runtime-requirements).

## Public interfaces

The authoritative headers are in this repository's `include/Interface/` and
`include/Interface/VGUI/`. Consumers should place these include directories before
MetaHook's historical copies; other SDK headers still come from MetaHook.

| Interface | Header |
| --- | --- |
| VGUI2 callbacks | [IVGUI2Extension.h](../../include/Interface/IVGUI2Extension.h) |
| DPI management | [IDpiManager.h](../../include/Interface/IDpiManager.h) |
| Extended input | [IInput2.h](../../include/Interface/VGUI/IInput2.h) |
| Extended schemes | [IScheme2.h](../../include/Interface/VGUI/IScheme2.h) |
| Extended surface and fonts | [ISurface2.h](../../include/Interface/VGUI/ISurface2.h) |

The standalone project retains the original interface versions and plugin export conventions.
