# AGENTS.md

This file provides guidance and important rules working with code in this repository.

## When coding / building plan

- Use a progressive disclosure approach for agent coding in this repository: start from high-level information in the Basic Memory knowledge base first, and only locate/read specific files or symbols when necessary, instead of expanding a large amount of context at once.

### Basic Memory knowledge base (project-scoped, `memory/`)

- Notes live in `memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git.
- Notes use the `vgui2extension/` permalink prefix to distinguish them from the source repository.

### High-level information in this repository (read corresponding notes first)

- Project overview and codebase entry points: `project_overview`
- Build commands, dependency pinning, gamedata sync, verification status: `build_and_verification`

### When notes are insufficient: source entry points (query and read on demand)

- Build: `CMakeLists.txt`, `cmake/Sources.cmake` (explicit compile list), `cmake/Dependencies.cmake`, `cmake/VCLTL.cmake`, `scripts/build-VGUI2Extension-x86-{Debug,Release}.bat`
- Plugin sources: `src/`; lifecycle entry `src/plugins.cpp`, callback dispatch `src/VGUI2ExtensionInternal.cpp`
- Public API / interfaces: `include/Interface/` (`IVGUI2Extension.h`, `IDpiManager.h`, `VGUI/` for `IInput2.h`, `IScheme2.h`, `ISurface2.h`). These five public interface headers take precedence over MetaHook's historical copies and are installed by this repository.
- Assets and localization: `assets/` (`svencoop/vgui2ext/`, `platform/`), installed to the prefix root
- gamedata: `scripts/manifests/vgui2extension.json` (static consumption contract), `scripts/sync-gamedata.py`, `scripts/validate-gamedata.py`
- Tests: C++ regression tests under `tests/` run by CTest; gamedata synchronizer behavior tests under `scripts/tests/` run by Python unittest
- Docs: `README.md` / `README.zh-CN.md`, prose pages under `docs/en/` and `docs/zh-CN/`
- MetaHook SDK is consumed, not built: taken from `METAHOOK_SOURCE_PATH`, or fetched from the latest `main` when the path is unset. Both SDL2 and SDL3 include arguments are required; SDL is not built or packaged here.
- Build output: `build/x86/<configuration>/`; install output: `install/x86/<configuration>/`. Neither is tracked, and nothing is deployed to the game automatically.

## Repository rules

- Preserve the MetaHook API, plugin exports, interface versions and original behavior. Keep `cmake/Sources.cmake` as the explicit compile list.
- Resolve private symbols through the host gamedata API; do not add scan fallbacks for symbols that already exist, and do not judge coverage across modules by symbol name alone.
- When gamedata usage changes, update `scripts/manifests/vgui2extension.json` in the same change, including module, Windows-version conditions and consecutive numbered patches.
- Match the naming, indentation and comment style of the original files in `src/`.
- Do not modify the external SDK or third-party sources. VC-LTL comes from a hash-verified binary cache; this repository has no third-party submodules, only the `thirdparty/cache` directory.
- Regression tests compile with assertions enabled even in Release (`/UNDEBUG`); configuration and documentation text are not assertion targets.
- Verification distinguishes compile, simulated tests and an actual game run. Claims about IME behavior or per-engine compatibility must not be made without real in-game verification.
