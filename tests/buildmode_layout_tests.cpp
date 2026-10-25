#include "../src/BuildModeDialogLayout.h"
#include <cassert>

using namespace BuildModeLayout;

static void CheckBounds(Rect client, const Layout& l)
{
    for (auto r : {l.file, l.status, l.list, l.divider, l.save, l.exit, l.apply, l.variables, l.remove, l.add})
    {
        assert(client.x <= r.x && client.y <= r.y);
        assert(r.wide > 0 && r.tall > 0);
        assert(r.x + r.wide <= client.x + client.wide);
        assert(r.y + r.tall <= client.y + client.tall);
    }
    assert(l.file.y + l.file.tall <= l.status.y);
    assert(l.status.y + l.status.tall <= l.list.y);
    assert(l.list.y + l.list.tall <= l.divider.y);
    assert(l.divider.y + l.divider.tall <= l.variables.y);
    assert(l.variables.y + l.variables.tall <= l.save.y);
    assert(l.variables.x + l.variables.wide <= l.remove.x);
    assert(l.remove.x + l.remove.wide <= l.add.x);
    assert(l.save.x + l.save.wide <= l.exit.x);
    assert(l.exit.x + l.exit.wide <= l.apply.x);
}

int main()
{
    //Use actual pixel client rectangles with an independently scaled caption.
    //The same design must fit the default and minimum editor sizes at each scale.
    for (int scale : {1, 2, 3})
    {
        const auto m = Measure([=](int value) { return value * scale; }, 14 * scale);
        for (int tall : {256, 400, 600})
        {
            Rect   client{5 * scale, 28 * scale, 290 * scale, (tall - 33) * scale};
            Layout layout{};
            assert(Calculate(client, m, layout));
            CheckBounds(client, layout);
            //Repeated native invalidations must not accumulate scaling.
            Layout repeated{};
            assert(Calculate(client, m, repeated));
            assert(layout.list.x == repeated.list.x && layout.list.tall == repeated.list.tall);
            assert(layout.apply.x == repeated.apply.x && layout.apply.y == repeated.apply.y);
        }
    }

    //A measured 60px font is not multiplied by the proportional scale again.
    const auto largeFont = Measure([](int value) { return value * 2; }, 60);
    assert(72 == largeFont.rowTall);
    Layout l{};
    assert(Calculate({10, 56, 580, 734}, largeFont, l));
    CheckBounds({10, 56, 580, 734}, l);

    //Reject an incomplete plan before mutating any controls in a tiny client area.
    const auto normal = Measure([](int value) { return value; }, 14);
    Layout     sentinel{};
    sentinel.list = {1, 2, 3, 4};
    assert(!Calculate({0, 0, 100, 400}, normal, sentinel));
    assert(!Calculate({0, 0, 300, 100}, normal, sentinel));
    assert(4 == sentinel.list.tall);
}
