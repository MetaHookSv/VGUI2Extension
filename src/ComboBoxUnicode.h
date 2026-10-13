#pragma once

#include <Windows.h>
#include <vector>

//Native GameUI converts menu captions to UTF-8, despite calling the operation ANSI.
//Size in bytes including the terminator, not in UTF-16 code units.
inline bool ConvertComboBoxItemToUTF8(const wchar_t* text, std::vector<char>& output)
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
    return true;
}
