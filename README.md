# VGUI2Extension

MetaHook VGUI2Extension 插件的独立 Windows x86 CMake 工程，迁自
`hzqst/MetaHookSv` 提交 `fe80b6d60bfb487b52aed7ea7ec0492e7b27a5d2`。
提供 VGUI2 回调扩展、字体与 HiDPI、语言设置及输入法处理。
插件源码和公共接口保持原样，原仓库副本保留。

## 构建

需要 Visual Studio 2022 C++ 桌面开发工具、Windows SDK、CMake 3.21+、Git 和 Python 3。
与独立 MetaHook、Renderer 工程一样，提供 x86 Debug 和 Release：

```bat
scripts\build-VGUI2Extension-x86-Debug.bat "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" "-DSDL2_INCLUDE_DIRS=D:/MetaHook/thirdparty/sdl2-compat-fork/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/thirdparty/SDL3_fork/include" -DVGUI2EXTENSION_BUILD_TESTS=ON
scripts\build-VGUI2Extension-x86-Release.bat "-DMETAHOOK_SOURCE_PATH=D:/MetaHook" "-DSDL2_INCLUDE_DIRS=D:/MetaHook/thirdparty/sdl2-compat-fork/include" "-DSDL3_INCLUDE_DIRS=D:/MetaHook/thirdparty/SDL3_fork/include" -DVGUI2EXTENSION_BUILD_TESTS=ON
```

脚本执行 configure、build、install，任一步失败返回非零退出码。支持从工程外调用；
默认通过脚本位置确定工程根，也可沿用 `SolutionDir`。构建目录为 `build/x86/<配置>`，
安装目录为 `install/x86/<配置>`，不会自动部署到游戏目录。

| 参数 | 含义 |
| --- | --- |
| `METAHOOK_SOURCE_PATH` | MetaHook 仓库根目录，提供公共 API、HLSDK、SourceSDK 和 VGUI 源码 |
| `SDL2_INCLUDE_DIRS` | 必填，目录中需有 `SDL2/SDL_syswm.h` |
| `SDL3_INCLUDE_DIRS` | 必填，目录中需有 `SDL3/SDL_events.h` |
| `VGUI2EXTENSION_BUILD_TESTS` | 默认 OFF；ON 时构建语言注册表和 IME 消息回归测试 |
| `VGUI2EXTENSION_SYNC_GAMEDATA` | 默认 ON；同步、裁剪并校验插件自己的 gamedata |
| `VGUI2EXTENSION_GAMEDATA_DIR` | catalog 输出目录，默认位于当前 build 目录的 assets 下 |
| `VGUI2EXTENSION_DEPENDENCY_CACHE_DIR` | VC-LTL 缓存目录，默认 `thirdparty/cache` |

前三个参数支持同名环境变量作为首次配置初值，显式 `-D` 参数优先。SDL 参数支持分号分隔
的目录列表；也可使用 MetaHook 安装树的 `include` 目录。SDL 由宿主构建和安装，本工程只消费
头文件；不构建、链接或安装 SDL、Capstone、GLEW。

`METAHOOK_SOURCE_PATH` 为空时，FetchContent 获取
`https://github.com/MetaHookSv/MetaHook` 的固定提交
`1d23fe946e6f0f09a1a892aa2156c3b462774026`，只消费 SDK，不构建宿主、不初始化它的 submodule。
显式提供路径时跳过获取。源码依赖不复制进本仓库。

VC-LTL 5.3.1 由 CMake 下载并核验 SHA-256：
`7a18799ed3aa84a225610a5447a56bc534c5c98ccb8dec05caba0e3f633431ad`。
首次配置和 gamedata 同步需要网络；同步器保留缓存以支持离线使用。
`VGUI2EXTENSION_SYNC_GAMEDATA=OFF` 只安装已有 catalog；空构建目录在此模式下不会获得 gamedata。

## 安装和使用

安装树包含：

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

将 `svencoop/` 的内容合并到目标 mod 目录，将 `platform/` 合并到游戏的 platform 目录。
宿主需提供 MetaHook API 115 或更新版本，以及递归合并 gamedata 子目录的功能。
在 `<mod>/metahook/configs/plugins.lst` 中启用 `VGUI2Extension.dll`，放在依赖它的
CaptionMod、BulletPhysics、SCModelDownloader 等插件之前。

公共接口以本仓库 `include/Interface` 及 `include/Interface/VGUI` 为准，编译时优先于
MetaHook 内的历史副本；其余 SDK 头仍由 MetaHook 提供。迁移不更改接口版本或导出约定。
功能及启动参数见 [中文文档](docs/VGUI2ExtensionCN.md) 和 [English](docs/VGUI2Extension.md)。

## gamedata

`scripts/manifests/vgui2extension.json` 声明当前插件使用的私有符号及其版本范围，覆盖 21 个
游戏快照。清单依据 Windows 符号和实际源码调用整理，包含原宿主 gate 尚未列全的
PropertySheet、Menu、FocusNavGroup、选项页和 Panel 尺寸补丁。
KeyValues 两种名称按当前各版本发布情况要求；上游若变更名称，需同步调整 manifest。

同步与校验脚本沿用 Renderer 的 manifest 流程，catalog 安装在
`metahook/gamedata/vgui2extension/`，由宿主与其他 catalog 合并。
非 HL25 的 gameui/serverbrowser 尺寸补丁按连续编号保留，而非固定补丁数量；
同步器为此扩展了条件组中的模块限定编号补丁支持。

## 验证

```bat
ctest --test-dir build/x86/Release -C Release --output-on-failure
ctest --test-dir build/x86/Debug -C Debug --output-on-failure
python -m unittest discover -s scripts/tests -v
python scripts/validate-gamedata.py install/x86/Release/svencoop/metahook/gamedata/vgui2extension --manifest scripts/manifests/vgui2extension.json
```

`cmake/Sources.cmake` 显式保留原工程 129 个编译单元。原有未编译的 `parsemsg.cpp` 和
`steam_api.cpp` 保留在 src 中，但不加入编译。两组 C++ 测试迁至 `src/tests/`，Release 仍启用断言。
普通 Debug 使用 `/MTd`，Release 使用 `/MT`、LTCG、`/OPT:REF` 和 `/OPT:ICF`，均生成 PDB。

GitHub Actions 沿用 Renderer 的 LiveBuild 和 tag Release 结构，消费同级 MetaHook SDK/SDL 头，
构建 Release、运行测试、校验 catalog，并将 `svencoop/` 与 `platform/` 打包为
`VGUI2Extension-windows-x86.7z`。接口头不放入运行包。工作流尚未在远程运行。

本地验证记录见 [memory/build_and_verification.md](memory/build_and_verification.md)。
构建及模拟测试不等同于游戏内插件加载、HiDPI 和实际 IME 行为验证。
许可证见 [LICENSE](LICENSE)。
