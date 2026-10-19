#pragma once

class CareerViewportTickScope
{
public:
    explicit CareerViewportTickScope(bool& active) : m_active(active), m_previous(active) { m_active = true; }
    ~CareerViewportTickScope() { m_active = m_previous; }
    CareerViewportTickScope(const CareerViewportTickScope&)            = delete;
    CareerViewportTickScope& operator=(const CareerViewportTickScope&) = delete;

private:
    bool& m_active;
    bool  m_previous;
};

//Only defer a repeated Tick activation of an already-modal box with a visible
//descendant popup. Walk the live ancestry so hidden parents and unrelated menus
//cannot suppress the native activation or its modal setup.
template <typename Panel, typename Panels>
bool ShouldPreserveCareerPopupOrder(bool inViewportTick, Panel box, Panel modal, Panels& panels)
{
    if (!inViewportTick || !box || modal != box || !panels.IsVisible(box))
        return false;

    const int count = panels.GetPopupCount();
    for (int i = 0; i < count; ++i)
    {
        auto popup = panels.GetPopup(i);
        if (!popup || popup == box || !panels.IsPopup(popup))
            continue;

        for (auto panel = popup; panel; panel = panels.GetParent(panel))
        {
            if (!panels.IsVisible(panel))
                break;
            if (panel == box)
                return true;
        }
    }
    return false;
}
