#include "../src/CareerPopupOrder.h"
#include <cassert>
#include <vector>

struct Node
{
    Node* parent  = nullptr;
    bool  visible = true;
    bool  popup   = false;
};

struct Panels
{
    std::vector<Node*> popups;
    int                queries = 0;

    int GetPopupCount()
    {
        ++queries;
        return static_cast<int>(popups.size());
    }
    Node* GetPopup(int index) { return popups[index]; }
    Node* GetParent(Node* panel) { return panel->parent; }
    bool  IsVisible(Node* panel) { return panel->visible; }
    bool  IsPopup(Node* panel) { return panel->popup; }
};

int main()
{
    Node   box, combo{&box}, menu{&combo, true, true}, otherBox, otherMenu{&otherBox, true, true};
    Panels panels{{nullptr, &box, &otherMenu, &menu}};
    bool   inTick   = false;
    auto   preserve = [&] {
        return ShouldPreserveCareerPopupOrder(inTick, &box, &box, panels);
    };

    //Initial display and direct BringToFront callers must retain native behavior.
    assert(!preserve());
    assert(0 == panels.queries);
    {
        CareerViewportTickScope scope(inTick);
        assert(inTick);
        assert(preserve());
        {
            CareerViewportTickScope nested(inTick);
            assert(preserve());
        }
        assert(inTick);

        //Do not suppress establishing or changing the native modal surface.
        assert(!ShouldPreserveCareerPopupOrder(inTick, &box, static_cast<Node*>(nullptr), panels));
        assert(!ShouldPreserveCareerPopupOrder(inTick, &box, &otherBox, panels));
        assert(!ShouldPreserveCareerPopupOrder(inTick, static_cast<Node*>(nullptr), &box, panels));

        menu.visible = false;
        assert(!preserve());
        menu.visible  = true;
        combo.visible = false;
        assert(!preserve());
        combo.visible = true;
        box.visible   = false;
        assert(!preserve());
        box.visible = true;
        menu.popup  = false;
        assert(!preserve());
        menu.popup = true;

        //Use current ancestry; stale/reparented and unrelated popups cannot block it.
        menu.parent = &otherBox;
        assert(!preserve());
        menu.parent = &combo;
        assert(preserve());
        panels.popups = {&box, &otherMenu};
        assert(!preserve());
        panels.popups.clear();
        assert(!preserve());
    }
    assert(!inTick);
    try
    {
        CareerViewportTickScope scope(inTick);
        throw 1;
    }
    catch (int)
    {
        assert(!inTick);
    }
}
