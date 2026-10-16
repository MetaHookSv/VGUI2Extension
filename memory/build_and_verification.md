---
title: build_and_verification
type: note
permalink: vgui2extension/build-and-verification
---

# Build and verification

## Build structure

The root CMake uses MSVC x86, C++20, Debug/Release, a static CRT and VC-LTL 5.3.1.
The compile source list comes from the 129 ClCompile items of the original vcxproj, of which 22 are plugin units and 107 are shared SDK units.
Capstone/GLEW are leftovers from the original MSBuild prerequisite steps only; the current plugin does not need them.
Both SDL include arguments are required. The MetaHook SDK can be consumed from a path, or fetched from the latest `main` via FetchContent.
Output goes to build/install and does not touch the local game directory.

## Issue 924: Condition Zero mission-pack menu

- Trigger: open `ComboBoxMissionPack` with enough mission packs to overlap the description or extend below the screen.
- Root causes: `CareerScheme.res` uses the literal color key `Menu/BgColor`, which path-based KeyValues lookup misses; the native career frame sets the menu's visible-line count to the total pack count. Native menu layout also disables its scrollbar.
- Fix: resolve literal color symbols before retaining path/alias fallbacks. Hook the native menu's `PerformLayout` slot through the existing PropertySheet catalog entry (the shared Panel slot), limit rows to the space below the combo using the current scaled row height and border insets, and enable the native scrollbar after layout. The layout change is restricted to the mission-pack combo; the hook is removed on shutdown.
- Validation: MSVC x86 Release build, 21-snapshot gamedata validation, and the component's pinned format check passed. Real czero build 10210 runs covered 36 packs at 1920x1080 (menu bottom 1053) and 1280x720 (bottom 704), dragging to the last pack and selecting it, a single pack without a scrollbar, and opening the unrelated `MapList` dropdown.
- GPU validation: RenderDoc replay at 1920x1080 confirmed the menu background pixel (1100,400) is RGBA `(60,60,60,255)`, replacing the underlying mission-pack image. The captured menu included the last pack after scrolling.
- Reproduction uses temporary `MissionPacks/CodexIssue924TestNN/Overview.vdf` directories; remove only those fixtures after exiting the test game. Back up the deployed DLL/PDB before testing and restore them afterward. Do not change the original mission-pack or scheme resources.
- Limits: other engine builds and custom career-frame layouts have not been tested in-game. These are live UI regression checks, not unit-test or cross-engine compatibility claims.

## Issue 9: Unicode ComboBox menu captions

- Root cause: native GameUI wide `ComboBox::AddItem` supplies `wcslen(text) + 1` bytes to a UTF-8 conversion. The six UTF-16 code units in `  零点行动` require 15 UTF-8 bytes including NUL, not 7; conversion failure leaves truncated text and residual bytes in the menu caption. The separate wide `SetText` message explains why the selected caption was unaffected.
- Fix: inline-hook the GameUI wide overload, query and allocate the exact UTF-8 byte count, and reject conversion failure before constructing a message. Preserve `SetText` as WSTRING and forward the original target/userData and returned item ID through the native menu. Resolve `GetMenu` and four-argument `AddMenuItem` virtual slots from gamedata, with both KeyValues namespace spellings; do not read the SDK ComboBox layout.
- Catalog: GoldSrc_VibeSignatures v20261013a; manifest includes the three methods (five records including aliases), pruned and validated across all 21 configured snapshots. This does not establish live compatibility across those builds. Client-side AddItem is outside this fix.
- Verification (2026-10-13): MSVC x86 Release build, 3 CTest tests, 21-snapshot catalog validation and pinned format check passed. Conversion regression cases cover the original Chinese caption, ASCII, empty text, 512 Chinese characters, a surrogate pair and rejected null input.
- Follow-up: conversion output now uses `std::string`; reserve the conversion's NUL within the writable size, then resize to exclude it from the logical length and pass `c_str()` to the menu. Release build, all 3 tests, catalog validation and format check passed again. This container-only change was not rerun in-game.
- Live verification: czero build 10210 at 1920x1080, original single mission pack. halflife-mcp reported MenuItem name `  零点行动`; selecting it closed the dropdown normally. The unrelated MapList opened with ASCII map entries and accepted selection. These are UI structure/input observations, not a screenshot or semantic text readback. The task-launched game exited with code 0; the original DLL/PDB/catalog were restored and hashes checked. No mission-pack fixtures were needed.

## Issue 925: Condition Zero career bot costs

- Trigger: set a friendly career bot's Cost to two or more decimal digits; native CBotButton::SetCharacter displays only GetPoints() % 10.
- Root constraint: formatting discards the high digits before TextImage receives the string. hl-10210 Windows/Linux confirm this; widening the control alone cannot repair it.
- Fix: czero-only inline hook calls the original first, preserves null-character clearing, then formats the complete integer into the existing #Career_Cost localization and updates m_pText[0]. Resolve SetCharacter, GetPoints() const, the m_pText member offset and TextImage::SetText(wchar_t const*) slot through GoldSrc_VibeSignatures PR #378 records. The native wide overload has no SDK-only bool argument; use its actual ABI. Keep the native deferred layout invalidation.
- Validation (2026-10-16): MSVC x86 Release build, 21-snapshot manifest validation, existing 3 CTest regression tests and pinned format check passed. Real czero build 10210 at 1280x720 using halflife-mcp snapshot showed pool costs 9/12/100/123, hired costs 123/12, and remaining points 865 = 1000 - 123 - 12. Dismissing the 123-point bot cleared the vacant slot and restored 988 points; hiring the 100-point bot displayed cost 100 and left 888 points. Game exited with code 0.
- Test cleanup: original DLL/PDB, plugin catalog, BotCampaignProfile.db, CareerGameEasy.vdf and pre-existing save/config files restored with byte comparisons. Temporary profile and PNG/TGA evidence retained under C:/Users/HZDEV/AppData/Local/Temp/issue925-runtime-backup (HalfLife84 through HalfLife87 show the UI checks).
- Scope/limits: only Windows czero 10210 was tested live. Four-digit initial remaining points (1000) still ellipsize in the separate LabelPoints control; that layout issue is outside the per-bot cost fix. No new test duplicates std::to_wstring; native ABI/layout and hire/dismiss behavior were checked in-game.
