[Back to README](../../README.md) | [中文](../zh-CN/gamedata.md)

# gamedata

This page covers the plugin's runtime catalog requirements and the GameSymbols it uses.
Synchronization and validation commands are in [Build instruction](build-instruction.md#gamedata);
deployment is described in [Installation](installation.md).

## Runtime requirements

Install the plugin's catalog under `<mod>/metahook/gamedata/vgui2extension/` and update it
together with the DLL and resources. MetaHook must merge nested catalogs and provide
API 115 or newer, with an API version at least as new as the SDK used to build the plugin.
The plugin resolves its gamedata-covered private symbols through the host catalog API.

GameUI needs PropertySheet methods and `_activePage`, FocusNavGroup `GetCurrentFocus`
and `_currentFocus`, the three options pages' `OnApplyChanges`, and
`CBasePanel::ApplySchemeSettings`. ServerBrowser needs a matching KeyValues `LoadFromFile`
record. PropertySheet virtual calls use the current object's vtable and a slot queried
from gamedata.

GameUI also requires `vgui2_EditablePanel_OnSizeChanged_call_GetChild_callsite_0`
on every supported Windows GameUI identity. Only this callsite filters popup children
to null, so parent resizing cannot move or clip their screen-space bounds. Other
`GetChild` callers and non-popup children retain their original behavior.
On HL25, the default ContentControlDialog resource width is increased from 328 to 448
before proportional scaling, leaving room for the AddonsFolder checkbox.

Menu handling needs `vgui2::Menu.m_pScroller` and
`vgui2::Menu::MakeItemsVisibleInScrollRange()`. The method is resolved directly and takes
no explicit arguments in the target GameUI binaries. The hook retains dependencies on
the Legacy/HL25 layout of members adjacent to `m_pScroller`; PropertySheet `_pageTabs`
also retains its legacy array layout dependency.

`IInput2::PostKeyMessage` resolves `CInputWin32::PostKeyMessage(KeyValues*)` directly
against `vgui2.dll` through gamedata. Older catalogs lacking required records cannot
resolve these hooks.

## GameSymbols inventory

The GameUI wide-character `ComboBox::AddItem` hook requires the ComboBox/Menu
records published in GoldSrc_VibeSignatures `v20261013a`. It allocates the exact UTF-8
byte count (including the terminator), preserves the wide `SetText` message and user
data, and calls `GetMenu` / four-argument `AddMenuItem` through catalog-resolved
virtual slots. The CS/CZ client now uses the same insertion helper and also hooks
wide `UpdateItem`: both the menu caption and `SetText` message stay Unicode, while
invalid IDs return false and successful updates retain deferred layout invalidation.
The client requires its own `AddItemW`, `UpdateItemW`, `IsItemIDValid`, `GetMenu`,
four-argument Menu `AddMenuItem`, wide Menu `UpdateMenuItem`, and Panel
`InvalidateLayout` records. The selected-player caption also hooks
`CBottomBar::SetPlayerText`, using its client `m_pPlayerList` offset and the native
`TextEntry::SetText(char const*)` slot with a correctly sized UTF-8 buffer.
For Windows CS/CZ 10210 the map-page implementation is inlined into four-argument
`CTeamMenu::Update`; older supported Windows clients use one-argument
`CTeamMenu::LoadMapPage`. The plugin resolves the actual published identity and
uses a matching wrapper. Game flags are initialized before native UI hook lookup.
Do not deploy this DLL with older catalogs that mislabeled Update as LoadMapPage.
Deploy the extended GoldSrc_VibeSignatures catalog with
this plugin; the new client dependency records are not in `v20261016b`.

The Condition Zero career bot-cost fix requires the four records added by
[GoldSrc_VibeSignatures #378](https://github.com/HLND2T/GoldSrc_VibeSignatures/pull/378).
On `czero`, it calls the original `CBotButton::SetCharacter`, then replaces only
the cost text using the complete integer and `#Career_Cost`. The TextImage member
offset and wide SetText virtual slot come from gamedata; the native overload has
no extra `bool` argument. Other games do not install this hook.

The Condition Zero career player-name fix requires the five records published in
GoldSrc_VibeSignatures `v20261016b`. It hooks the difficulty-box constructor and
base-frame Activate, calls the originals first, then explicitly converts the full
UTF-8 name cvar to Unicode and updates the Label through its catalog-resolved wide
virtual slot. The frame retains the localized difficulty from the public
`CareerUI001` interface. Member offsets come from gamedata, including the inherited
`CCareerBaseBox.m_pTextLabel`; neither CRT locale nor a 31-byte temporary limits the
replacement name. Invalid UTF-8 or missing localization leaves the original text.
These hooks are installed only on `czero`.

The Condition Zero buy-preset popup-order fix requires the three client records
published in GoldSrc_VibeSignatures `v20261020a`: `CounterStrikeViewport::OnTick()`,
static `CCareerBaseBox::BringToFront()` and the `CCareerBaseBox::s_currentBox`
pointer-storage global. During the original Tick only, defer repeated activation
when the current visible box is already the app-modal surface and has a visible
descendant popup. This keeps the dropdown above its owner without changing initial
activation, other callers or modal setup. Closing the popup resumes native behavior.
Only `czero` installs these hooks; CS and CZDS do not publish these client records.

The declaration is [scripts/manifests/vgui2extension.json](../../scripts/manifests/vgui2extension.json).
The list below follows its exact module, name and kind and has been checked against the
source calls. It contains **130 explicit records** across 5 modules, including the
initial `_0` records of **7 numbered patch sets**. Each numbered set can consume
additional records; 130 is not the total size of every installed catalog.

| Module | Explicit entries |
| --- | ---: |
| `engine` | 14 |
| `client` | 33 |
| `gameui` | 73 |
| `serverbrowser` | 9 |
| `vgui2` | 1 |
| Total | 130 |

A GameSymbol is identified by its module and name. Identical names in `engine`, `client`,
`gameui` and `serverbrowser` refer to separate records. Conditions below describe the
manifest's catalog pruning and validation scope; the source can make optional or
conditional requests within that scope. Some games share engine or UI binaries, so
these groups must not be interpreted as runtime mod-directory checks.

### Symbol kinds

| Kind | Use in this plugin |
| --- | --- |
| `function` | Resolve a function entry |
| `global` | Resolve a global variable's address |
| `virtualFunction` | Resolve a virtual method entry or query its owning vtable slot |
| `vtable` | Resolve a module-owned virtual table for public-interface hooks |
| `structMember` | Query a member's byte offset from the object base |
| `patch` | Resolve an instruction or branch site to redirect or patch |

The wrappers are in [plugins.h](../../src/plugins.h). Required lookup failures report an
error; optional lookups can return null. Querying `virtualFunction` slot indices uses
API 115's `mh_gamesymbol_t::vfuncIndex`.

### Manifest conditions

`All` is the manifest's top-level `gameVersions` list. `G1` through `G24` identify the
current `conditionalGroups` entries in order. The `cl_time` exemption is explained under
[Runtime conditions and optional records](#runtime-conditions-and-optional-records).

| Condition | gameVersions in the manifest |
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
| G19 | `cof-5936` |
| G20 | `cstrike-3248`, `cstrike-3647`, `cstrike-4554`, `cstrike-6153`, `cstrike-8684`, `czero-8684`, `czeror-8684` |
| G21 | `cstrike-10210`, `czero-10210`, `czeror-10210` |
| G22 | `hl-8684`, `hl-10210`, `svencoop-8948`, `svencoop-10257` |
| G23 | `hl-10210` |
| G24 | `czero-10210`, `czero-8684` |

## Symbols by module

### engine

Sources: [privatefuncs.cpp](../../src/privatefuncs.cpp),
[EngineSurfaceHook.cpp](../../src/EngineSurfaceHook.cpp) and [exportfuncs.cpp](../../src/exportfuncs.cpp).

| GameSymbol | Kind | Manifest condition |
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

Sources: [ClientVGUI.cpp](../../src/ClientVGUI.cpp) and [privatefuncs.cpp](../../src/privatefuncs.cpp).

| GameSymbol | Kind | Manifest condition |
| --- | --- | --- |
| `g_iVisibleMouse` | `global` | G5 |
| `CounterStrikeViewport::OnTick()` | `virtualFunction` | G24 |
| `CCareerBaseBox::BringToFront()` | `function` | G24 |
| `CCareerBaseBox::s_currentBox` | `global` | G24 |
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

Source: [GameUI.cpp](../../src/GameUI.cpp).

| GameSymbol | Kind | Manifest condition |
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

### serverbrowser

Source: the ServerBrowser handlers in [GameUI.cpp](../../src/GameUI.cpp).

[ServerBrowserHistory.cpp](../../src/ServerBrowserHistory.cpp) redirects the date
`strftime` callsites and hooks the module's KeyValues `SetString` public ABI slot.
It captures a wide date using an explicit default LC_TIME locale, preserves native
narrow formatting/lowercasing, and substitutes `SetWString` only for a matching
`LastPlayed` buffer. It does not hook either owner function. The date patches require
GoldSrc_VibeSignatures v20261017a or newer; older filter loops have no date callsite.
For `schinese` and `tchinese`, dates use `YYYY-MM-DD HH:MM` (24-hour time);
other game languages retain the existing localized date format.

| GameSymbol | Kind | Manifest condition |
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

Source: [InputWin32.cpp](../../src/InputWin32.cpp).

| GameSymbol | Kind | Manifest condition |
| --- | --- | --- |
| `CInputWin32::PostKeyMessage(KeyValues*)` | `function` | G2 |

## Numbered patches

[PatchPanelSizeCallsites in GameUI.cpp](../../src/GameUI.cpp) constructs the following
names with `n = 0, 1, 2, ...`. The first record is required; later records are queried
optionally, stopping at the first missing number. The `SetSize` / `SetMinimumSize`
sets run on non-HL25 engines, including SvEngine; the `hl-10210` HL25 engine publishes
only the `SetBounds` set (the two other sets appear as the `_ScaledConst` variants
upstream), so it carries its own G18 condition. `hl-10210` is therefore a distinct
group and must not be folded into G16, which requires all three prefixes.

| Module | Numbered GameSymbol pattern | Kind | Manifest condition |
| --- | --- | --- | --- |
| `gameui` | `vgui2_Panel_SetSize_Const_callsite_<n>` | `patch` | G16 |
| `gameui` | `vgui2_Panel_SetMinimumSize_Const_callsite_<n>` | `patch` | G16 |
| `gameui` | `vgui2_Panel_SetBounds_Const_callsite_<n>` | `patch` | G16, G18 |
| `serverbrowser` | `vgui2_Panel_SetSize_Const_callsite_<n>` | `patch` | G17 |
| `serverbrowser` | `vgui2_Panel_SetMinimumSize_Const_callsite_<n>` | `patch` | G17 |
| `engine` | `FileSystem_SetGameDirectory_V_strncpy_callsite_<n>` | `patch` | G19 |
| `engine` | `FileSystem_AddFallbackGameDir_V_strncpy_callsite_<n>` | `patch` | G19 |

The manifest lists each initial `_0` record explicitly and uses `numberedPatchSets` to
retain the remaining consecutive records. Its `prefix` omits the final underscore;
source prefixes include it before appending the number. Keep module and version
conditions when updating the catalog.

G18 retains the published `SetBounds` callsites on `hl-10210`, but the runtime does
not redirect them: HL25 already scales its dimensions, and dialog resources overwrite
constructor bounds. ContentControlDialog is widened in its resource before scaling;
the separate `OnSizeChanged` patch prevents subsequent parent layout from clipping it.

The engine's two language patches are numbered as well. Most engines merge the Steam-language
and default-English arms into a single `call`, so each `_0` covers its whole owner; CoF emits one
copy per arm and additionally publishes `_1`. G19 keeps that set for `cof-5936` only, while the
other engine group (G4) declares just the two required `_0` records.

## Runtime conditions and optional records

- HL25's default server-name hook requires the new `LoadGameOptionsList` record.
  After the original creates its controls, it finds `hostname` / `DescEdit` through
  the VGUI2 tree and sends a UTF-8 `SetText` message. It preserves the Steam persona
  name and truncates at a complete UTF-8 boundary to fit the native 63-byte payload,
  including `"'s Server"`. Missing Steam API/name or invalid UTF-8 preserves native
  text. User editing and the launch/apply paths are unchanged. Ship the new catalog
  with the DLL; `v20261017a` does not contain this record.
- `engine / cl_time` is exempted by the manifest on the ten CS/CZ/CZDS versions in G9.
  This is a catalog validation exemption. The source still performs a required lookup;
  the exemption does not make the runtime symbol optional.
- `engine / VGUIClient001_CreateInterface` is queried first. When that record is absent,
  the legacy path requires `engine / g_pClientFactory`; other query failures report an error.
- `engine / Sys_GetRegKeyValueUnderRoot` is queried optionally. When unavailable, both
  filesystem `V_strncpy_callsite_0` patches are required; the `_1` duplicates are
  queried optionally and exist only on `cof-5936`. `CGame::WindowProc` is used
  when the SDL2 `SDL_GetWindowWMInfo` path is unavailable.
- `client / g_iVisibleMouse` is queried optionally for clients without native VGUI2.
  Native client `Panel::Init` and KeyValues `LoadFromFile` are also queried optionally.
- Each KeyValues lookup tries the `vgui2::KeyValues::LoadFromFile(...)` name before
  `KeyValues::LoadFromFile(...)`. GameUI and ServerBrowser require one matching record
  in their respective module; the client lookup remains optional.
- The Career constructors are requested for Condition Zero and CZDS. Their manifest
  records are under HL/COF GameUI catalog identities because the UI binary is shared;
  a mod directory name alone does not determine the catalog group.
- `COptionsSubVideo::ApplyVidSettings(bool)` is optional and is inlined into
  `OnApplyChanges()` in `hl-10210`. `RichText::InsertChar(wchar_t)` is queried optionally;
  when absent, `RichText::InsertString(wchar_t const*)` is required.
- CS/CZ background panel members and `Activate()` exclude CZDS. The world map records
  apply to CZDS; common CS-family Frame and menu records include all three variants.
- FocusNavGroup, options-page and CBasePanel callback hooks resolve some records when
  their hooks are first installed. The inventory describes all possible consumption,
  rather than a list queried in full at startup.

Update this inventory with the manifest when symbol names, kinds, modules, version
conditions or numbered patch sets change.
