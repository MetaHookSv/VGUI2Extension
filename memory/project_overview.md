---
title: project_overview
type: note
permalink: vgui2extension/project-overview
---

# VGUI2Extension

VGUI2Extension 是 MetaHook 的 UI 扩展插件，为其他插件提供修改 GoldSrc VGUI2 组件的能力，并提供字体与 HiDPI 支持、游戏语言覆盖和输入法处理。

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

## 对外文档

`README.md` 为英文首页，`README.zh-CN.md` 为中文首页；结构对齐独立 Renderer 与 MetaHook。
详细文档按构建、安装、功能、gamedata 和 CI 分页，双语分别位于 `docs/en/` 与 `docs/zh-CN/`，
页首提供返回对应 README 和语言切换的相对链接。README 统一链接到双语主题页面。
gamedata 页面集中维护运行要求及按 module/kind/版本条件列出的符号清单，包含 92 条显式记录
和 5 组连续编号补丁；manifest 发布条件与源码运行条件分别说明，不能把清单豁免当作运行时可选。

更多信息见 `README.md`、`docs/zh-CN/features.md`、`memory/build_and_verification.md`。
