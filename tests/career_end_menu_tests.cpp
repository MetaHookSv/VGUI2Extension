#include "../src/CareerEndMenuInput.h"
#include <cassert>
#include <initializer_list>

enum class TestKey
{
    Escape,
    Enter,
    Other
};

struct NativeCalls
{
    int         keyCalls     = 0;
    int         commandCalls = 0;
    int         dummy        = 0;
    TestKey     key          = TestKey::Escape;
    const char* command      = nullptr;
};

static void __fastcall NativeKey(void* self, int dummy, TestKey key)
{
    auto& calls = *static_cast<NativeCalls*>(self);
    ++calls.keyCalls;
    calls.dummy = dummy;
    calls.key   = key;
}

static void __fastcall NativeCommand(void* self, int dummy, const char* command)
{
    auto& calls = *static_cast<NativeCalls*>(self);
    ++calls.commandCalls;
    calls.dummy   = dummy;
    calls.command = command;
}

int main()
{
    NativeCalls roundEnd;
    NativeCalls matchEnd;
    for (auto* calls : {&roundEnd, &matchEnd})
    {
        //Both Pressed and Typed adapters must leave native close/unpause handlers untouched.
        for (int path = 0; path < 2; ++path)
        {
            ForwardCareerEndMenuKey(NativeKey, calls, path, TestKey::Escape, TestKey::Escape);
            assert(0 == calls->keyCalls);
        }
        for (const char* command : {"Close", "Cancel", "cLoSe", "cAnCeL"})
            ForwardCareerEndMenuCommand(NativeCommand, calls, 0, command);
        assert(0 == calls->commandCalls);

        for (TestKey key : {TestKey::Enter, TestKey::Other})
        {
            ForwardCareerEndMenuKey(NativeKey, calls, 1, key, TestKey::Escape);
            assert(key == calls->key && 1 == calls->dummy);
        }
        assert(2 == calls->keyCalls);

        //Native business commands must receive the original pointer without rewriting it.
        for (const char* command : {"disconnect", "career_restart", "Credits", "career_continue", "surrender", "vguicancel", "CloseModal", "", static_cast<const char*>(nullptr)})
        {
            const auto previousCalls = calls->commandCalls;
            ForwardCareerEndMenuCommand(NativeCommand, calls, 1, command);
            assert(previousCalls + 1 == calls->commandCalls);
            assert(command == calls->command && 1 == calls->dummy);
        }
    }
}
