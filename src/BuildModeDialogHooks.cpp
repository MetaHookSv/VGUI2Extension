#include <metahook.h>
#include <vgui/IPanel.h>
#include <vgui/ISystem.h>
#include <vgui_controls/Controls.h>
#include <KeyValues.h>

#include "plugins.h"
#include "privatefuncs.h"
#include "DpiManagerInternal.h"
#include "BuildModeDialogHooks.h"
#include "BuildModeDialogLayout.h"

namespace
{
using NativePanel     = void*;
using LayoutFn        = void(__fastcall*)(NativePanel, int);
using ConstructorFn   = NativePanel(__fastcall*)(NativePanel, int, void*);
using ActiveControlFn = void(__fastcall*)(NativePanel, int, NativePanel);

struct Symbols
{
    void**        editorTable = nullptr;
    void**        listTable   = nullptr;
    DWORD         layoutSlot = 0, activeSlot = 0, listLayoutSlot = 0;
    DWORD         getVPanelSlot = 0, clientAreaSlot = 0;
    DWORD         itemCountSlot = 0, itemLabelSlot = 0, itemPanelSlot = 0;
    DWORD         addComboOffset = 0, sortedMemoryOffset = 0, sortedSizeOffset = 0;
    DWORD         columnOffset = 0, bufferOffset = 0;
    ConstructorFn construct = nullptr;
} g_symbols;

hook_t*         g_constructorHook = nullptr;
hook_t*         g_layoutHook      = nullptr;
hook_t*         g_activeHook      = nullptr;
hook_t*         g_listLayoutHook  = nullptr;
LayoutFn        g_layout          = nullptr;
LayoutFn        g_listLayout      = nullptr;
ActiveControlFn g_setActive       = nullptr;

template <typename T>
T& Member(NativePanel panel, DWORD offset)
{
    return *reinterpret_cast<T*>(static_cast<byte*>(panel) + offset);
}

template <typename Fn>
Fn Method(NativePanel panel, DWORD slot)
{
    return reinterpret_cast<Fn>((*static_cast<void***>(panel))[slot]);
}

vgui::VPANEL GetVPanel(NativePanel panel)
{
    return panel ? Method<vgui::VPANEL(__fastcall*)(NativePanel, int)>(panel, g_symbols.getVPanelSlot)(panel, 0) : 0;
}

NativePanel Client(vgui::VPANEL panel)
{
    return panel ? vgui::ipanel()->Client(panel) : nullptr;
}

bool IsEditor(NativePanel panel)
{
    return panel && *static_cast<void***>(panel) == g_symbols.editorTable;
}

bool ShouldScale(vgui::VPANEL panel)
{
    return panel && DpiManagerInternal()->IsHighDpiSupportEnabled() && vgui::ipanel()->IsProportional(panel);
}

vgui::VPANEL FindPanel(vgui::VPANEL parent, const char* name)
{
    //Only direct editor/list children are needed. Do not accidentally find a
    //property's nested dropdown or a similarly named panel in the edited UI.
    auto* panels = vgui::ipanel();
    for (int i = 0; i < panels->GetChildCount(parent); ++i)
    {
        const auto child = panels->GetChild(parent, i);
        if (!strcmp(panels->GetName(child), name))
            return child;
    }
    return 0;
}

auto Metrics(vgui::VPANEL editor)
{
    const auto scheme = vgui::ipanel()->GetScheme(editor);
    const auto scale  = [=](int value) {
        return g_pVGuiSchemeManager2->GetProportionalScaledValueEx(scheme, value);
    };
    int fontTall = 0;
    if (auto* settings = g_pVGuiSchemeManager2->GetIScheme(scheme))
    {
        //The native editor inherits Frame's scheme handler. Its children already
        //use proportional scheme fonts; measure them without changing shared fonts.
        for (const auto* name : {"Default", "DefaultSmall", "DefaultVerySmall"})
        {
            const auto font = settings->GetFont(name, true);
            if (font)
                fontTall = (std::max)(fontTall, vgui::surface()->GetFontTall(font));
        }
    }
    return BuildModeLayout::Measure(scale, fontTall);
}

void SetBounds(vgui::VPANEL panel, BuildModeLayout::Rect bounds)
{
    auto* panels = vgui::ipanel();
    int   x, y, wide, tall;
    panels->GetPos(panel, x, y);
    panels->GetSize(panel, wide, tall);
    if (x != bounds.x || y != bounds.y)
        panels->SetPos(panel, bounds.x, bounds.y);
    if (wide != bounds.wide || tall != bounds.tall)
        panels->SetSize(panel, bounds.wide, bounds.tall);
}

void SetRowHeight(vgui::VPANEL panel, int rowTall)
{
    if (!panel)
        return;
    int wide, tall;
    vgui::ipanel()->GetSize(panel, wide, tall);
    if (tall != rowTall)
        vgui::ipanel()->SetSize(panel, wide, rowTall);
}

bool GetRows(NativePanel list, int& count, const int*& ids)
{
    count                 = Method<int(__fastcall*)(NativePanel, int)>(list, g_symbols.itemCountSlot)(list, 0);
    const int sortedCount = Member<int>(list, g_symbols.sortedSizeOffset);
    ids                   = Member<const int*>(list, g_symbols.sortedMemoryOffset);
    //Only read the two published vector fields. Neither the SDK CUtlVector nor
    //the native linked-list node layout is part of this consumer's ABI.
    return count >= 0 && sortedCount == count && (count == 0 || ids);
}

void GetRow(NativePanel list, int id, vgui::VPANEL& label, vgui::VPANEL& edit)
{
    using GetItemFn = NativePanel(__fastcall*)(NativePanel, int, int);
    label           = GetVPanel(Method<GetItemFn>(list, g_symbols.itemLabelSlot)(list, 0, id));
    edit            = GetVPanel(Method<GetItemFn>(list, g_symbols.itemPanelSlot)(list, 0, id));
}

void SizeRows(NativePanel list, const BuildModeLayout::Metrics& m)
{
    int        count;
    const int* ids;
    if (!GetRows(list, count, ids))
        return;
    Member<int>(list, g_symbols.columnOffset) = m.columnWide;
    Member<int>(list, g_symbols.bufferOffset) = m.cellGap;
    for (int row = 0; row < count; ++row)
    {
        vgui::VPANEL label, edit;
        GetRow(list, ids[row], label, edit);
        SetRowHeight(label, m.rowTall);
        SetRowHeight(edit, m.rowTall);
    }
}

void LayoutEditor(NativePanel self)
{
    const auto editor = GetVPanel(self);
    if (!ShouldScale(editor))
        return;

    struct Control
    {
        const char*           name;
        BuildModeLayout::Rect BuildModeLayout::Layout::* bounds;
        vgui::VPANEL                                     panel = 0;
    } controls[] = {
        {"FileSelectionCombo", &BuildModeLayout::Layout::file},
        {"StatusLabel", &BuildModeLayout::Layout::status},
        {"BuildModeControls", &BuildModeLayout::Layout::list},
        {"Divider", &BuildModeLayout::Layout::divider},
        {"SaveButton", &BuildModeLayout::Layout::save},
        {"ExitButton", &BuildModeLayout::Layout::exit},
        {"ApplyButton", &BuildModeLayout::Layout::apply},
        {"VarsButton", &BuildModeLayout::Layout::variables},
        {"DeletePanelButton", &BuildModeLayout::Layout::remove},
    };
    for (auto& control : controls)
    {
        control.panel = FindPanel(editor, control.name);
        if (!control.panel)
            return;
    }
    const auto add = GetVPanel(Member<NativePanel>(self, g_symbols.addComboOffset));
    if (!add || vgui::ipanel()->GetParent(add) != editor)
        return;

    BuildModeLayout::Rect client{};
    using ClientAreaFn = void(__fastcall*)(NativePanel, int, int&, int&, int&, int&);
    Method<ClientAreaFn>(self, g_symbols.clientAreaSlot)(self, 0, client.x, client.y, client.wide, client.tall);
    BuildModeLayout::Layout layout{};
    if (!BuildModeLayout::Calculate(client, Metrics(editor), layout))
        return;
    //Resolve the whole plan before applying it. All coordinates are local pixels.
    for (const auto& control : controls)
        SetBounds(control.panel, layout.*(control.bounds));
    SetBounds(add, layout.add);
}

void __fastcall PerformLayout(NativePanel self, int dummy)
{
    g_layout(self, dummy);
    LayoutEditor(self);
}

void __fastcall ListPerformLayout(NativePanel self, int dummy)
{
    const auto list   = GetVPanel(self);
    const auto editor = vgui::ipanel()->GetParent(list);
    const bool fix    = IsEditor(Client(editor)) && ShouldScale(editor);
    if (fix)
        SizeRows(self, Metrics(editor));
    //The native list owns scroll range, slider state and the embedded panel.
    g_listLayout(self, dummy);
    if (!fix)
        return;

    const auto embedded = FindPanel(list, "PanelListEmbedded");
    int        count;
    const int* ids;
    if (!embedded || !GetRows(self, count, ids))
        return;
    const auto m = Metrics(editor);
    int        wide, tall;
    vgui::ipanel()->GetSize(embedded, wide, tall);
    int listWide, listTall;
    vgui::ipanel()->GetSize(list, listWide, listTall);
    int left, top, right, bottom;
    vgui::ipanel()->GetInset(list, left, top, right, bottom);
    const int clientWide = listWide - left - right;
    const int clientTall = listTall - top - bottom;
    //The native list disables its scrollbar even when the content overflows.
    //Keep the native range/slider calculations, but allow normal dragging.
    if (const auto scrollbar = FindPanel(list, "PanelListPanelVScroll"))
    {
        int barWide, barTall;
        vgui::ipanel()->GetSize(scrollbar, barWide, barTall);
        if (clientWide <= barWide || clientTall <= 0)
            return;
        SetBounds(scrollbar, {clientWide - barWide, 0, barWide, clientTall});
        vgui::ipanel()->SetEnabled(scrollbar, tall > clientTall);
        int x, y;
        vgui::ipanel()->GetPos(embedded, x, y);
        wide = clientWide - barWide;
        SetBounds(embedded, {0, y, wide, tall});
    }
    const int editX    = m.margin + m.columnWide + m.cellGap;
    const int editWide = wide - editX - m.margin;
    if (editWide <= 0)
        return;
    //Native layout still uses fixed horizontal insets and can extend an editor
    //past its embedded panel. Reposition every row, including rows off screen,
    //in content coordinates; retain the native embedded panel's scroll offset.
    int y = m.cellGap;
    for (int row = 0; row < count; ++row)
    {
        vgui::VPANEL label, edit;
        GetRow(self, ids[row], label, edit);
        if (label)
            SetBounds(label, {m.margin, y, m.columnWide, m.rowTall});
        if (edit)
            SetBounds(edit, {editX, y, editWide, m.rowTall});
        y += m.rowTall + m.cellGap;
    }
}

void __fastcall SetActiveControl(NativePanel self, int dummy, NativePanel selected)
{
    g_setActive(self, dummy, selected);
    const auto editor = GetVPanel(self);
    if (!ShouldScale(editor))
        return;
    const auto list = FindPanel(editor, "BuildModeControls");
    if (auto* nativeList = Client(list))
    {
        //Do not wait for the outer dialog to invalidate: native SetActiveControl
        //can rebuild the property rows without changing the frame's dimensions.
        ListPerformLayout(nativeList, 0);
    }
}

NativePanel __fastcall ConstructEditor(NativePanel self, int dummy, void* group)
{
    const auto result = g_symbols.construct(self, dummy, group);
    const auto editor = GetVPanel(result);
    if (!ShouldScale(editor))
        return result;

    const auto scheme = vgui::ipanel()->GetScheme(editor);
    const auto scale  = [=](int value) {
        return g_pVGuiSchemeManager2->GetProportionalScaledValueEx(scheme, value);
    };
    //Native LoadUserConfig has already run. Persisted dimensions are pixels;
    //scale only missing defaults, not an existing size on each editor reopening.
    auto*      config        = vgui::system()->GetUserConfigFileData("BuildModeDialog", 0);
    const bool savedWide     = config && config->GetInt("wide", 0) > 0;
    const bool savedTall     = config && config->GetInt("tall", 0) > 0;
    const bool savedPosition = config && config->FindKey("xpos", false) && config->FindKey("ypos", false);
    int        workX, workY, workWide, workTall;
    vgui::surface()->GetWorkspaceBounds(workX, workY, workWide, workTall);
    if (workWide <= 0 || workTall <= 0)
        return result;
    const int minWide = (std::min)(scale(BuildModeLayout::DefaultWide), workWide);
    const int minTall = (std::min)(scale(BuildModeLayout::MinimumTall), workTall);
    vgui::ipanel()->SetMinimumSize(editor, minWide, minTall);
    int x, y, wide, tall;
    vgui::ipanel()->GetPos(editor, x, y);
    vgui::ipanel()->GetSize(editor, wide, tall);
    wide = std::clamp(savedWide ? wide : scale(BuildModeLayout::DefaultWide), minWide, workWide);
    tall = std::clamp(savedTall ? tall : scale(BuildModeLayout::DefaultTall), minTall, workTall);
    if (!savedPosition)
    {
        x = workX + (workWide - wide) / 2;
        y = workY + (workTall - tall) / 2;
    }
    SetBounds(editor, {std::clamp(x, workX, workX + workWide - wide), std::clamp(y, workY, workY + workTall - tall), wide, tall});
    return result;
}
} // namespace

void BuildModeDialog_FillAddress()
{
    const auto base       = g_GameUIDllInfo.ImageBase;
    g_symbols.editorTable = static_cast<void**>(GamedataResolvePtrIfAvailable(base, "gameui", "vgui2::BuildModeDialog", MH_GAMESYMBOL_KIND_VTABLE));
    if (!g_symbols.editorTable)
        return;
    g_symbols.listTable          = static_cast<void**>(GamedataResolvePtr(base, "gameui", "vgui2::PanelListPanel", MH_GAMESYMBOL_KIND_VTABLE));
    g_symbols.construct          = reinterpret_cast<ConstructorFn>(GamedataResolvePtr(base, "gameui", "vgui2::BuildModeDialog::BuildModeDialog(vgui2::BuildGroup*)", MH_GAMESYMBOL_KIND_FUNCTION));
    g_symbols.layoutSlot         = GamedataResolveVFuncIndex(base, "gameui", "vgui2::BuildModeDialog::PerformLayout()");
    g_symbols.activeSlot         = GamedataResolveVFuncIndex(base, "gameui", "vgui2::BuildModeDialog::SetActiveControl(vgui2::Panel*)");
    g_symbols.listLayoutSlot     = GamedataResolveVFuncIndex(base, "gameui", "vgui2::PanelListPanel::PerformLayout()");
    g_symbols.getVPanelSlot      = GamedataResolveVFuncIndex(base, "gameui", "vgui2::Panel::GetVPanel()");
    g_symbols.clientAreaSlot     = GamedataResolveVFuncIndex(base, "gameui", "vgui2::Frame::GetClientArea(int&, int&, int&, int&)");
    g_symbols.itemCountSlot      = GamedataResolveVFuncIndex(base, "gameui", "vgui2::PanelListPanel::GetItemCount()");
    g_symbols.itemLabelSlot      = GamedataResolveVFuncIndex(base, "gameui", "vgui2::PanelListPanel::GetItemLabel(int)");
    g_symbols.itemPanelSlot      = GamedataResolveVFuncIndex(base, "gameui", "vgui2::PanelListPanel::GetItemPanel(int)");
    g_symbols.addComboOffset     = GamedataResolveStructMember(base, "gameui", "vgui2::BuildModeDialog.m_pAddNewControlCombo");
    g_symbols.sortedMemoryOffset = GamedataResolveStructMember(base, "gameui", "vgui2::PanelListPanel.m_SortedItems_m_pMemory");
    g_symbols.sortedSizeOffset   = GamedataResolveStructMember(base, "gameui", "vgui2::PanelListPanel.m_SortedItems_m_Size");
    g_symbols.columnOffset       = GamedataResolveStructMember(base, "gameui", "vgui2::PanelListPanel.m_iFirstColumnWidth");
    g_symbols.bufferOffset       = GamedataResolveStructMember(base, "gameui", "vgui2::PanelListPanel.m_iPanelBuffer");
}

void BuildModeDialog_InstallHooks()
{
    if (!g_symbols.editorTable || g_constructorHook)
        return;
    g_constructorHook = g_pMetaHookAPI->InlineHook(reinterpret_cast<void*>(g_symbols.construct), ConstructEditor, reinterpret_cast<void**>(&g_symbols.construct));
    g_layoutHook      = g_pMetaHookAPI->VFTHookEx(g_symbols.editorTable, g_symbols.layoutSlot, PerformLayout, reinterpret_cast<void**>(&g_layout));
    g_activeHook      = g_pMetaHookAPI->VFTHookEx(g_symbols.editorTable, g_symbols.activeSlot, SetActiveControl, reinterpret_cast<void**>(&g_setActive));
    g_listLayoutHook  = g_pMetaHookAPI->VFTHookEx(g_symbols.listTable, g_symbols.listLayoutSlot, ListPerformLayout, reinterpret_cast<void**>(&g_listLayout));
    if (!g_constructorHook || !g_layoutHook || !g_activeHook || !g_listLayoutHook)
    {
        BuildModeDialog_UninstallHooks();
        Sys_Error("Could not install GameUI build editor DPI hooks.");
    }
}

void BuildModeDialog_UninstallHooks()
{
    for (auto** hook : {&g_listLayoutHook, &g_activeHook, &g_layoutHook, &g_constructorHook})
    {
        if (*hook)
        {
            g_pMetaHookAPI->UnHook(*hook);
            *hook = nullptr;
        }
    }
    g_layout     = nullptr;
    g_listLayout = nullptr;
    g_setActive  = nullptr;
    g_symbols    = {};
}
