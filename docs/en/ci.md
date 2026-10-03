[Back to README](../../README.md) | [中文](../zh-CN/ci.md)

# Automated builds

- [LiveBuild](../../.github/workflows/livebuild.yml) builds Windows x86 Release on pushes
  to `main`, pull requests targeting `main`, and manual runs. Its artifact is
  `VGUI2Extension-windows-x86.7z`.
- [Build](../../.github/workflows/msbuild.yml) runs when a `v*` tag is pushed and creates
  a `VGUI2Extension-<tag>` GitHub Release containing `VGUI2Extension-windows-x86.7z`.

Both workflows use the shared
[build-windows-x86 action](../../.github/actions/build-windows-x86/action.yml). It checks
out a sibling MetaHook source tree from `main` and initializes only its SDL2 and SDL3
header dependencies. This explicit SDK path takes precedence over the pinned FetchContent
SDK used by local builds without `METAHOOK_SOURCE_PATH`.

The action runs the Release build script with regression tests enabled, runs Python
unittest and CTest, and validates the installed gamedata against the plugin manifest.
It packages the installed `svencoop/` and `platform/` directories with 7-Zip and checks
archive integrity before upload. DLL, PDB, resources and gamedata are included; public
interface headers are available through the install tree.

These descriptions reflect the workflow definitions. The migration verification record
documents local packaging; it does not establish a successful remote workflow run.

For local build, dependency and verification commands, see [Build instruction](build-instruction.md).
