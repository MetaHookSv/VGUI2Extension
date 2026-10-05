[返回 README](../../README.zh-CN.md) | [English](../en/ci-cd.md)

# 自动化构建

- [LiveBuild](../../.github/workflows/livebuild.yml) 在推送到 `main`、面向 `main` 的拉取请求
  以及手动运行时构建 Windows x86 Release，产物为 `VGUI2Extension-windows-x86.7z`。
- [Build](../../.github/workflows/msbuild.yml) 在推送 `v*` 标签时运行，创建包含
  `VGUI2Extension-windows-x86.7z` 的 `VGUI2Extension-<tag>` GitHub Release。

两个工作流共用 [build-windows-x86 action](../../.github/actions/build-windows-x86/action.yml)。
它从 `main` 检出同级 MetaHook 源码树，仅初始化 SDL2 和 SDL3 头文件依赖。
该显式 SDK 路径优先于本地未指定 `METAHOOK_SOURCE_PATH` 时使用的 FetchContent 最新 `main`。

action 使用 Release 构建脚本启用回归测试，运行 Python unittest 和 CTest，
并按插件 manifest 校验安装后的 gamedata。随后使用 7-Zip 打包安装树中的 `svencoop/` 与
`platform/`，上传前检查归档完整性。运行包包含 DLL、PDB、资源和 gamedata；公共接口头通过安装树提供。

以上描述依据工作流定义。迁移验证记录中的本地打包结果不代表远程工作流已成功运行。

本地构建、依赖与验证命令见[构建说明](build-instruction.md)。
