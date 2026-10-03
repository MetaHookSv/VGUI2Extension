[Back to README](../../README.md) | [中文](../zh-CN/build-instruction.md)

# Build instruction

This page covers the build, dependencies, gamedata and regression tests of VGUI2Extension.
For the install layout and enabling the plugin, see [Installation](installation.md);
for compatibility and launch parameters, see [Features](features.md).
CI workflows and release archives are described in [Automated builds](ci.md).

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

`METAHOOK_SOURCE_PATH` points to the **MetaHook repository root** containing
`include/metahook.h`, `include/HLSDK/`, `include/SourceSDK/` and `include/vgui_controls/`.
With a valid explicit path, the SDK download is skipped.

SDL headers can also come directly from MetaHook's initialized dependency source trees:

```bat
scripts\build-VGUI2Extension-x86-Release.bat "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" "-DSDL2_INCLUDE_DIRS=D:/MetaHook/thirdparty/sdl2-compat-fork/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/thirdparty/SDL3_fork/include"
```

`METAHOOK_SOURCE_PATH`, `SDL2_INCLUDE_DIRS` and `SDL3_INCLUDE_DIRS` accept same-named
environment variables on first configure. Explicit `-D` arguments take precedence;
environment variables only seed the CMake cache. `-DMETAHOOK_SOURCE_PATH=` restores
the FetchContent fetch. For offline builds, supply a local SDK or reuse a populated build
directory and dependency caches.

Both SDL parameters are required and accept semicolon-separated directory lists. SDL2
must provide `SDL2/SDL_syswm.h`, and SDL3 must provide `SDL3/SDL_events.h`. CMake normalizes
the include paths and rejects missing directories or required headers at configure time.
This project consumes the headers; MetaHook provides the SDL runtime.

Calling CMake directly:

```bat
cmake -S . -B build/x86/Release -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX=install/x86/Release -DMETAHOOK_SOURCE_PATH=D:/MetaHook -DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include -DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include
cmake --build build/x86/Release --config Release --target install --parallel
```

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

| Dependency | Source | Version | Purpose |
| --- | --- | --- | --- |
| MetaHookSv/MetaHook | Local source path or FetchContent | `1d23fe946e6f0f09a1a892aa2156c3b462774026` when fetched | Public API, HLSDK/SourceSDK and VGUI sources |
| SDL2 / SDL3 | External include directories | Provided by MetaHook | Input and window declarations |
| VC-LTL | Verified binary cache | 5.3.1 | CRT compatibility |

MetaHook is consumed as an SDK: the FetchContent path does not build the host or initialize
its submodules. This repository has no third-party submodules and does not build, link or
install SDL, Capstone or GLEW.

VC-LTL comes from `Chuyu-Team/VC-LTL5` v5.3.1 `VC-LTL-Binary.7z`, verified with SHA-256:
`7a18799ed3aa84a225610a5447a56bc534c5c98ccb8dec05caba0e3f633431ad`.
Debug and Release share the cache under `thirdparty/cache/`; use
`VGUI2EXTENSION_DEPENDENCY_CACHE_DIR` to choose another directory.

VGUI2Extension uses C++20, Debug `/MTd` and Release `/MT`. Release enables LTCG,
`/OPT:REF` and `/OPT:ICF`; both configurations emit a PDB. The explicit compilation list
is in [cmake/Sources.cmake](../../cmake/Sources.cmake), with 129 compilation units.
`src/parsemsg.cpp` and `src/steam_api.cpp` are retained but are not compiled.

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

## License

See [LICENSE](../../LICENSE); each dependency keeps its own license.
