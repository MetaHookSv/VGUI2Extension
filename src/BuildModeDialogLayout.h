#pragma once

#include <algorithm>

namespace BuildModeLayout
{
//Defaults of the native GameUI editor, rather than the newer SDK dialog.
constexpr int DefaultWide = 300;
constexpr int DefaultTall = 400;
constexpr int MinimumTall = 256;

struct Rect
{
    int x, y, wide, tall;
};

struct Metrics
{
    int margin, gap, groupGap, rowTall, dividerTall;
    int buttonWide, variablesWide, addWide, columnWide, cellGap;
};

template <typename Scale>
Metrics Measure(Scale scale, int fontTall)
{
    constexpr Metrics design{8, 6, 8, 24, 2, 64, 72, 116, 100, 10};
    constexpr int     fontPadding = 6;
    //Font measurements and client bounds are already in pixels.
    return {scale(design.margin), scale(design.gap), scale(design.groupGap),
            (std::max)(scale(design.rowTall), fontTall + scale(fontPadding)),
            (std::max)(1, scale(design.dividerTall)), scale(design.buttonWide),
            scale(design.variablesWide), scale(design.addWide), scale(design.columnWide), scale(design.cellGap)};
}

struct Layout
{
    Rect file, status, list, divider, save, exit, apply, variables, remove, add;
};

inline bool Calculate(Rect client, const Metrics& m, Layout& out)
{
    const int left        = client.x + m.margin;
    const int wide        = client.wide - 2 * m.margin;
    const int bottom      = client.y + client.tall - m.margin;
    const int toolsWide   = m.variablesWide + m.buttonWide + m.addWide + 2 * m.gap;
    const int actionsWide = 3 * m.buttonWide + 2 * m.gap;
    if (m.rowTall <= 0 || wide < (std::max)(toolsWide, actionsWide))
        return false;

    Layout result{};
    result.file      = {left, client.y + m.margin, wide, m.rowTall};
    result.status    = {left, result.file.y + m.rowTall + m.gap, wide, m.rowTall};
    result.apply     = {left + wide - m.buttonWide, bottom - m.rowTall, m.buttonWide, m.rowTall};
    result.exit      = {result.apply.x - m.gap - m.buttonWide, result.apply.y, m.buttonWide, m.rowTall};
    result.save      = {result.exit.x - m.gap - m.buttonWide, result.apply.y, m.buttonWide, m.rowTall};
    const int toolsY = result.apply.y - m.groupGap - m.rowTall;
    result.variables = {left, toolsY, m.variablesWide, m.rowTall};
    result.remove    = {left + m.variablesWide + m.gap, toolsY, m.buttonWide, m.rowTall};
    result.add       = {result.remove.x + m.buttonWide + m.gap, toolsY, m.addWide, m.rowTall};
    result.divider   = {left, toolsY - m.groupGap - m.dividerTall, wide, m.dividerTall};
    const int listY  = result.status.y + m.rowTall + m.gap;
    result.list      = {left, listY, wide, result.divider.y - m.groupGap - listY};
    if (result.list.tall < m.rowTall)
        return false;

    out = result;
    return true;
}
} // namespace BuildModeLayout
