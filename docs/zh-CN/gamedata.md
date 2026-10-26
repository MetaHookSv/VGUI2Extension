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

GameUI 宽字符 `ComboBox::AddItem` hook 需要 GoldSrc_VibeSignatures `v20261013a`
发布的 ComboBox/Menu 记录。它按 UTF-8 实际字节数（含终止符）分配缓冲区，保留宽字符
`SetText` 消息与 userData，并通过 catalog 中的虚函数槽位调用 `GetMenu` 和四参数
`AddMenuItem`。CS/CZ 客户端现在复用该新增逻辑，并 hook 宽字符 `UpdateItem`：
菜单文字及 `SetText` 消息全程保持宽字符；无效 ID 返回 false，成功更新保留延迟布局刷新。
客户端需要本模块的 `AddItemW`、`UpdateItemW`、`IsItemIDValid`、`GetMenu`、四参数
Menu `AddMenuItem`、宽字符 Menu `UpdateMenuItem` 和 Panel `InvalidateLayout` 记录。
选中玩家名称还通过 `CBottomBar::SetPlayerText` hook 修复，使用客户端的 `m_pPlayerList`
成员偏移和原生 `TextEntry::SetText(char const*)` 槽位，按实际 UTF-8 字节数转换文本。
应将扩展后的 GoldSrc_VibeSignatures catalog 与插件一起部署；`v20261016b` 尚不包含新增依赖。
Windows CS/CZ 10210 的地图说明实现内联在四参数 `CTeamMenu::Update` 中；其余已支持的
旧 Windows 客户端仍使用单参数 `CTeamMenu::LoadMapPage`。插件根据发布的真实身份选择
匹配的 wrapper，并在原生 UI hook 解析前初始化游戏类型。不能将此 DLL 与错误地把
Update 标为 LoadMapPage 的旧 catalog 搭配部署。

零点行动任期模式的 bot 费用修复需要
[GoldSrc_VibeSignatures #378](https://github.com/HLND2T/GoldSrc_VibeSignatures/pull/378)
新增的四项记录。仅在 `czero` 中安装 hook：先调用原始 `CBotButton::SetCharacter`，
再以完整整数和 `#Career_Cost` 更新费用文字。TextImage 成员偏移与宽字符 SetText
虚槽位均来自 gamedata；原生重载没有额外的 `bool` 参数。其他游戏不安装此 hook。

任期模式的玩家名修复需要 GoldSrc_VibeSignatures `v20261016b` 发布的五项记录。
仅在 `czero` 中 hook 难度框构造函数与基类 Activate：先调用原函数，再将完整的
UTF-8 name cvar 显式转换成 Unicode，通过 gamedata 提供的宽字符 Label 虚槽位更新文字。
标题保留从公开 `CareerUI001` 接口取得的当前难度及其本地化文本；两个成员偏移均由
gamedata 提供，包含继承自 `CCareerBaseBox` 的 `m_pTextLabel`。替换文字不再依赖
CRT locale，也不经过原有的 31 字节截断。无效 UTF-8 或缺失本地化时保留原文字。

零点行动购买包编辑界面的 popup 层级修复需要 GoldSrc_VibeSignatures `v20261020a`
发布的三个 client 记录：`CounterStrikeViewport::OnTick()`、静态函数
`CCareerBaseBox::BringToFront()` 和指针存储位置 `CCareerBaseBox::s_currentBox`。
仅在原生 Tick 内，当前可见对话框已是 app-modal surface 且有可见子 popup 时，
跳过重复激活，防止对话框覆盖下拉菜单。首次激活、其他调用方和模态设置保留原行为；
关闭 popup 后恢复原生处理。仅 `czero` 安装这组 hook；CS 和 CZDS 不发布这组 client 记录。

任期回合结束窗口的 ESC 修复还需要 client 的 `CCSCareerRoundEndMenu` 虚表及
`vgui2::Frame::OnKeyCodePressed(vgui2::KeyCode)`、
`vgui2::Frame::OnKeyCodeTyped(vgui2::KeyCode)` 的虚槽位，已通过
[GoldSrc_VibeSignatures #397](https://github.com/HLND2T/GoldSrc_VibeSignatures/issues/397)
请求补齐。修改 RoundEnd 的虚表：ESC 保留待确认窗口，引擎仍切换主菜单；其他按键与
原有按钮命令照常处理。两个 KeyCode 入口均受保护，包括 Typed 的 app-modal 关闭路径。
这三项记录已在 `v20261023a` 发布，旧 catalog 无法通过扩展后的 manifest，也无法在
`czero` 中运行此 DLL；应搭配匹配 catalog 部署。其他游戏不解析或安装这组 hook。

投降确认框的回归还需要 `CCSCareerRoundEndMenu::OnCommand(char const*)`，已通过
[GoldSrc_VibeSignatures #399](https://github.com/HLND2T/GoldSrc_VibeSignatures/issues/399)
请求补齐。RoundEnd 虚表中的命令 hook 忽略子确认框转发的通用 `Close` / `Cancel`，
保留待确认的 Career 选择；业务命令继续由原生处理。这第四项记录已在 `v20261023b`
发布；最终 DLL 应搭配该版本或更新 catalog，`v20261023a` 不满足扩展后的 manifest。

MatchEnd 的 ESC 修复还需要 client 的 `CCSCareerMatchEndMenu` 虚表与
`CCSCareerMatchEndMenu::OnCommand(char const*)` 虚槽位。相同的按键和通用命令保护安装在
MatchEnd 自己的虚表上，切换 GameUI 时保留当前音乐和待确认选择；完成、重试、Credits
继续交给原生处理。两个 Frame 按键槽位与 RoundEnd 共用，不修改共享 Frame 实现或音乐处理。
这两项记录已通过 [GoldSrc_VibeSignatures #409](https://github.com/HLND2T/GoldSrc_VibeSignatures/issues/409)
请求补齐，已在 `v20261026a` 发布。在 `czero` 使用此 DLL 时，应部署该版本或更新的
catalog；旧版本无法通过扩展后的 manifest 校验。

原生构建编辑器 DPI hook 需要 `v20261025b` 或更新版本。它们 hook GameUI 构造入口和
编辑器/列表虚表，保留原生编辑、保存和所有权行为；行列布局及滚动条修正只作用于编辑器列表。
原生编辑器继承 Frame 的 scheme 处理，子控件已使用比例字体，因此不修改共享 Frame 函数。
已保存的窗口尺寸保持像素单位，仅缩放缺失的默认尺寸和设计常量。列表 item ID 通过独立发布的
两个 `m_SortedItems` 字段读取，不将原生容器或对话框强转为 SDK 私有类。

声明来源为 [scripts/manifests/vgui2extension.json](../../scripts/manifests/vgui2extension.json)。
以下清单保留 manifest 中的完整 module、名称与 kind，并已对照源码调用核对。
共 5 个 module、**258 条不同的显式记录**，其中包含条件编号补丁组的首条 `_0` 记录
（共 23 处声明）。每组编号补丁还可能消费后续记录，因此 258 不是每份安装 catalog 的记录总数。

| Module | 显式记录数 |
| --- | ---: |
| `engine` | 14 |
| `client` | 39 |
| `gameui` | 195 |
| `serverbrowser` | 9 |
| `vgui2` | 1 |
| 合计 | 258 |

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
| `vtable` | 解析模块自身的虚表，用于公开接口 hook |
| `scalar` | 读取对应 identity 的容量、ABI 或 hook 策略整数 |
| `structMember` | 查询成员相对对象起点的字节偏移 |
| `patch` | 解析需重定向或修改的指令、分支位置 |

解析封装位于 [plugins.h](../../src/plugins.h)。必需解析失败时报告错误，可选解析可返回空指针。
查询 `virtualFunction` 的槽位使用 API 115 的 `mh_gamesymbol_t::vfuncIndex`。

### Manifest 条件

`All` 表示 manifest 顶层的 `gameVersions` 列表；`G1` 至 `G28` 按顺序对应当前
`conditionalGroups` 的条目。`cl_time` 的豁免见[运行条件与可选记录](#运行条件与可选记录)。

| 条件 | Manifest 中的 gameVersions |
| --- | --- |
| All | `cof-5936`, `cstrike-10210`, `cstrike-3248`, `cstrike-3647`, `cstrike-4554`, `cstrike-6153`, `cstrike-8684`, `cstrikegameui-8684`, `czero-10210`, `czero-8684`, `czeror-10210`, `czeror-8684`, `decay`, `hl-10210`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G1 | `cof-5936`, `hl-10210`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684` |
| G2 | `cof-5936`, `hl-10210`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G3 | `cof-5936`, `hl-10210`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G4 | `cof-5936`, `hl-10210`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
| G5 | `cof-5936`, `decay`, `hl-10210`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |
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
| G19 | `cof-5936` |
| G20 | `cstrike-3248`, `cstrike-3647`, `cstrike-4554`, `cstrike-6153`, `cstrike-8684`, `czero-8684`, `czeror-8684` |
| G21 | `cstrike-10210`, `czero-10210`, `czeror-10210` |
| G22 | `hl-8684`, `hl-10210`, `svencoop-8948`, `svencoop-10257` |
| G23 | `hl-10210` |
| G24 | `czero-10210`, `czero-8684` |
| G25 | `hl-10210` |
| G26 | `hl-10210`, `hl-8684`, `cstrikegameui-8684` |
| G27 | `cstrikegameui-8684` |
| G28 | `cof-5936`, `cstrikegameui-8684`, `hl-10210`, `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554`, `hl-6153`, `hl-8684`, `svencoop-10257`, `svencoop-8948` |

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
| `FileSystem_AddFallbackGameDir_V_strncpy_callsite_1` | `patch` | G19 |
| `FileSystem_SetGameDirectory_V_strncpy_callsite_1` | `patch` | G19 |
| `CGame::WindowProc` | `function` | G6 |
| `VGUIClient001_CreateInterface` | `patch` | G13 |
| `Sys_GetRegKeyValueUnderRoot` | `function` | G15 |

### client

源码：[ClientVGUI.cpp](../../src/ClientVGUI.cpp) 和 [privatefuncs.cpp](../../src/privatefuncs.cpp)。

| GameSymbol | Kind | Manifest 条件 |
| --- | --- | --- |
| `g_iVisibleMouse` | `global` | G5 |
| `CounterStrikeViewport::OnTick()` | `virtualFunction` | G24 |
| `CCareerBaseBox::BringToFront()` | `function` | G24 |
| `CCareerBaseBox::s_currentBox` | `global` | G24 |
| `CCSCareerRoundEndMenu` | `vtable` | G24 |
| `CCSCareerRoundEndMenu::OnCommand(char const*)` | `virtualFunction` | G24 |
| `CCSCareerMatchEndMenu` | `vtable` | G24 |
| `CCSCareerMatchEndMenu::OnCommand(char const*)` | `virtualFunction` | G24 |
| `vgui2::Frame::OnKeyCodePressed(vgui2::KeyCode)` | `virtualFunction` | G24 |
| `vgui2::Frame::OnKeyCodeTyped(vgui2::KeyCode)` | `virtualFunction` | G24 |
| `CounterStrikeViewport.m_pCSBackGround` | `structMember` | G8 |
| `CounterStrikeViewport::CCSBackGroundPanel.m_offsetX` | `structMember` | G8 |
| `CounterStrikeViewport::CCSBackGroundPanel.m_offsetY` | `structMember` | G8 |
| `CounterStrikeViewport::CCSBackGroundPanel::Activate()` | `virtualFunction` | G8 |
| `CTeamMenu::LoadMapPage(char const*)` | `function` | G20 |
| `CTeamMenu::Update(char const*, bool, char const**, int)` | `function` | G21 |
| `vgui2::ComboBox::GetMenu()` | `virtualFunction` | G9 |
| `vgui2::ComboBox::IsItemIDValid(int)` | `virtualFunction` | G9 |
| `vgui2::Panel::InvalidateLayout(bool, bool)` | `virtualFunction` | G9 |
| `CBottomBar::SetPlayerText(wchar_t const*)` | `function` | G9 |
| `CBottomBar.m_pPlayerList` | `structMember` | G9 |
| `vgui2::TextEntry::SetText(char const*)` | `virtualFunction` | G9 |
| `vgui2::ComboBox::AddItem(wchar_t const*, KeyValues const*)` | `virtualFunction` | G10 |
| `vgui2::ComboBox::UpdateItem(int, wchar_t const*, KeyValues const*)` | `virtualFunction` | G10 |
| `vgui2::Menu::AddMenuItem(char const*, KeyValues*, vgui2::Panel*, KeyValues const*)` | `virtualFunction` | G10 |
| `vgui2::Menu::UpdateMenuItem(int, wchar_t const*, KeyValues*, KeyValues const*)` | `virtualFunction` | G10 |
| `vgui2::ComboBox::AddItem(wchar_t const*, vgui2::KeyValues const*)` | `virtualFunction` | G11 |
| `vgui2::ComboBox::UpdateItem(int, wchar_t const*, vgui2::KeyValues const*)` | `virtualFunction` | G11 |
| `vgui2::Menu::AddMenuItem(char const*, vgui2::KeyValues*, vgui2::Panel*, vgui2::KeyValues const*)` | `virtualFunction` | G11 |
| `vgui2::Menu::UpdateMenuItem(int, wchar_t const*, vgui2::KeyValues*, vgui2::KeyValues const*)` | `virtualFunction` | G11 |
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

源码：[GameUI.cpp](../../src/GameUI.cpp) 和
[BuildModeDialogHooks.cpp](../../src/BuildModeDialogHooks.cpp)。

| GameSymbol | Kind | Manifest 条件 |
| --- | --- | --- |
| `CCareerBotFrame::CCareerBotFrame(vgui2::Panel*)` | `function` | G1 |
| `CBotButton::SetCharacter(CCareerCharacter const*)` | `function` | G1 |
| `CBotButton.m_pText` | `structMember` | G1 |
| `CCareerDifficultyBox::CCareerDifficultyBox(char const*, vgui2::Panel*)` | `function` | G1 |
| `CCareerBaseFrame::Activate()` | `function` | G1 |
| `CCareerBaseBox.m_pTextLabel` | `structMember` | G1 |
| `CCareerBaseFrame.m_pPlayerName` | `structMember` | G1 |
| `vgui2::Label::SetText(wchar_t const*)` | `virtualFunction` | G1 |
| `CCareerCharacter::GetPoints() const` | `function` | G1 |
| `vgui2::TextImage::SetText(wchar_t const*)` | `virtualFunction` | G1 |
| `CCareerMapFrame::CCareerMapFrame(vgui2::Panel*)` | `function` | G1 |
| `CCareerProfileFrame::CCareerProfileFrame(vgui2::Panel*)` | `function` | G1 |
| `CBasePanel::ApplySchemeSettings(vgui2::IScheme*)` | `virtualFunction` | G2 |
| `CBasePanel::CBasePanel()` | `function` | G2 |
| `CCreateMultiplayerGameDialog::CCreateMultiplayerGameDialog(vgui2::Panel*)` | `function` | G2 |
| `CCreateMultiplayerGameGameplayPage::LoadGameOptionsList()` | `function` | G23 |
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
| `vgui2::ComboBox::AddItem(wchar_t const*, KeyValues const*)` | `virtualFunction` | G3 |
| `vgui2::ComboBox::AddItem(wchar_t const*, vgui2::KeyValues const*)` | `virtualFunction` | G14 |
| `vgui2::ComboBox::GetMenu()` | `virtualFunction` | G2 |
| `vgui2::Menu::AddMenuItem(char const*, KeyValues*, vgui2::Panel*, KeyValues const*)` | `virtualFunction` | G3 |
| `vgui2::Menu::AddMenuItem(char const*, vgui2::KeyValues*, vgui2::Panel*, vgui2::KeyValues const*)` | `virtualFunction` | G14 |
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
| `vgui2::BuildModeDialog` | `vtable` | G28 |
| `vgui2::BuildModeDialog::BuildModeDialog(vgui2::BuildGroup*)` | `function` | G28 |
| `vgui2::BuildModeDialog::PerformLayout()` | `virtualFunction` | G28 |
| `vgui2::BuildModeDialog::SetActiveControl(vgui2::Panel*)` | `virtualFunction` | G28 |
| `vgui2::BuildModeDialog.m_pAddNewControlCombo` | `structMember` | G28 |
| `vgui2::Frame::GetClientArea(int&, int&, int&, int&)` | `virtualFunction` | G28 |
| `vgui2::Panel::GetVPanel()` | `virtualFunction` | G28 |
| `vgui2::PanelListPanel` | `vtable` | G28 |
| `vgui2::PanelListPanel::PerformLayout()` | `virtualFunction` | G28 |
| `vgui2::PanelListPanel::GetItemCount()` | `virtualFunction` | G28 |
| `vgui2::PanelListPanel::GetItemLabel(int)` | `virtualFunction` | G28 |
| `vgui2::PanelListPanel::GetItemPanel(int)` | `virtualFunction` | G28 |
| `vgui2::PanelListPanel.m_SortedItems_m_pMemory` | `structMember` | G28 |
| `vgui2::PanelListPanel.m_SortedItems_m_Size` | `structMember` | G28 |
| `vgui2::PanelListPanel.m_iFirstColumnWidth` | `structMember` | G28 |
| `vgui2::PanelListPanel.m_iPanelBuffer` | `structMember` | G28 |

### serverbrowser

源码：[GameUI.cpp](../../src/GameUI.cpp) 中的 ServerBrowser 处理逻辑。

[ServerBrowserHistory.cpp](../../src/ServerBrowserHistory.cpp) 重定向日期 `strftime`
callsite，并通过公开 ABI 槽位 hook 本模块 KeyValues 的 `SetString`。使用显式的默认
LC_TIME locale 生成宽字符日期，保留原生窄字符格式化及大小写处理，仅在 `LastPlayed`
缓冲区匹配时转发至 `SetWString`。不 hook 两个所属函数。日期补丁要求
GoldSrc_VibeSignatures v20261017a 或更新版本；旧版过滤循环没有日期 callsite。
游戏语言为 `schinese` 或 `tchinese` 时显示 `YYYY-MM-DD HH:MM`（24 小时制）；
其他游戏语言保留现有本地化日期格式。

| GameSymbol | Kind | Manifest 条件 |
| --- | --- | --- |
| `vgui2::Panel::Init(int, int, int, int)` | `function` | G2 |
| `KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)` | `virtualFunction` | G3 |
| `vgui2::KeyValues::LoadFromFile(IFileSystem*, char const*, char const*)` | `virtualFunction` | G14 |
| `vgui2_Panel_SetSize_Const_callsite_0` | `patch` | G17 |
| `vgui2_Panel_SetMinimumSize_Const_callsite_0` | `patch` | G17 |
| `KeyValues` | `vtable` | G3 |
| `vgui2::KeyValues` | `vtable` | G14 |
| `CBaseGamesPage_ServerResponded_to_strftime_callsite_0` | `patch` | G2 |
| `CBaseGamesPage_ApplyGameFilters_to_strftime_callsite_0` | `patch` | G22 |

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
| `engine` | `FileSystem_SetGameDirectory_V_strncpy_callsite_<n>` | `patch` | G19 |
| `engine` | `FileSystem_AddFallbackGameDir_V_strncpy_callsite_<n>` | `patch` | G19 |

manifest 显式列出每组的首条 `_0`，并通过 `numberedPatchSets` 保留后续连续记录。
manifest 的 `prefix` 不含末尾下划线；源码前缀包含该下划线，再拼接编号。
更新 catalog 时需保留 module 和版本条件。

G18 保留 `hl-10210` 发布的 `SetBounds` callsite，但运行期不再重定向这些调用：
HL25 已进行尺寸缩放，且对话框资源会覆盖构造函数尺寸。ContentControlDialog 在资源
缩放前加宽，独立的 `OnSizeChanged` 补丁则阻止父窗口后续布局再次裁剪它。

engine 的两个语言补丁同样是连续编号的：多数引擎把 Steam 语言分支与默认 `english`
分支合并到同一个 `call`，因此 `_0` 即覆盖整个 owner；CoF 为每个分支各发射一次拷贝，
因此额外发布 `_1`。manifest 的 G19 条件组只对 `cof-5936` 保留该组，其他引擎组（G4）
只声明两条必需的 `_0`。

## 运行条件与可选记录

- HL25 默认服务器名修复需要新增的 `LoadGameOptionsList` 记录。原函数创建控件后，
  通过 VGUI2 控件树找到 `hostname` / `DescEdit` 并发送 UTF-8 `SetText` 消息。
  保留 Steam 昵称，并按完整 UTF-8 字符截断，使包含 `"'s Server"` 的结果最多为
  63 字节。无法取得 Steam API/昵称或 UTF-8 无效时保留原文字；用户编辑及启动、
  应用流程保持原样。新 catalog 必须与 DLL 一起部署；`v20261017a` 不含此记录。
- manifest 对 CS/CZ/CZDS 客户端身份、`cstrikegameui-8684` 和 `decay`
  豁免 `engine / cl_time`，这些身份不发布 engine 模块。
  这是 catalog 校验豁免；源码仍执行必需解析，不能将其理解为运行时可选。
- 先查询 `engine / VGUIClient001_CreateInterface`；缺少该记录时，
  旧版路径改为必需解析 `engine / g_pClientFactory`。其他查询错误会报告错误。
- `engine / Sys_GetRegKeyValueUnderRoot` 为可选解析；不可用时，
  两个文件系统 `V_strncpy_callsite_0` 补丁均为必需；`_1` 重复拷贝为可选解析，
  只在 `cof-5936` 上存在。
  SDL2 的 `SDL_GetWindowWMInfo` 路径不可用时使用 `CGame::WindowProc`。
- 客户端没有原生 VGUI2 时，可选解析 `client / g_iVisibleMouse`。
  原生客户端的 `Panel::Init` 与 KeyValues `LoadFromFile` 也使用可选解析。
- Half-Life: Decay（ModDB）使用 GoldSrc_VibeSignatures `v20261024a` 新增的
  无版本 tag `decay`。其 Windows client catalog 仅保留 `g_iVisibleMouse`；
  engine 与 UI 依赖由匹配的共享二进制身份提供。同步脚本显式允许 `decay`，
  继续拒绝其他无版本 tag。默认在线同步需要 Pages 索引发布 `decay`，仅有 release
  发布包仍不足以完成在线同步。Catalog 校验不代表已验证游戏内兼容性。
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

## 战役存储契约（G25/G26）

此组为 [#20](https://github.com/MetaHookSv/VGUI2Extension/issues/20) 的 u8hex
方案提供 92 条基础记录，对应[上游 #390](https://github.com/HLND2T/GoldSrc_VibeSignatures/issues/390)。
此契约依赖可保留上游 release
[v20261020b](https://github.com/HLND2T/GoldSrc_VibeSignatures/releases/tag/v20261020b)
发布的记录。运行实现还依赖 G26 的 14 条入口预检/输入记录；这批记录不在 v20261020b 中，
须先发布上游契约再分发消费端。本地验证使用 candidate catalog。

记录全部归属共享的 `hl-10210/gameui/windows`，不放入 `czero/client`，也不要求其他 GameUI identity
提供这些记录。安装还须在既有游戏类型初始化阶段确认 `g_bIsCZero && !g_bIsCZDS`。Linux 数据仅用于静态 ABI
交叉核验，不代表 Windows 插件支持 Linux。

manifest 明确列出函数、虚函数、成员、scalar 及 12 组连续编号调用点。DeleteProfile 路径和按钮双击
命令同样必需；Windows selected-name copy 组恰好有两项。难度框完整构造函数复用已有真实 symbolName。

每个 patch 同时消费 `_instruction_length`、`_hook_strategy` 与 `_abi` scalar。
策略 0 使用分支重定向。策略 1 是 BuildProfileList 内联读取中的 3 字节 filesystem Open 观察点：
hook 公开 Open 槽位，用 `site + length` 返回地址、真实 receiver、`rb` 和枚举 scope 过滤。
策略 2 是 ResumeProfile 的 2 字节 `call eax` 观察点：hook 公开 engine `Cvar_Set` 目标，用精确返回地址
与 scope 过滤。这两种观察点均不能写入五字节 CALL。原目标从真实公开实例/表取得，不能依靠间接
分支地址解析器。卸载时由现有 hook handle 恢复完整原指令或槽位，无需扩展 MetaHook 公共 API。

Windows hl-10210 GameUI 请求 `VFileSystem009`，实际使用旧版公开 `IFileSystem` ABI：
原生 Read/Write 调用的字节偏移分别为 `0x54`/`0x58`，Open/Close 为 `0x28`/`0x2c`。
应按真实 GameUI receiver 选择公开接口，不能仅按 engine build 选用 `IFileSystem_HL25`。
Open/Write/Close/Flush/IsOk 均复用该 SDK 接口，不再发布另一套 filesystem 私有槽位表。

ABI 编号：1 为 `sprintf(dst,fmt,...)`，2 为 `snprintf(dst,n,fmt,...)`，3/4 为对应 fortified
变体，5 为 `strncpy`，6 为返回字符串的 `UTIL_va(fmt,...)`，7/8 为 thiscall/cdecl filesystem
Open，9 为 cdecl ReadFile，10 为 EAX=path、EDX=length 指针的 ReadFile，11 为 cdecl
`Cvar_Set(name,value)`。变参格式化均由调用方清栈，必须保留全部原参数；Linux 编号不表示插件已实现对应 adapter。

Windows 已核验容量：CZP 路径与 SaveProfile CFG 命令均为 260，SendMatchCommand 为 1024，
PopulateProfiles 选择命令为 62，legacy/name 字段为 32，序列化缓冲区为 16384。31 字节名字会生成
68 字符 key，必须直接写入经容量检查的最终路径/命令，不能经过旧 32 字节字段。
UI sidecar 需在截断前捕获完整名字，覆盖默认/焦点选择及按钮双击；返回式格式化替身需保证字符串
在同步命令链内存活。

安装任何新存储 hook 前必须完整解析版本契约，核对组数量、ABI、策略、容量及实际调用指令；
缺失/冲突时整体 fail closed。区分原始名、legacy key、canonical key 和枚举真实文件名。
BuildProfileList 保留 native STL 返回，不跨 CRT 接收；CreateBlankProfile 原型为 void，失败通过操作
上下文传播；PopulateProfiles 返回 EAX 中的 int 档案数量。未归因磁盘错误及短写后的原子替换属于后续独立工作。

G26 的 runtime contract v1 仅适用于 Windows hl-10210，消费以下 `gameui` 记录：

| GameSymbol | Kind |
| --- | --- |
| `CareerStorage_runtime_contract_version` (=1) | `scalar` |
| `CCareerNewProfileBox_native_input_capacity` (=64) | `scalar` |
| `TheCareerDefinitions` | `global` |
| `TheCareerGame` | `global` |
| `vgui2::g_pFileSystemInterface` | `global` |
| `CareerDefinitionManager.m_activeCareer` | `structMember` |
| `CareerDefinition.m_saveDir` | `structMember` |
| `CCareerGame.m_writer` | `structMember` |
| `CCareerGame.m_needAutosave` | `structMember` |
| `BufferWriter.m_overflow` | `structMember` |
| `CCareerNewProfileBox.m_pEntry` | `structMember` |
| `vgui2::TextEntry.m_TextStream.m_Size` | `structMember` |
| `CCareerGame::CreateProfileBuffer(CCareerProfileData*, unsigned char*, int, bool)` | `function` |
| `vgui2::TextEntry::GetText(wchar_t*, int)` | `virtualFunction` |

活动 saveDir 是 manager 指针链中的字符串指针，不是内联数组。serializer 返回长度不能证明数据完整；
必须在 `wb Open` 截断目标前核对 writer overflow 标志、完整 CZP 结构及 raw name。
该 binary 的宽 GetText 虚槽 136 按 wchar_t 字符数计容量，虽然外部 SDK 参数名写 bufLenInBytes，
实际应传 `textLength + 1` 个字符。NewProfileBox Ok 在旧 64 字节 apply 缓冲区之前截获；
sidecar 保存完整名字和生命周期句柄，GameUI 仍拥有其原生分配。
原生枚举可能跨搜索路径重复返回同一 basename：按实际 Open 的胜出文件检查，每次 Populate 仅允许一次。
物理读取根与写入根可能分别为 `czero_schinese`、`czero`；同时检查两个根的路径容量和目录错误，
已有文件还需先解析实际本地路径再读取。
