#pragma once

using TeamMenuLoadMapPageFn = void(__fastcall*)(void*, int, const char*);
using TeamMenuUpdateFn      = void(__fastcall*)(void*, int, const char*, bool, const char**, int);

//Restore the previous scope even when a native callback nests another update.
template <typename Function, typename... Args>
void CallWithMapPageText(bool& active, Function function, Args... args)
{
    struct Restore
    {
        bool& active;
        bool  previous;
        ~Restore() { active = previous; }
    } restore{active, active};
    active = true;
    function(args...);
}
