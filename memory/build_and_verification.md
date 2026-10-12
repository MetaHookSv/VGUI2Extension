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
