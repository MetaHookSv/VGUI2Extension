#pragma once

#include <Windows.h>
#include <string>
#include <optional>

//Native VGUI converts menu captions to UTF-8, despite calling the operation ANSI.
//Size in bytes including the terminator, not in UTF-16 code units.
inline bool ConvertComboBoxItemToUTF8(const wchar_t* text, std::string& output)
{
    output.clear();
    if (!text)
        return false;

    const int bytes = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (bytes <= 0)
        return false;

    output.resize(bytes);
    if (WideCharToMultiByte(CP_UTF8, 0, text, -1, output.data(), bytes, nullptr, nullptr) != bytes)
    {
        output.clear();
        return false;
    }
    //The conversion writes NUL; exclude it from the string's logical length.
    output.resize(bytes - 1);
    return true;
}

//All slots belong to the module that owns the receiving native objects.
struct ComboBoxUnicodeSlots
{
    int getMenu;
    int addMenuItem;
    int isItemIDValid;
    int updateMenuItemW;
    int invalidateLayout;
};

inline bool SetUnicodeSpectatorCaption(void* bottomBar, const wchar_t* text, size_t playerListOffset, int setTextSlot)
{
    std::string caption;
    if (!ConvertComboBoxItemToUTF8(text, caption))
        return false;

    auto       combo   = *reinterpret_cast<void**>(static_cast<unsigned char*>(bottomBar) + playerListOffset);
    const auto table   = *static_cast<void***>(combo);
    const auto setText = reinterpret_cast<void(__fastcall*)(void*, int, const char*)>(table[setTextSlot]);
    setText(combo, 0, caption.c_str());
    return true;
}

template <typename Message>
std::optional<int> AddUnicodeComboBoxItem(void* combo, const wchar_t* text, const void* userData, const ComboBoxUnicodeSlots& slots)
{
    std::string caption;
    if (!ConvertComboBoxItemToUTF8(text, caption))
        return std::nullopt;

    const auto comboTable = *static_cast<void***>(combo);
    const auto getMenu    = reinterpret_cast<void*(__fastcall*)(void*, int)>(comboTable[slots.getMenu]);
    auto       menu       = getMenu(combo, 0);
    const auto menuTable  = *static_cast<void***>(menu);
    const auto addItem    = reinterpret_cast<int(__fastcall*)(void*, int, const char*, Message*, void*, const void*)>(menuTable[slots.addMenuItem]);
    //Menu takes ownership. The selected caption remains a WSTRING.
    return addItem(menu, 0, caption.c_str(), new Message("SetText", "text", text), combo, userData);
}

template <typename Message>
bool UpdateUnicodeComboBoxItem(void* combo, int itemID, const wchar_t* text, const void* userData, const ComboBoxUnicodeSlots& slots)
{
    const auto comboTable = *static_cast<void***>(combo);
    const auto isValid    = reinterpret_cast<bool(__fastcall*)(void*, int, int)>(comboTable[slots.isItemIDValid]);
    if (!isValid(combo, 0, itemID) || !text)
        return false;

    const auto getMenu   = reinterpret_cast<void*(__fastcall*)(void*, int)>(comboTable[slots.getMenu]);
    auto       menu      = getMenu(combo, 0);
    const auto menuTable = *static_cast<void***>(menu);
    const auto update    = reinterpret_cast<void(__fastcall*)(void*, int, int, const wchar_t*, Message*, const void*)>(menuTable[slots.updateMenuItemW]);
    //The native wide menu overload needs no UTF-8 round trip for either caption.
    update(menu, 0, itemID, text, new Message("SetText", "text", text), userData);
    const auto invalidate = reinterpret_cast<void(__fastcall*)(void*, int, bool, bool)>(comboTable[slots.invalidateLayout]);
    invalidate(combo, 0, false, false);
    return true;
}
