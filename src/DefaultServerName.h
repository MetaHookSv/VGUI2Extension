#pragma once

#include <Windows.h>
#include <string>

inline bool BuildDefaultServerName(const char* personaName, std::string& output)
{
    output.clear();
    if (!personaName || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, personaName, -1, nullptr, 0) <= 0)
        return false;

    //OnOK copies the hostname into char[64]. Reserve its NUL and the suffix.
    constexpr size_t kHostnameCapacity = 64;
    constexpr char   kSuffix[]         = "'s Server";
    constexpr size_t kPersonaBytes     = kHostnameCapacity - sizeof(kSuffix);
    output                             = personaName;
    if (output.size() > kPersonaBytes)
    {
        size_t end = kPersonaBytes;
        while ((static_cast<unsigned char>(output[end]) & 0xC0) == 0x80)
            --end;
        output.resize(end);
    }
    while (!output.empty() && output.back() == ' ')
        output.pop_back();
    if (output.empty())
        output = "Someone";
    output += kSuffix;
    return true;
}
