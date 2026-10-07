[返回 README](../../README.zh-CN.md) | [English](../en/gamedata.md)

# gamedata

本页介绍插件的运行时 catalog 要求及实际使用的 GameSymbols。
同步与校验命令见[构建说明](build-instruction.md#gamedata)，部署方式见[安装说明](installation.md)。

## 运行要求

将插件 catalog 安装到 `<mod>/metahook/gamedata/vgui2extension/`，随 DLL 和资源一起更新。
MetaHook 必须支持合并嵌套 catalog，并提供 API 115 或更新版本；
其 API 版本还须不低于编译插件时使用的 SDK 版本。
插件通过宿主 catalog API 解析 gamedata 覆盖的私有符号。

GameUI 需要 PropertySheet 方法和 `_activePage`、FocusNavGroup 的 `GetCurrentFocus` 与
`_currentFocus`、三个选项页的 `OnApplyChanges`，以及 `CBasePanel::ApplySchemeSettings`。
ServerBrowser 需要匹配的 KeyValues `LoadFromFile` 记录。
PropertySheet 虚调用使用当前对象的 vtable，以及从 gamedata 查询的槽位。

所有支持的 Windows GameUI 身份还需要
`vgui2_EditablePanel_OnSizeChanged_call_GetChild_callsite_0`。
仅此调用点将 popup 子窗口过滤为空指针，避免父窗口调整大小时移动或裁剪其屏幕坐标范围。
其他 `GetChild` 调用及普通子控件保持原有行为。
HL25 下还将 ContentControlDialog 资源的默认宽度从 328 增加到 448，再进行比例缩放，
为 AddonsFolder 复选框留出空间。

Menu 处理需要 `vgui2::Menu.m_pScroller` 和 `vgui2::Menu::MakeItemsVisibleInScrollRange()`。
该方法直接解析，在目标 GameUI 二进制中没有显式参数。
hook 仍依赖 Legacy/HL25 中 `m_pScroller` 相邻成员的布局；PropertySheet 的 `_pageTabs`
也仍依赖旧版数组布局。

`IInput2::PostKeyMessage` 通过 gamedata，在 `vgui2.dll` 中直接解析
`CInputWin32::PostKeyMessage(KeyValues*)`。旧 catalog 缺少必要记录时无法解析这些 hook。

## GameSymbols 清单

声明来源为 [scripts/manifests/vgui2extension.json](../../scripts/manifests/vgui2extension.json)。
以下清单保留 manifest 中的完整 module、名称与 kind，并已对照源码调用核对。
共 5 个 module、**93 条显式记录**，其中包含 **5 组连续编号补丁**的首条 `_0` 记录。
每组编号补丁还可能消费后续记录，因此 93 不是每份安装 catalog 的记录总数。

| Module | 显式记录数 |
| --- | ---: |
| `engine` | 14 |
| `client` | 15 |
| `gameui` | 58 |
| `serverbrowser` | 5 |
| `vgui2` | 1 |
| 合计 | 93 |

GameSymbol 由 module 与名称共同定位。`engine`、`client`、`gameui`、`serverbrowser`
中的同名符号是独立记录。下列条件表示 manifest 的 catalog 裁剪与校验范围；
源码在该范围内还可能进行可选或条件解析。部分游戏共用引擎或 UI 二进制，
不能把这些条件组直接当作运行时 mod 目录判断。

### 符号种类

| Kind | 本插件中的用途 |
| --- | --- |
| `function` | 解析函数入口 |
| `global` | 解析全局变量地址 |
| `virtualFunction` | 解析虚函数入口，或查询所属 vtable 槽位 |
| `structMember` | 查询成员相对对象起点的字节偏移 |
| `patch` | 解析需重定向或修改的指令、分支位置 |

解析封装位于 [plugins.h](../../src/plugins.h)。必需解析失败时报告错误，可选解析可返回空指针。
查询 `virtualFunction` 的槽位使用 API 115 的 `mh_gamesymbol_t::vfuncIndex`。

### Manifest 条件

`All` 表示 manifest 顶层的 `gameVersions` 列表；`G1` 至 `G18` 按顺序对应当前
`conditionalGroups` 的条目。`cl_time` 的豁免见[运行条件与可选记录](#运行条件与可选记录)。

| 条件 | Manifest 中的 gameVersions |
| --- | --- |
| All | `cof-5936`, `cstrike-10210`, `cstrike-3248`, `cstrike-3647`, `cstrike-4554`, `cstrike-6153`, `cstrike-8684`, `czero-10210`, `czero-8684`, `czeror-10210`, `czeror-8684`, `hl-10210`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G1 | `cof-5936`, `hl-10210`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684` |
| G2 | `cof-5936`, `hl-10210`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G3 | `cof-5936`, `hl-10210`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G4 | `cof-5936`, `hl-10210`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G5 | `cof-5936`, `hl-10210`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G6 | `cof-5936`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554` |
| G7 | `cof-5936`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G8 | `cstrike-10210`, `cstrike-3248`, `cstrike-3647`, `cstrike-4554`, `cstrike-6153`, `cstrike-8684`, `czero-10210`, `czero-8684` |
| G9 | `cstrike-10210`, `cstrike-3248`, `cstrike-3647`, `cstrike-4554`, `cstrike-6153`, `cstrike-8684`, `czero-10210`, `czero-8684`, `czeror-10210`, `czeror-8684` |
| G10 | `cstrike-10210`, `cstrike-4554`, `cstrike-6153`, `cstrike-8684`, `czero-10210`, `czero-8684`, `czeror-10210`, `czeror-8684` |
| G11 | `cstrike-3248`, `cstrike-3647` |
| G12 | `czeror-10210`, `czeror-8684` |
| G13 | `hl-10210`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G14 | `hl-3248`, `hl-3266`, `hl-3329` |
| G15 | `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554` |
| G16 | `cof-5936`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G17 | `cof-5936`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G18 | `hl-10210` |

## 按 module 列出的符号

### engine

源码：[privatefuncs.cpp](../../src/privatefuncs.cpp)、
[EngineSurfaceHook.cpp](../../src/EngineSurfaceHook.cpp) 和 [exportfuncs.cpp](../../src/exportfuncs.cpp)。

| GameSymbol | Kind | Manifest 条件 |
| --- | --- | --- |
| `cl_time` | `global` | All |
| `cl_oldtime` | `global` | G2 |
| `g_ScissorRect` | `global` | G2 |
| `g_bScissor` | `global` | G2 |
| `g_pClientFactory` | `global` | G2 |
| `host_parms` | `global` | G2 |
| `pmainwindow` | `global` | G2 |
| `staticEngineSurface` | `global` | G2 |
| `vgui2::Panel::Init(int, int, int, int)` | `function` | G2 |
| `FileSystem_AddFallbackGameDir_V_strncpy_callsite_0` | `patch` | G4 |
| `FileSystem_SetGameDirectory_V_strncpy_callsite_0` | `patch` | G4 |
| `CGame::WindowProc` | `function` | G6 |
| `VGUIClient001_CreateInterface` | `patch` | G13 |
| `Sys_GetRegKeyValueUnderRoot` | `function` | G15 |

### client

源码：[ClientVGUI.cpp](../../src/ClientVGUI.cpp) 和 [privatefuncs.cpp](../../src/privatefuncs.cpp)。

| GameSymbol | Kind | Manifest 条件 |
| --- | --- | --- |
| `g_iVisibleMouse` | `global` | G5 |
| `CounterStrikeViewport.m_pCSBackGround` | `structMember` | G8 |
| `CounterStrikeViewport::CCSBackGroundPanel.m_offsetX` | `structMember` | G8 |
| `CounterStrikeViewport::CCSBackGroundPanel.m_offsetY` | `structMember` | G8 |
| `CounterStrikeViewport::CCSBackGroundPanel::Activate()` | `virtualFunction` | G8 |
| `CTeamMenu::LoadMapPage(char const*)` | `function` | G9 |
| `vgui2::Frame::Activate()` | `virtualFunction` | G9 |
| `vgui2::Frame::LoadControlSettings(char const*, char const*)` | `function` | G9 |
| `vgui2::Panel::Init(int, int, int, int)` | `function` | G9 |
| `vgui2::RichText::SetText(wchar_t const*)` | `function` | G9 |
| `KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)` | `virtualFunction` | G10 |
| `vgui2::KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)` | `virtualFunction` | G11 |
| `CWorldMap::PaintBackground()` | `virtualFunction` | G12 |
| `CWorldMapMissionSelect::PaintBackground()` | `virtualFunction` | G12 |
| `CZEROViewPort.m_pWorldMapPanel` | `structMember` | G12 |

### gameui

源码：[GameUI.cpp](../../src/GameUI.cpp)。

| GameSymbol | Kind | Manifest 条件 |
| --- | --- | --- |
| `CCareerBotFrame::CCareerBotFrame(vgui2::Panel*)` | `function` | G1 |
| `CCareerMapFrame::CCareerMapFrame(vgui2::Panel*)` | `function` | G1 |
| `CCareerProfileFrame::CCareerProfileFrame(vgui2::Panel*)` | `function` | G1 |
| `CBasePanel::ApplySchemeSettings(vgui2::IScheme*)` | `virtualFunction` | G2 |
| `CBasePanel::CBasePanel()` | `function` | G2 |
| `CCreateMultiplayerGameDialog::CCreateMultiplayerGameDialog(vgui2::Panel*)` | `function` | G2 |
| `CGameConsoleDialog::CGameConsoleDialog()` | `function` | G2 |
| `COptionsDialog::COptionsDialog(vgui2::Panel*)` | `function` | G2 |
| `COptionsSubAudio::COptionsSubAudio(vgui2::Panel*)` | `function` | G2 |
| `COptionsSubAudio::OnApplyChanges()` | `virtualFunction` | G2 |
| `COptionsSubMultiplayer::COptionsSubMultiplayer(vgui2::Panel*)` | `function` | G2 |
| `COptionsSubMultiplayer::OnApplyChanges()` | `virtualFunction` | G2 |
| `COptionsSubVideo::COptionsSubVideo(vgui2::Panel*)` | `function` | G2 |
| `COptionsSubVideo::OnApplyChanges()` | `virtualFunction` | G2 |
| `CTaskbar::CTaskbar(vgui2::Panel*, char const*)` | `function` | G2 |
| `CTaskbar::OnCommand(char const*)` | `virtualFunction` | G2 |
| `TabCatchingTextEntry::OnKeyCodeTyped(vgui2::KeyCode)` | `virtualFunction` | G2 |
| `vgui2::FocusNavGroup._currentFocus` | `structMember` | G2 |
| `vgui2::FocusNavGroup::GetCurrentFocus()` | `virtualFunction` | G2 |
| `vgui2::Menu.m_pScroller` | `structMember` | G2 |
| `vgui2::Menu::MakeItemsVisibleInScrollRange()` | `virtualFunction` | G2 |
| `vgui2::MessageBox::ApplySchemeSettings(vgui2::IScheme*)` | `virtualFunction` | G2 |
| `vgui2::MessageBox::MessageBox(char const*, char const*, vgui2::Panel*)` | `function` | G2 |
| `vgui2::Panel::Init(int, int, int, int)` | `function` | G2 |
| `vgui2::PropertyDialog._propertySheet` | `structMember` | G2 |
| `vgui2::PropertySheet._activePage` | `structMember` | G2 |
| `vgui2::PropertySheet::AddPage(vgui2::Panel*, char const*)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::ApplyChanges()` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::ChangeActiveTab(int)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::DeletePage(vgui2::Panel*)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::DisablePage(char const*)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::EnablePage(char const*)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::GetActivePage()` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::GetActivePageNum()` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::GetActiveTab()` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::GetActiveTabTitle(char*, int)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::GetNumPages()` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::GetPage(int)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::GetTabTitle(int, char*, int)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::HasHotkey(wchar_t)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::PerformLayout()` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::ResetAllData()` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::SetActivePage(vgui2::Panel*)` | `virtualFunction` | G2 |
| `vgui2::PropertySheet::SetTabWidth(int)` | `virtualFunction` | G2 |
| `vgui2::RichText carriage-return filter branch` | `patch` | G2 |
| `vgui2::RichText::InsertString(wchar_t const*)` | `function` | G2 |
| `vgui2::RichText::OnThink()` | `virtualFunction` | G2 |
| `vgui2::TextEntry::GetStartDrawIndex(int&)` | `virtualFunction` | G2 |
| `vgui2::TextEntry::LayoutVerticalScrollBarSlider()` | `virtualFunction` | G2 |
| `vgui2_EditablePanel_OnSizeChanged_call_GetChild_callsite_0` | `patch` | G2 |
| `KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)` | `virtualFunction` | G3 |
| `COptionsSubVideo::ApplyVidSettings(bool)` | `function` | G7 |
| `vgui2::MessageBox::ApplySchemeSettings to vgui2::Panel::SetSize callsite` | `patch` | G7 |
| `vgui2::RichText::InsertChar(wchar_t)` | `function` | G7 |
| `vgui2::KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)` | `virtualFunction` | G14 |
| `vgui2_Panel_SetSize_Const_callsite_0` | `patch` | G16 |
| `vgui2_Panel_SetMinimumSize_Const_callsite_0` | `patch` | G16 |
| `vgui2_Panel_SetBounds_Const_callsite_0` | `patch` | G16 |
| `vgui2_Panel_SetBounds_Const_callsite_0` | `patch` | G18 |

### serverbrowser

源码：[GameUI.cpp](../../src/GameUI.cpp) 中的 ServerBrowser 处理逻辑。

| GameSymbol | Kind | Manifest 条件 |
| --- | --- | --- |
| `vgui2::Panel::Init(int, int, int, int)` | `function` | G2 |
| `KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)` | `virtualFunction` | G3 |
| `vgui2::KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)` | `virtualFunction` | G14 |
| `vgui2_Panel_SetSize_Const_callsite_0` | `patch` | G17 |
| `vgui2_Panel_SetMinimumSize_Const_callsite_0` | `patch` | G17 |

### vgui2

源码：[InputWin32.cpp](../../src/InputWin32.cpp)。

| GameSymbol | Kind | Manifest 条件 |
| --- | --- | --- |
| `CInputWin32::PostKeyMessage(KeyValues*)` | `function` | G2 |

## 连续编号补丁

[GameUI.cpp 中的 PatchPanelSizeCallsites](../../src/GameUI.cpp) 使用
`n = 0, 1, 2, ...` 构造下列名称。首条记录必需，后续记录可选查询，遇到首个编号缺失即停止。
`SetSize` / `SetMinimumSize` 两组用于非 HL25 引擎，包括 SvEngine；`hl-10210` 这一 HL25
引擎只发布 `SetBounds` 组（另外两组在上游以 `_ScaledConst` 变体出现），因此单独使用 G18
条件。`hl-10210` 不能并入 G16——G16 要求三个前缀同时存在。

| Module | GameSymbol 编号模式 | Kind | Manifest 条件 |
| --- | --- | --- | --- |
| `gameui` | `vgui2_Panel_SetSize_Const_callsite_<n>` | `patch` | G16 |
| `gameui` | `vgui2_Panel_SetMinimumSize_Const_callsite_<n>` | `patch` | G16 |
| `gameui` | `vgui2_Panel_SetBounds_Const_callsite_<n>` | `patch` | G16、G18 |
| `serverbrowser` | `vgui2_Panel_SetSize_Const_callsite_<n>` | `patch` | G17 |
| `serverbrowser` | `vgui2_Panel_SetMinimumSize_Const_callsite_<n>` | `patch` | G17 |

manifest 显式列出每组的首条 `_0`，并通过 `numberedPatchSets` 保留后续连续记录。
manifest 的 `prefix` 不含末尾下划线；源码前缀包含该下划线，再拼接编号。
更新 catalog 时需保留 module 和版本条件。

G18 保留 `hl-10210` 发布的 `SetBounds` callsite，但运行期不再重定向这些调用：
HL25 已进行尺寸缩放，且对话框资源会覆盖构造函数尺寸。ContentControlDialog 在资源
缩放前加宽，独立的 `OnSizeChanged` 补丁则阻止父窗口后续布局再次裁剪它。

engine 的两个 `V_strncpy_callsite_0` 语言补丁是固定名称，不属于这些连续编号组。

## 运行条件与可选记录

- manifest 对 G9 中十个 CS/CZ/CZDS 版本豁免 `engine / cl_time`。
  这是 catalog 校验豁免；源码仍执行必需解析，不能将其理解为运行时可选。
- 先查询 `engine / VGUIClient001_CreateInterface`；缺少该记录时，
  旧版路径改为必需解析 `engine / g_pClientFactory`。其他查询错误会报告错误。
- `engine / Sys_GetRegKeyValueUnderRoot` 为可选解析；不可用时，
  两个文件系统 `V_strncpy_callsite_0` 补丁均为必需。
  SDL2 的 `SDL_GetWindowWMInfo` 路径不可用时使用 `CGame::WindowProc`。
- 客户端没有原生 VGUI2 时，可选解析 `client / g_iVisibleMouse`。
  原生客户端的 `Panel::Init` 与 KeyValues `LoadFromFile` 也使用可选解析。
- 每个 KeyValues 查询先尝试 `vgui2::KeyValues::LoadFromFile(...)`，
  再尝试 `KeyValues::LoadFromFile(...)`。
  GameUI 与 ServerBrowser 各自 module 必须有一个匹配记录；client 查询仍为可选。
- Career 构造函数在 Condition Zero、CZDS 下请求，但因 UI 二进制共用，
  manifest 记录位于 HL/COF GameUI catalog 身份组中。不能仅凭 mod 目录名判断 catalog 条件组。
- `COptionsSubVideo::ApplyVidSettings(bool)` 为可选解析，在 `hl-10210` 中已内联到
  `OnApplyChanges()`。`RichText::InsertChar(wchar_t)` 为可选解析；
  缺少时改为必需解析 `RichText::InsertString(wchar_t const*)`。
- CS/CZ 背景面板成员及 `Activate()` 排除 CZDS；世界地图记录用于 CZDS。
  通用 CS 系列 Frame 和菜单记录涵盖三种变体。
- FocusNavGroup、选项页和 CBasePanel 回调 hook 的部分记录在首次安装对应 hook 时解析。
  清单列出所有可能的消费，不表示启动时一次性查询全部记录。

符号名称、kind、module、版本条件或连续编号组变化时，随 manifest 更新本清单。
