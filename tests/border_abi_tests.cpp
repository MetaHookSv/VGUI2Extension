#include <vgui/IBorder.h>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

class BorderProbe : public vgui::IBorder
{
public:
    void Paint(vgui::VPANEL) override {}
    void Paint(int, int, int, int) override {}
    void Paint(int, int, int, int, int, int, int) override {}
    void SetInset(int, int, int, int) override {}
    void GetInset(int&, int&, int&, int&) override {}
    void ApplySchemeSettings(vgui::IScheme*, KeyValues*) override {}

    // If this helper becomes virtual again, native GetName dispatch must fail.
    void ParseSideSettings(int, KeyValues*, vgui::IScheme*)
    {
        std::fputs("Native IBorder dispatch reached ParseSideSettings\n", stderr);
        std::exit(EXIT_FAILURE);
    }

    const char* GetName() override
    {
        ++getNameCalls;
        return name;
    }

    void SetName(const char* value) override
    {
        ++setNameCalls;
        name = value;
    }

    backgroundtype_e GetBackgroundType() override { return BACKGROUND_FILLED; }
    bool             PaintFirst() override { return false; }

    const char* name         = nullptr;
    int         getNameCalls = 0;
    int         setNameCalls = 0;
};

int main()
{
    // GoldSrc clients call these slots using the native IBorder ABI.
    constexpr std::size_t NativeGetNameSlot = 6;
    constexpr std::size_t NativeSetNameSlot = 7;
    using GetNameFn                         = const char*(__fastcall*)(void*, int);
    using SetNameFn                         = void(__fastcall*)(void*, int, const char*);

    BorderProbe border;
    auto        vtable  = *reinterpret_cast<void***>(&border);
    auto        getName = reinterpret_cast<GetNameFn>(vtable[NativeGetNameSlot]);
    auto        setName = reinterpret_cast<SetNameFn>(vtable[NativeSetNameSlot]);

    assert(nullptr == getName(&border, 0));
    assert(1 == border.getNameCalls);

    const char name[] = "ScoreboardBorder";
    setName(&border, 0, name);
    assert(1 == border.setNameCalls);
    assert(name == getName(&border, 0));
    assert(2 == border.getNameCalls);
}
