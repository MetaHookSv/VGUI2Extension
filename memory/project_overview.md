---
title: project_overview
type: note
permalink: vgui2extension/project-overview
---

# VGUI2Extension

独立 Windows x86 插件，源码迁自 MetaHookSv `fe80b6d60bfb487b52aed7ea7ec0492e7b27a5d2`。
参考独立 Renderer 的 CMake、SDK 消费、安装和 gamedata 打包结构。
原 MetaHookSv 与其他参考仓库未改动。

## 职责与入口

- `src/plugins.cpp`：MetaHook IPluginsV4 生命周期，加载引擎和客户端时安装 hooks。
- `src/VGUI2ExtensionInternal.cpp`：VGUI2 回调注册及按 altitude 排序的前后调用分发。
- `src/BaseUI.cpp`、`GameUI.cpp`、`ClientVGUI.cpp`：引擎 UI、GameUI/ServerBrowser 和客户端 UI 接管。
- `src/Surface2.cpp`、`Scheme2.cpp`、`Font*.cpp`：surface、scheme、字体代理。
- `src/DpiManagerInternal.cpp`：HiDPI 和 SKIN 搜索路径。
- `src/InputWin32.cpp`、`exportfuncs.cpp`、`IMEWindowMessage.h`：Win32/SDL 输入法路径和消息归属。
- `src/LanguageRegistry.h`：旧引擎 Steam 语言注册表覆盖逻辑。

## 依赖与公共接口

MetaHook 提供 API 115、共享 HLSDK/SourceSDK/VGUI 源码及运行时 gamedata 解析。
SDL2/SDL3 只消费外部头文件；运行时接口保留原获取方式。
公共接口由本仓库提供：IVGUI2Extension、IDpiManager、ISurface2、IScheme2、IInput2。
CaptionMod、BulletPhysics、Renderer、SCModelDownloader 是主要消费者。

## 构建与数据流

`scripts/build-VGUI2Extension-x86-{Debug,Release}.bat` → CMake → 编译 DLL → install。
`scripts/manifests/vgui2extension.json` → 同步/裁剪/校验 → 独立 catalog → 宿主递归合并。
编译显式保留 129 个单元，共享 SDK 文件来自外部 MetaHook。
资源来自原 `Build/svencoop/vgui2ext/` 与 `Build/platform/` 的两份中文本地化文件。
测试包含语言注册表、IME 消息以及条件编号补丁裁剪。

## 当前代码优先于旧笔记

原 MetaHookSv memory 中声称 IInput2 已升为 006，但本次源提交的头和注册代码仍为
`VGUI_Input2_005`，且含 `CancelIMEComposition`。本次迁移原样保留这一状态；
未引入接口版本变更，也不复制旧笔记中有关 006 别名的错误结论。

更多信息见 `README.md`、`docs/VGUI2Extension.md`、`memory/build_and_verification.md`。
