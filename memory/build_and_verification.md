---
title: build_and_verification
type: note
permalink: vgui2extension/build-and-verification
---

# Build and verification

## 迁移范围与来源

- 源仓库：MetaHookSv `fe80b6d60bfb487b52aed7ea7ec0492e7b27a5d2`。
- 参考 Renderer：`a23e1c55d79fb2b87312ac92717e1f841f4ea55b`，沿用构建脚本、VC-LTL 准备、manifest 同步与校验、CI 打包结构。
- MetaHook SDK：固定 `1d23fe946e6f0f09a1a892aa2156c3b462774026`；本地验证使用 `D:/MetaHook`。
- 原仓库保持不变；目标仓库原有 main 分支尚无提交，origin 已指向 `https://github.com/MetaHookSv/VGUI2Extension`。未自动提交或推送。
- 不迁入机器专属 vcxproj.user、旧 vcxproj/filters、旧测试 runner、源仓库输出和第三方预编译产物。
- 原测试 runner 的职责改由 CTest 承担；宿主 IME 消息测试迁入 `src/tests/`，仅修改首行 include 相对路径。

## 构建结构

根 CMake 使用 MSVC x86、C++20、Debug/Release、静态 CRT 和 VC-LTL 5.3.1。
编译源列表来自原 vcxproj 的 129 个 ClCompile 项，其中 22 个为插件单元，107 个为共享 SDK 单元。
Capstone/GLEW 仅为原 MSBuild 前置步骤残留；当前插件不需要它们。
两个 SDL include 参数均必填。MetaHook SDK 可从路径消费，也可通过固定提交 FetchContent 获取。
输出到 build/install，不访问本机游戏目录。

## 本地验证（2026-10-03，Asia/Singapore）

环境：VS 2022 Community、MSVC 19.44.35228、Windows SDK 10.0.26100.0、CMake 3.31.12、Python 3.12.5。

- x86 Release configure/build/install：退出 0，日志 `build/release.log`。
- x86 Debug build 脚本 configure/build/install：退出 0，日志 `build/debug.log`。
- CTest：Debug 2/2，Release 2/2；语言注册表和 IME 消息测试均通过。
- Python 条件编号补丁裁剪测试：1/1，通过前先在原同步器上得到预期失败。
- Debug/Release 安装后的 catalog：各 21 个快照，manifest 校验退出 0。
- 默认 FetchContent 路径配置退出 0，实际获取 SHA 与固定提交一致；没有通过该目录再次编译。
- 无效 SDL include 路径配置退出 1，明确报告错误。
- 原插件 43 个源码/头/测试文件、5 个公共接口、8 个资源文件逐字节一致。
- 使用中的 107 个 MetaHook SDK 编译单元与源仓库逐字节一致。
- 两种生成工程各有 129 个编译单元，均未引用 `D:/MetaHookSv`。
- 两种安装 DLL 均为 PE x86。Release 导出 CreateInterface、SDL_GL_GetCurrentWindow、SDL_GetWindowFromID；导入为 Windows 系统 DLL 和 msvcrt，无 SDL/Capstone import DLL。
- 本地创建并通过 `7z t` 检查的运行包：`build/artifacts/VGUI2Extension-windows-x86.7z`，32 个文件，包括 DLL/PDB、8 个资源和 22 个 catalog 文件（含 index）。

## gamedata 条件编号补丁经验

- 触发信号：非 HL25 的 UI 尺寸补丁需要 gameui/serverbrowser 内连续 callsite；原 Renderer 同步器只把条件组的编号补丁归入 engine。
- 根因：条件组 parser 只接受字符串 prefix，保留集合生成又硬编码 engine；顶层编号补丁已经支持 module/prefix。
- 正确做法：条件组复用已有 `_parse_numbered_sets`，保存并使用 module/prefix 二元组。原字符串形式仍解释为 engine。
- 验证：合成记录测试覆盖两个模块、连续编号、编号缺口及版本条件；真实 21 快照同步和安装后校验。
- 适用范围：需要条件化保留非 engine 模块编号补丁的独立插件。

初次整理 manifest 时发现快照同时含 Windows/Linux 记录。按 Windows 平台筛选并去重后，
HL25 的 ApplyVidSettings 条件与插件的实际可用性一致。最终 manifest 是静态消费契约，
正常构建不会从源码或当前 catalog 重新生成它。

## 尚未验证

未运行真实游戏，因此插件加载、各引擎 UI/HiDPI、真实 IME 输入行为仍需运行时验证。
GitHub Actions 文件已迁入，但未提交、未触发远程运行。此处本地打包验证不等同于远程 CI 成功。
