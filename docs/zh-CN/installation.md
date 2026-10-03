[返回 README](../../README.zh-CN.md) | [English](../en/installation.md)

# 安装说明

本页介绍安装目录布局、插件加载顺序与公共接口头文件。
从源码构建见[构建说明](build-instruction.md)；运行要求与启动参数见[功能说明](features.md)。

## 安装目录布局

`install/x86/<Debug|Release>/`：

```text
svencoop/
  metahook/plugins/VGUI2Extension.dll
  metahook/plugins/VGUI2Extension.pdb
  metahook/gamedata/vgui2extension/
  vgui2ext/resource/
  vgui2ext/resource_hl25/
platform/
  resource/vgui_schinese.txt
  servers/serverbrowser_schinese.txt
include/Interface/
  IVGUI2Extension.h
  IDpiManager.h
  VGUI/IInput2.h
  VGUI/IScheme2.h
  VGUI/ISurface2.h
```

运行包 `VGUI2Extension-windows-x86.7z` 包含 `svencoop/` 和 `platform/`。
公共接口头随安装提供给开发者，不包含在运行包中。

## 启用插件

1. 安装提供 API 115 或更新版本、支持合并嵌套 gamedata catalog 的 MetaHook。
   其 API 版本还须不低于编译插件时使用的 SDK 版本。
2. 将 `svencoop/` 的内容合并到目标 mod 目录，例如 `valve/`、`cstrike/` 或 `svencoop/`。
3. 将 `platform/` 合并到游戏的 platform 目录。
4. 在 `<mod>/metahook/configs/plugins.lst` 中添加：

   ```text
   VGUI2Extension.dll
   ```

5. 将它放在依赖它的 CaptionMod、BulletPhysics、SCModelDownloader 等插件之前，
   并通过 MetaHook 启动游戏。

更新时一并更新 DLL、资源与 `metahook/gamedata/vgui2extension/`。
旧 catalog 可能缺少插件所需的私有符号，详见 [gamedata 要求](gamedata.md#运行要求)。

## 公共接口

公共接口以本仓库的 `include/Interface/` 和 `include/Interface/VGUI/` 为准。
使用接口的工程应将这两个 include 目录放在 MetaHook 历史副本之前；其他 SDK 头仍由 MetaHook 提供。

| 接口 | 头文件 |
| --- | --- |
| VGUI2 回调 | [IVGUI2Extension.h](../../include/Interface/IVGUI2Extension.h) |
| DPI 管理 | [IDpiManager.h](../../include/Interface/IDpiManager.h) |
| 扩展输入 | [IInput2.h](../../include/Interface/VGUI/IInput2.h) |
| 扩展 scheme | [IScheme2.h](../../include/Interface/VGUI/IScheme2.h) |
| 扩展 surface 与字体 | [ISurface2.h](../../include/Interface/VGUI/ISurface2.h) |

独立工程保留原有接口版本和插件导出约定。
