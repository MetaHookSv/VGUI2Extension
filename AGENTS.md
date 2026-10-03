# AGENTS.md

This file provides guidance and important rules working with code in this repository.

## When coding / building plan

- Use a progressive disclosure approach for agent coding in this repository: start from high-level information in the Basic Memory knowledge base first, and only locate/read specific files or symbols when necessary, instead of expanding a large amount of context at once.

#### Basic Memory knowledge base (project-scoped, `memory/`)

- Notes live in `memory/` (markdown with YAML frontmatter: `title`/`type`/`permalink`), tracked in git.
- This repository contains the standalone VGUI2Extension plugin, extracted from MetaHookSv. Its notes were migrated from MetaHookSv and adapted to the CMake workspace; see `memory/project_overview.md` for scope and provenance.
- Basic Memory is registered as MCP server `basic-memory`, pinned to the `vgui2extension` project (project-level `.mcp.json`, mirrored by `.codex/config.toml`). The `metahooksv` project belongs to the source repository.
- Prefer Basic Memory MCP tools (`search_notes` / `read_note` / `write_note` / `edit_note`) only when their project resolves to this repository's `memory/` directory. Verify the project binding before writing; when no matching project is available, read and edit the local markdown files directly.
- Notes use the `vgui2extension/` permalink prefix to distinguish them from the source repository.
- Current code takes precedence over stale notes: this checkout still registers `VGUI_Input2_005` with `CancelIMEComposition`, while older MetaHookSv notes claim IInput2 was bumped to 006. Do not copy that conclusion.

#### High-level information in this repository (read corresponding notes first)

- Project overview and codebase entry points: `project_overview`
- Build commands, dependency pinning, gamedata sync, verification status: `build_and_verification`

#### When notes are insufficient: source entry points (query and read on demand)

- Build: `CMakeLists.txt`, `cmake/Sources.cmake` (explicit compile list), `cmake/Dependencies.cmake`, `cmake/VCLTL.cmake`, `scripts/build-VGUI2Extension-x86-{Debug,Release}.bat`
- Plugin sources: `src/`; lifecycle entry `src/plugins.cpp`, callback dispatch `src/VGUI2ExtensionInternal.cpp`
- Public API / interfaces: `include/Interface/` (`IVGUI2Extension.h`, `IDpiManager.h`, `VGUI/` for `IInput2.h`, `IScheme2.h`, `ISurface2.h`). These five public interface headers take precedence over MetaHook's historical copies and are installed by this repository.
- Assets and localization: `assets/` (`svencoop/vgui2ext/`, `platform/`), installed to the prefix root
- gamedata: `scripts/manifests/vgui2extension.json` (static consumption contract), `scripts/sync-gamedata.py`, `scripts/validate-gamedata.py`
- Tests: C++ regression tests under `tests/` run by CTest; gamedata synchronizer behavior tests under `scripts/tests/` run by Python unittest
- Docs: `README.md` / `README.zh-CN.md`, prose pages under `docs/en/` and `docs/zh-CN/`
- MetaHook SDK is consumed, not built: taken from `METAHOOK_SOURCE_PATH`, or fetched at a fixed commit when the path is unset. Both SDL2 and SDL3 include arguments are required; SDL is not built or packaged here.
- Build output: `build/x86/<configuration>/`; install output: `install/x86/<configuration>/`. Neither is tracked, and nothing is deployed to the game automatically.

#### Progressive disclosure key points

- Read notes first, then locate a single file/symbol; do not read the whole repository at once.
- Prefer correctly scoped Basic Memory MCP tools for knowledge retrieval; otherwise use the local notes before reading source.
- Prefer Context7 for external dependency/library usage (query on demand).

## Repository rules

- Preserve the MetaHook API, plugin exports, interface versions and original behavior. Keep `cmake/Sources.cmake` as the explicit compile list.
- Resolve private symbols through the host gamedata API; do not add scan fallbacks for symbols that already exist, and do not judge coverage across modules by symbol name alone.
- When gamedata usage changes, update `scripts/manifests/vgui2extension.json` in the same change, including module, Windows-version conditions and consecutive numbered patches.
- Match the naming, indentation and comment style of the original files in `src/`.
- Do not modify the external SDK or third-party sources. VC-LTL comes from a hash-verified binary cache; this repository has no third-party submodules, only the `thirdparty/cache` directory.
- Regression tests compile with assertions enabled even in Release (`/UNDEBUG`); configuration and documentation text are not assertion targets.
- Verification distinguishes compile, simulated tests and an actual game run. Claims about IME behavior or per-engine compatibility must not be made without real in-game verification.

## Explore SKILLs

- Project-level skills, when present, live in `.claude/skills` no matter what harness tool is being used.
