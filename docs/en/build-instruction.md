[Back to README](../../README.md) | [中文](../zh-CN/build-instruction.md)

# Build instruction

This page covers the build, dependencies, gamedata and regression tests of VGUI2Extension.
For the install layout and enabling the plugin, see [Installation](installation.md);
for compatibility and launch parameters, see [Features](features.md).
CI workflows and release archives are described in [Automated builds](ci-cd.md).

## Requirements

- Windows with Visual Studio 2022 C++ desktop workload, an x86 MSVC toolchain and Windows SDK
- CMake 3.21+, Git and Python 3, available on `PATH`
- SDL2 and SDL3 headers, from a MetaHook source tree or its install tree
- Network access on first configure to fetch the MetaHook SDK when no local path is
  supplied, prepare VC-LTL 5.3.1 and synchronize gamedata

## Build

After building MetaHook, pass the matching install tree's `include` directory:

```bat
scripts\build-VGUI2Extension-x86-Debug.bat "-DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Debug/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Debug/include"
scripts\build-VGUI2Extension-x86-Release.bat "-DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include"
```

Both entry points use `Visual Studio 17 2022 -A Win32` and perform configure, build and
install. They can be invoked from outside the project; when `SolutionDir` is unset,
the project root is located from the script path. A failed step returns a non-zero exit
code. Build directories are `build/x86/<Debug|Release>` and install directories are
`install/x86/<Debug|Release>`. Deployment to a game directory is manual.

## Specifying source paths manually

The MetaHook SDK is downloaded automatically from the latest `main`. To reuse a local
checkout, set `METAHOOK_SOURCE_PATH`. Both SDL header paths are always required:

| Parameter | Directory |
| --- | --- |
| `METAHOOK_SOURCE_PATH` (optional) | MetaHook repository root with `include/metahook.h` and the HLSDK, SourceSDK and VGUI sources |
| `SDL2_INCLUDE_DIRS` (required) | Include directory containing `SDL2/SDL_syswm.h` |
| `SDL3_INCLUDE_DIRS` (required) | Include directory containing `SDL3/SDL_events.h` |

SDL headers can come from MetaHook's install tree, as shown above, or its initialized
dependency source trees:

```bat
scripts\build-VGUI2Extension-x86-Release.bat ^
  "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" ^
  "-DSDL2_INCLUDE_DIRS=D:/MetaHook/thirdparty/sdl2-compat-fork/include" ^
  "-DSDL3_INCLUDE_DIRS=D:/MetaHook/thirdparty/SDL3_fork/include"
```

All three parameters accept same-named environment variables on first configure.
Use `-DNAME=value` to change a cached value; `-DMETAHOOK_SOURCE_PATH=` restores automatic
SDK downloading. SDL paths accept semicolon-separated directory lists; quote the entire
`-D` argument. MetaHook provides the SDL runtime.

## Build options

| Option | Default | Purpose |
| --- | --- | --- |
| `VGUI2EXTENSION_BUILD_TESTS` | `OFF` | Build language registry and IME message regression tests |
| `VGUI2EXTENSION_SYNC_GAMEDATA` | `ON` | Synchronize, prune and validate the plugin's gamedata |
| `VGUI2EXTENSION_GAMEDATA_DIR` | `<build>/assets/svencoop/metahook/gamedata/vgui2extension` | Catalog output directory |
| `VGUI2EXTENSION_DEPENDENCY_CACHE_DIR` | `thirdparty/cache` | VC-LTL download and extraction cache |

## gamedata

[scripts/manifests/vgui2extension.json](../../scripts/manifests/vgui2extension.json)
declares the plugin's private symbols, module ownership and Windows game-version
conditions, covering 21 game snapshots.

With `VGUI2EXTENSION_SYNC_GAMEDATA=ON`, the build runs `scripts/sync-gamedata.py` to prune
the upstream catalog to those records, then runs `scripts/validate-gamedata.py` against
the manifest. The result is installed under `metahook/gamedata/vgui2extension/` and merged
by the host with other catalogs. Raw snapshots are cached under
`build/x86/<configuration>/gamedata-sync/` for offline reuse.

With `OFF`, only an existing catalog is installed. A fresh build directory in this mode
does not provide gamedata.

For catalog requirements, the complete GameSymbols list, module ownership and version
conditions, see [gamedata](gamedata.md). Update the manifest when symbol requirements change.

## Dependencies and build conventions

- CMake downloads the MetaHook SDK and VC-LTL 5.3.1 automatically. The SDK version is
  recorded in [cmake/Dependencies.cmake](../../cmake/Dependencies.cmake).
- VC-LTL is verified after download. Debug and Release share its cache under
  `thirdparty/cache/`; use `-DVGUI2EXTENSION_DEPENDENCY_CACHE_DIR=<path>` on first configure
  to choose another directory.
- For offline builds, prepare the SDK, VC-LTL and gamedata caches with an online build first.
  A local SDK path alone does not cover every download.
- External SDK sources are left unchanged. Both Debug and Release include a PDB for debugging.

## Regression tests

Enable the two existing C++ test groups with `VGUI2EXTENSION_BUILD_TESTS=ON`:

```bat
scripts\build-VGUI2Extension-x86-Release.bat -DVGUI2EXTENSION_BUILD_TESTS=ON "-DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include"
ctest --test-dir build/x86/Release -C Release --output-on-failure
ctest --test-dir build/x86/Debug -C Debug --output-on-failure
python -m unittest discover -s scripts/tests -v
python scripts/validate-gamedata.py install/x86/Release/svencoop/metahook/gamedata/vgui2extension --manifest scripts/manifests/vgui2extension.json
```

Build Debug with the same test option before running its CTest command. C++ tests cover
language registry and IME messages, with assertions retained in Release. Python unittest
covers conditional pruning of numbered gamedata patches.

Local build and test records are in
[memory/build_and_verification.md](../../memory/build_and_verification.md).
Compilation and simulated tests do not verify in-game loading, HiDPI or actual IME input.
