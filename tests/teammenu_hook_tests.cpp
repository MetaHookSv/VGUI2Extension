#include "../src/TeamMenuText.h"
#include <cassert>

static bool        active = false;
static int         object;
static const char  map[]   = "de_dust2";
static const char* teams[] = {"T", "CT", "Auto", "Spectator"};
static int         calls;

static void __fastcall Load(void* self, int, const char* name)
{
    assert(active && &object == self && map == name);
    ++calls;
}

static void __fastcall Update(void* self, int, const char* name, bool spectators, const char** names, int count)
{
    assert(active && &object == self && map == name);
    assert((spectators && teams == names && 2 == count) || (!spectators && nullptr == names && 0 == count));
    CallWithMapPageText(active, TeamMenuLoadMapPageFn{Load}, self, 0, name);
    assert(active);
    ++calls;
}

int main()
{
    for (int i = 0; i < 100; ++i)
    {
        CallWithMapPageText(active, TeamMenuLoadMapPageFn{Load}, &object, 0, map);
        assert(!active);
        CallWithMapPageText(active, TeamMenuUpdateFn{Update}, &object, 0, map, false, nullptr, 0);
        assert(!active);
        CallWithMapPageText(active, TeamMenuUpdateFn{Update}, &object, 0, map, true, teams, 2);
        assert(!active);
    }
    assert(500 == calls);
    active = true;
    CallWithMapPageText(active, TeamMenuLoadMapPageFn{Load}, &object, 0, map);
    assert(active && 501 == calls);
}
