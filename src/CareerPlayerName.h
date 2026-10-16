#pragma once

#include <Windows.h>
#include <string>

inline bool ConvertCareerPlayerNameToUnicode(const char* name, std::wstring& output)
{
    output.clear();
    if (!name)
        return false;

    //Names are UTF-8, independently of the CRT locale used by native %hs.
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, nullptr, 0);
    if (count <= 0)
        return false;

    output.resize(count);
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, output.data(), count) != count)
    {
        output.clear();
        return false;
    }
    output.resize(count - 1);
    return true;
}

inline bool BuildCareerPlayerNameText(const char* name, const wchar_t* difficulty, std::wstring& output)
{
    output.clear();
    if (!difficulty || !ConvertCareerPlayerNameToUnicode(name, output))
        return false;

    //Read the full cvar, without the native frame's 31-byte truncation.
    output += L" / ";
    output += difficulty;
    return true;
}
