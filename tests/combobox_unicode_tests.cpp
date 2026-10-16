#include "../src/ComboBoxUnicode.h"

#include <cassert>
#include <cstring>
#include <cstddef>
#include <string>

struct Message
{
    inline static int alive = 0;
    std::wstring      text;
    Message(const char* name, const char* key, const wchar_t* value) : text(value)
    {
        assert(0 == std::strcmp("SetText", name));
        assert(0 == std::strcmp("text", key));
        ++alive;
    }
    ~Message() { --alive; }
};

struct FakePanel
{
    void** vtable;
};

static FakePanel    g_menu;
static FakePanel    g_combo;
static const void*  expectedUser;
static std::string  g_caption;
static std::wstring selected;
static int          updates;
static int          invalidations;

static void* __fastcall FakeGetMenu(void* self, int)
{
    assert(&g_combo == self);
    return &g_menu;
}

static int __fastcall AddMenuItem(void* self, int, const char* text, Message* message, void* target, const void* user)
{
    assert(&g_menu == self && &g_combo == target && expectedUser == user);
    g_caption = text;
    selected  = message->text;
    delete message;
    return 0x123456;
}

static bool __fastcall IsValid(void* self, int, int id)
{
    assert(&g_combo == self);
    return id == 42;
}

static void __fastcall UpdateMenuItem(void* self, int, int id, const wchar_t* text, Message* message, const void* user)
{
    assert(&g_menu == self && 42 == id && expectedUser == user);
    assert(std::wstring(text) == message->text);
    selected = message->text;
    ++updates;
    delete message;
}

static void __fastcall Invalidate(void* self, int, bool layoutNow, bool reloadScheme)
{
    assert(&g_combo == self && !layoutNow && !reloadScheme);
    ++invalidations;
}

static void TestNativeDispatch()
{
    //Deliberately unrelated slots exercise dispatch through each object's own table.
    void* comboTable[] = {reinterpret_cast<void*>(IsValid), reinterpret_cast<void*>(Invalidate), reinterpret_cast<void*>(FakeGetMenu)};
    void* menuTable[]  = {reinterpret_cast<void*>(UpdateMenuItem), reinterpret_cast<void*>(AddMenuItem)};
    g_combo.vtable     = comboTable;
    g_menu.vtable      = menuTable;
    int user           = 123;
    expectedUser       = &user;
    const ComboBoxUnicodeSlots slots{2, 1, 0, 0, 1};
    const wchar_t*             text = L"Bot\u4e2d\u6587 (CT)";
    auto                       id   = AddUnicodeComboBoxItem<Message>(&g_combo, text, &user, slots);
    assert(id && 0x123456 == *id);
    assert("Bot\xe4\xb8\xad\xe6\x96\x87 (CT)" == g_caption);
    assert(text == selected);
    assert(!AddUnicodeComboBoxItem<Message>(&g_combo, nullptr, &user, slots));
    assert(!UpdateUnicodeComboBoxItem<Message>(&g_combo, -1, text, &user, slots));
    assert(!UpdateUnicodeComboBoxItem<Message>(&g_combo, 42, nullptr, &user, slots));
    assert(0 == updates && 0 == invalidations && 0 == Message::alive);
    assert(UpdateUnicodeComboBoxItem<Message>(&g_combo, 42, text, &user, slots));
    assert(text == selected && 1 == updates && 1 == invalidations && 0 == Message::alive);
    assert(UpdateUnicodeComboBoxItem<Message>(&g_combo, 42, L"", &user, slots));
    assert(selected.empty() && 2 == updates && 2 == invalidations && 0 == Message::alive);
}

static void __fastcall SetCaption(void* self, int, const char* text)
{
    assert(&g_combo == self);
    g_caption = text;
}

static void TestSelectedCaption()
{
    struct BottomBar
    {
        void*      unrelated;
        FakePanel* playerList;
    } bar{nullptr, &g_combo};
    void* table[]         = {nullptr, reinterpret_cast<void*>(SetCaption)};
    g_combo.vtable        = table;
    constexpr auto offset = offsetof(BottomBar, playerList);
    assert(SetUnicodeSpectatorCaption(&bar, L"\u4e2d\u6587\u6d4b\u8bd5 Eugene (100)", offset, 1));
    assert("\xe4\xb8\xad\xe6\x96\x87\xe6\xb5\x8b\xe8\xaf\x95 Eugene (100)" == g_caption);
    const auto before = g_caption;
    assert(!SetUnicodeSpectatorCaption(&bar, nullptr, offset, 1));
    assert(before == g_caption);
    assert(SetUnicodeSpectatorCaption(&bar, L"", offset, 1));
    assert(g_caption.empty());
}

int main()
{
    TestNativeDispatch();
    TestSelectedCaption();
    std::string output;
    assert(ConvertComboBoxItemToUTF8(L"  \u96f6\u70b9\u884c\u52a8", output));
    assert(14 == output.size());
    assert(0 == std::strcmp("  \xe9\x9b\xb6\xe7\x82\xb9\xe8\xa1\x8c\xe5\x8a\xa8", output.c_str()));

    assert(ConvertComboBoxItemToUTF8(L"ASCII", output));
    assert("ASCII" == output);
    assert(ConvertComboBoxItemToUTF8(L"", output));
    assert(output.empty() && '\0' == output.c_str()[0]);

    const std::wstring longText(512, L'\u96f6');
    assert(ConvertComboBoxItemToUTF8(longText.c_str(), output));
    const std::string expected = [] {
        std::string result;
        for (int i = 0; i < 512; ++i)
            result += "\xe9\x9b\xb6";
        return result;
    }();
    assert(expected == output);
    assert(expected.size() == output.size());

    assert(ConvertComboBoxItemToUTF8(L"\U0001f600", output));
    assert("\xf0\x9f\x98\x80" == output);
    assert(!ConvertComboBoxItemToUTF8(nullptr, output));
    assert(output.empty());
}
