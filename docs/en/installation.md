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

5. Launch the game through MetaHook.
