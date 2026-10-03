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

1. 安装最新版本的MetaHook
2. 将 `svencoop/` 的内容合并到目标 mod 目录，例如 `valve/`、`cstrike/` 或 `svencoop/`。
3. 将 `platform/` 合并到游戏的 platform 目录。
4. 在 `<mod>/metahook/configs/plugins.lst` 中添加：

   ```text
   VGUI2Extension.dll
   ```

5. 通过 MetaHook 启动游戏。
