---
title: project_overview
type: note
permalink: vgui2extension/project-overview
---

# VGUI2Extension

VGUI2Extension is a UI extension plugin for MetaHook. It gives other plugins the ability to modify GoldSrc VGUI2 components, and provides font and HiDPI support, game language overrides, and input method handling.

## Responsibilities and entry points

- `src/plugins.cpp`: MetaHook IPluginsV4 lifecycle; installs hooks when the engine and client are loaded.
- `src/VGUI2ExtensionInternal.cpp`: VGUI2 callback registration and altitude-sorted pre/post call dispatch.
- `src/BaseUI.cpp`, `GameUI.cpp`, `ClientVGUI.cpp`: takeover of the engine UI, GameUI/ServerBrowser and client UI.
- `src/Surface2.cpp`, `Scheme2.cpp`, `Font*.cpp`: surface, scheme and font proxies.
- `src/DpiManagerInternal.cpp`: HiDPI and SKIN search paths.
- `src/InputWin32.cpp`, `exportfuncs.cpp`, `IMEWindowMessage.h`: Win32/SDL input method path and message ownership.
- `src/LanguageRegistry.h`: legacy engine Steam language registry override logic.

## Dependencies and public interfaces

MetaHook provides API 115, the shared HLSDK/SourceSDK/VGUI sources, and runtime gamedata resolution.
SDL2/SDL3 only consume external headers; runtime interfaces keep their original acquisition method.
The public interfaces are provided by this repository: IVGUI2Extension, IDpiManager, ISurface2, IScheme2, IInput2.
CaptionMod, BulletPhysics, Renderer and SCModelDownloader are the main consumers.

## Build and data flow

`scripts/build-VGUI2Extension-x86-{Debug,Release}.bat` → CMake → compile the DLL → install.
`scripts/manifests/vgui2extension.json` → sync/prune/validate → standalone catalog → host recursive merge.
The compile explicitly keeps 129 units; shared SDK files come from the external MetaHook.
Assets come from the two Chinese localization files originally at `Build/svencoop/vgui2ext/` and `Build/platform/`.
Tests cover the language registry, IME messages, and pruning of conditionally numbered patches.

## External documentation

`README.md` is the English landing page and `README.zh-CN.md` the Chinese one; the structure aligns with the standalone Renderer and MetaHook.
Detailed docs are paginated by build, install, features, gamedata and CI, in bilingual form under `docs/en/` and `docs/zh-CN/`,
with relative links at the top of each page back to the corresponding README and for language switching. The READMEs link uniformly to the bilingual topic pages.
The gamedata page centralizes runtime requirements and the symbol list grouped by module/kind/version condition, containing 92 explicit records
and 5 groups of consecutively numbered patches; manifest publication conditions and source runtime conditions are documented separately — a listing's exemption must not be treated as runtime optionality.

For more information see `README.md`, `docs/zh-CN/features.md`, `memory/build_and_verification.md`.
