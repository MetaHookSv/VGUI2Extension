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
Both SDL include arguments are required. The MetaHook SDK can be consumed from a path, or fetched at a fixed commit via FetchContent.
Output goes to build/install and does not touch the local game directory.
