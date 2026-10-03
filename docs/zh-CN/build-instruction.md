[返回 README](../../README.zh-CN.md) | [English](../en/build-instruction.md)

# 构建说明

本页涵盖 VGUI2Extension 的构建、依赖、gamedata 与回归测试。
安装目录布局与启用插件见[安装说明](installation.md)；兼容性与启动参数见[功能说明](features.md)。
CI 工作流与发布归档见[自动化构建](ci.md)。

## 依赖要求

- Windows、Visual Studio 2022 的 C++ 桌面开发工具、x86 MSVC 工具链和 Windows SDK
- CMake 3.21+、Git 和 Python 3，且均位于 `PATH`
- 来自 MetaHook 源码树或安装树的 SDL2、SDL3 头文件
- 首次配置需要网络，用于在未指定本地路径时获取 MetaHook SDK、准备 VC-LTL 5.3.1 和同步 gamedata

## 构建

构建 MetaHook 后，传入对应配置安装树的 `include` 目录：

```bat
scripts\build-VGUI2Extension-x86-Debug.bat "-DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Debug/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Debug/include"
scripts\build-VGUI2Extension-x86-Release.bat "-DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include"
```

两个入口均使用 `Visual Studio 17 2022 -A Win32`，执行 configure、build 和 install。
可从工程外调用；未设置 `SolutionDir` 时由脚本位置定位工程根目录。任一步失败返回非零退出码。
构建目录为 `build/x86/<Debug|Release>`，安装目录为 `install/x86/<Debug|Release>`。
部署到游戏目录需手动进行。

## 手动指定源码路径

`METAHOOK_SOURCE_PATH` 指向包含 `include/metahook.h`、`include/HLSDK/`、
`include/SourceSDK/` 和 `include/vgui_controls/` 的 **MetaHook 仓库根目录**。
显式指定有效路径后，跳过 SDK 下载。

SDL 头文件也可以直接取自 MetaHook 已初始化的依赖源码树：

```bat
scripts\build-VGUI2Extension-x86-Release.bat "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" "-DSDL2_INCLUDE_DIRS=D:/MetaHook/thirdparty/sdl2-compat-fork/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/thirdparty/SDL3_fork/include"
```

`METAHOOK_SOURCE_PATH`、`SDL2_INCLUDE_DIRS` 和 `SDL3_INCLUDE_DIRS` 支持首次配置的同名环境变量。
显式 `-D` 参数优先；环境变量只用于初始化 CMake cache。`-DMETAHOOK_SOURCE_PATH=` 恢复
FetchContent 获取。离线构建时，提供本地 SDK，或复用已准备好的构建目录和依赖缓存。

两个 SDL 参数均必填，支持分号分隔的目录列表。SDL2 必须提供 `SDL2/SDL_syswm.h`，
SDL3 必须提供 `SDL3/SDL_events.h`。CMake 规范化 include 路径，并在配置阶段检查目录与必要头文件。
本工程消费这些头文件，SDL 运行时由 MetaHook 提供。

直接调用 CMake：

```bat
cmake -S . -B build/x86/Release -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX=install/x86/Release -DMETAHOOK_SOURCE_PATH=D:/MetaHook -DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include -DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include
cmake --build build/x86/Release --config Release --target install --parallel
```

## 构建选项

| 选项 | 默认值 | 用途 |
| --- | --- | --- |
| `VGUI2EXTENSION_BUILD_TESTS` | `OFF` | 构建语言注册表和 IME 消息回归测试 |
| `VGUI2EXTENSION_SYNC_GAMEDATA` | `ON` | 同步、裁剪并校验插件 gamedata |
| `VGUI2EXTENSION_GAMEDATA_DIR` | `<build>/assets/svencoop/metahook/gamedata/vgui2extension` | catalog 输出目录 |
| `VGUI2EXTENSION_DEPENDENCY_CACHE_DIR` | `thirdparty/cache` | VC-LTL 下载与解压缓存 |

## gamedata

[scripts/manifests/vgui2extension.json](../../scripts/manifests/vgui2extension.json)
声明插件的私有符号、module 归属及 Windows 游戏版本条件，覆盖 21 个游戏快照。

`VGUI2EXTENSION_SYNC_GAMEDATA=ON` 时，构建调用 `scripts/sync-gamedata.py` 将上游 catalog
裁剪为这些记录，再调用 `scripts/validate-gamedata.py` 按 manifest 校验。
结果安装到 `metahook/gamedata/vgui2extension/`，由宿主与其他 catalog 合并。
原始快照缓存于 `build/x86/<configuration>/gamedata-sync/`，供离线复用。

设为 `OFF` 时只安装已有 catalog。空构建目录在此模式下不会获得 gamedata。

catalog 要求、完整 GameSymbols 清单、module 归属与版本条件见 [gamedata](gamedata.md)。
符号需求变化时需同步更新 manifest。

## 依赖与构建约定

| 依赖 | 来源 | 版本 | 用途 |
| --- | --- | --- | --- |
| MetaHookSv/MetaHook | 本地源码路径或 FetchContent | 获取时固定为 `1d23fe946e6f0f09a1a892aa2156c3b462774026` | 公共 API、HLSDK/SourceSDK 与 VGUI 源码 |
| SDL2 / SDL3 | 外部 include 目录 | 由 MetaHook 提供 | 输入和窗口声明 |
| VC-LTL | 校验过的二进制缓存 | 5.3.1 | CRT 兼容性 |

MetaHook 作为 SDK 使用：FetchContent 路径不构建宿主，也不初始化它的 submodule。
本仓库没有第三方 submodule，不构建、链接或安装 SDL、Capstone、GLEW。

VC-LTL 来自 `Chuyu-Team/VC-LTL5` v5.3.1 的 `VC-LTL-Binary.7z`，通过以下 SHA-256 校验：
`7a18799ed3aa84a225610a5447a56bc534c5c98ccb8dec05caba0e3f633431ad`。
Debug 与 Release 共用 `thirdparty/cache/` 下的缓存；可通过
`VGUI2EXTENSION_DEPENDENCY_CACHE_DIR` 指定其他目录。

VGUI2Extension 使用 C++20、Debug `/MTd`、Release `/MT`。Release 启用 LTCG、`/OPT:REF`
和 `/OPT:ICF`；两种配置均生成 PDB。显式编译清单位于
[cmake/Sources.cmake](../../cmake/Sources.cmake)，共 129 个编译单元。
`src/parsemsg.cpp` 和 `src/steam_api.cpp` 保留在源码树中，但不参与编译。

## 回归测试

通过 `VGUI2EXTENSION_BUILD_TESTS=ON` 启用两组既有 C++ 测试：

```bat
scripts\build-VGUI2Extension-x86-Release.bat -DVGUI2EXTENSION_BUILD_TESTS=ON "-DSDL2_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/install/x86/Release/include"
ctest --test-dir build/x86/Release -C Release --output-on-failure
ctest --test-dir build/x86/Debug -C Debug --output-on-failure
python -m unittest discover -s scripts/tests -v
python scripts/validate-gamedata.py install/x86/Release/svencoop/metahook/gamedata/vgui2extension --manifest scripts/manifests/vgui2extension.json
```

运行 Debug 的 CTest 命令前，也需使用同一测试选项构建 Debug。
C++ 测试覆盖语言注册表和 IME 消息，Release 保留断言。
Python unittest 覆盖 gamedata 连续编号补丁的条件裁剪。

本地构建和测试记录见 [memory/build_and_verification.md](../../memory/build_and_verification.md)。
编译与模拟测试不等同于游戏内插件加载、HiDPI 和实际 IME 输入验证。

## 许可证

许可证见 [LICENSE](../../LICENSE)；各依赖保留自己的许可证。
