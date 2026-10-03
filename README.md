# VGUI2Extension

[中文文档](README.zh-CN.md)

VGUI2Extension is a UI extension plugin for MetaHook. It lets other plugins customize
GoldSrc VGUI2 components and provides font and HiDPI support, game language overrides
and input method handling.

* Windows x86 and MetaHook API 115 or newer are required. The host must also meet the
  API version used to build the plugin.
* BugFixedHL uses a different VGUI2 object layout and is not compatible.

## Compatibility

| Engine | Build range |
| --- | --- |
| GoldSrc_blob | 3248–4554 |
| GoldSrc_legacy | 4554–6153 |
| GoldSrc_new | 8684 and later |
| SvEngine | 8832 and later |
| GoldSrc_HL25 | 9884 and later |

These ranges follow the original plugin documentation. See [gamedata](docs/en/gamedata.md)
for symbol requirements and [Features](docs/en/features.md) for runtime verification status.

## Quick start

Obtain `VGUI2Extension-windows-x86.7z` from
[GitHub Releases](https://github.com/MetaHookSv/VGUI2Extension/releases), or
[build the plugin locally](docs/en/build-instruction.md).

Merge the extracted `svencoop/` into the target mod directory and `platform/` into the
game's platform directory.

Enable `VGUI2Extension.dll` in MetaHook's `metahook/configs/plugins.lst`, then launch the game through MetaHook.

## Documentation

- [Build instruction](docs/en/build-instruction.md)
- [Installation](docs/en/installation.md)
- [Features](docs/en/features.md)
- [gamedata](docs/en/gamedata.md)
- [CI/CD](docs/en/ci-cd.md)

## License

Licensed under the [MIT License](LICENSE); each dependency keeps its own license.
