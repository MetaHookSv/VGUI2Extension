---
title: HL25 引擎只发布部分编号 callsite 组时的 manifest 处理
type: note
permalink: vgui2extension/hl25-partial-numbered-callsite-groups
tags:
- gamedata
- symbol-resolution
- hl25
- hl-10210
- manifest
- numbered-patches
- case-study
---

# HL25 引擎只发布部分编号 callsite 组时的 manifest 处理

## 触发信号

CZ / CS 1.6 HL25 引擎（`MetaHook.exe -insecure -game czero`，`Engine buildnum: 10210`）启动时，
在 `VGUIClient001` 初始化阶段弹出 fatal error 并终止进程：

```
[VGUI2Extension] Could not resolve gamedata symbol: vgui2_Panel_SetBounds_Const_callsite_0
(module gameui, symbol was not found in the matched gamedata snapshot)
Engine buildnum: 10210
```

`hl-10210` 的 `gameui` 记录被裁剪后，`SetSize` / `SetMinimumSize` / `SetBounds` 三组
编号 callsite 全部为 0 条。

## 根因

两处独立缺陷叠加：

1. **数据端（本仓库 manifest）**：`scripts/manifests/vgui2extension.json` 中负责编号 callsite 的
   `conditionalGroups`（G16 gameui / G17 serverbrowser）的 `when` 列表**不含 `hl-10210`**，
   于是 `sync-gamedata.py` 的 `manifest_keep_set()` 不保留这三组记录，pruned 快照把它们整体删除。
2. **消费端（同一处未提交改动）**：`src/GameUI.cpp` 的 `GameUI_PatchPanelSize()` 里 HL25 分支
   原本被注释掉，改动把它打开：

   ```cpp
   if (g_iEngineType == ENGINE_GOLDSRC_HL25) {
       PatchPanelSizeCallsites(..., "gameui", "vgui2_Panel_SetBounds_Const_callsite_", ...);
   }
   ```

   `PatchPanelSizeCallsites` 对 `index == 0` 用 `GamedataResolvePtr`（**必需**，缺失即 fatal），
   只有 `index >= 1` 才走 `...IfAvailable`。所以该分支一旦启用，`_0` 缺失必然致命。

`hl-10210` 由引擎 CRC64 匹配决定：`czero/cl_dlls/` 没有 `GameUI.dll`，引擎实际加载
`valve/cl_dlls/GameUI.dll`（CRC64-XZ `10b459a488880db0`），该 CRC 只属于 `hl-10210` 快照；
`GetEngineType()` 对 `build > 9000` 返回 `ENGINE_GOLDSRC_HL25`。

## 关键约束：`hl-10210` 必须独立成组

**不要把 `hl-10210` 加进 G16/G17。** `hl-10210` 只发布 `SetBounds` 组（`_0.._12`），
另外两组在上游快照里是 `vgui2_Panel_SetSize_ScaledConst_callsite_*` /
`vgui2_Panel_SetMinimumSize_ScaledConst_callsite_*` 变体，`SetSize` / `SetMinimumSize`
这两个**精确前缀**根本不存在：

| 组 | 上游 `hl-10210` 记录 |
| --- | --- |
| `gameui / vgui2_Panel_SetSize_Const_callsite` | 0 条 |
| `gameui / vgui2_Panel_SetMinimumSize_Const_callsite` | 0 条 |
| `gameui / vgui2_Panel_SetBounds_Const_callsite` | 13 条（`_0.._12`） |
| `serverbrowser / *` | 0 条 |

把 `hl-10210` 塞进 G16 会让 `validate-gamedata.py --manifest` 直接报
`missing conditional symbol 'vgui2_Panel_SetSize_Const_callsite_0'` —— 因为条件组的每个符号都按
「必需」校验。独立成组同时保留了「该 `when` 列表所列前缀确实存在」这一断言能力。

## 正确做法

`scripts/manifests/vgui2extension.json` 尾部新增一个条件组（当前为 G18）：

```json
{
  "when": ["hl-10210"],
  "symbols": {
    "gameui": { "vgui2_Panel_SetBounds_Const_callsite_0": "patch" }
  },
  "numberedPatchSets": [
    { "module": "gameui", "prefix": "vgui2_Panel_SetBounds_Const_callsite" }
  ]
}
```

- `symbols` 只声明 `_0`（manifest 契约要求显式列出首条），`numberedPatchSets` 负责 `_1.._n` 的连续保留。
- `numberedPatchSets` 的 `prefix` **不含末尾下划线**；源码前缀含下划线，再拼接编号。
- 同步更新 `docs/{en,zh-CN}/gamedata.md`：`G1`–`G18` 计数、条件表新增 G18 行、
  gameui 符号表新增 G18 行、编号补丁小节说明为何 `hl-10210` 独立成组。
- **只改 manifest 是不够的**：`sync-gamedata.py` 必须重跑才会重新生成 pruned 快照并部署；
  不要直接手改 `assets/.../gamedata/vgui2extension/*.json`。

## 验证方式

| 步骤 | 命令 | 期望 |
| --- | --- | --- |
| 同步 | `python scripts/sync-gamedata.py --target-dir <catalog> --manifest scripts/manifests/vgui2extension.json --temp-root <build>/gamedata-sync` | `published 21 pruned snapshot(s)` |
| 校验 | `python scripts/validate-gamedata.py <catalog> --manifest scripts/manifests/vgui2extension.json` | `validation passed`，exit 0 |
| 单测 | `python -m unittest discover -s scripts/tests` | OK |
| 实机 | 部署 catalog 后 `MetaHook.exe -insecure -game czero -windowed -novid` | 无 fatal；`halflifecli.endpoint.json` 达 `status:ready`；`errors.log` 不再新增 |

排障要点（本次踩到的坑）：

- **判定 pruned 产物以 `assets/**/gamedata/<plugin>/<gv>.json` 或线上快照为准**；
  `build/**/gamedata-sync/raw/snapshots/` 是 content-addressed 缓存，会残留多个历史版本
  （同一 `<gv>` 可能同时存在 3 个 `<gv>.<sha256>.json`），只看第一个会得出错误结论。
  用 `raw/index.json` 里该 `<gv>` 的 `sha256` 去匹配你正在看的那个文件。
- 上游快照里同一 `symbolName` 常有 **linux + windows 两条记录**（RVA/签名不同）。
  这是正常现象：`prune_snapshot` 只保留 `platform == "windows"`，不会触发运行期的
  `MH_GAMESYMBOL_CATALOG_CONFLICT`（该状态只在同模块同符号出现**不同** payload 时产生）。
- 排查「还有没有别的必需符号缺失」时，**不要只 grep 符号名**：模块名（`"gameui"`、`"engine"`）
  也会被同一正则匹配成伪符号。应手工维护必需符号清单，逐条在 pruned 快照里查找。

## 适用范围

- 适用于「某引擎标识只发布编号 callsite 组的一个子集」的一般情形；`hl-10210` 是首个实例。
- 具体判定（`hl-10210` 只发 `SetBounds`）绑定到 GoldSrc_VibeSignatures `2026-10-06` 发布时点的
  gamedata；上游补齐 `SetSize` / `SetMinimumSize` 后应把该版本并入 G16 并删除 G18，不要照抄。
- 该结论只覆盖「解析必需符号」这一层；HL25 分支 `GameUI_Panel_SetBounds_HL25` 的缩放行为
  仍需实机交互验证，本次未做。
