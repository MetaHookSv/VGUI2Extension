---
title: build_and_verification
type: note
permalink: vgui2extension/build-and-verification
---

# Build and verification

## 构建结构

根 CMake 使用 MSVC x86、C++20、Debug/Release、静态 CRT 和 VC-LTL 5.3.1。
编译源列表来自原 vcxproj 的 129 个 ClCompile 项，其中 22 个为插件单元，107 个为共享 SDK 单元。
Capstone/GLEW 仅为原 MSBuild 前置步骤残留；当前插件不需要它们。
两个 SDL include 参数均必填。MetaHook SDK 可从路径消费，也可通过固定提交 FetchContent 获取。
输出到 build/install，不访问本机游戏目录。