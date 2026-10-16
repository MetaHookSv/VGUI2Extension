#include "../src/CareerPlayerName.h"

#include <cassert>
#include <string>

int main()
{
    std::wstring output;
    assert(ConvertCareerPlayerNameToUnicode("Ab\xe4\xb8\xad\xe6\x96\x87Z", output));
    assert(L"Ab\u4e2d\u6587Z" == output);
    assert(ConvertCareerPlayerNameToUnicode("ASCII", output));
    assert(L"ASCII" == output);
    assert(ConvertCareerPlayerNameToUnicode("", output));
    assert(output.empty());
    assert(ConvertCareerPlayerNameToUnicode("\xf0\x9f\x98\x80", output));
    assert(L"\U0001f600" == output);

    //The native frame cuts this three-byte character after its first byte.
    const std::string boundaryName = std::string(30, 'A') + "\xe4\xb8\xadZ";
    assert(BuildCareerPlayerNameText(boundaryName.c_str(), L"\u56f0\u96be", output));
    assert(std::wstring(30, L'A') + L"\u4e2dZ / \u56f0\u96be" == output);
    assert(BuildCareerPlayerNameText("A / B%hs", L"Expert", output));
    assert(L"A / B%hs / Expert" == output);

    const std::string longName(1024, 'X');
    assert(BuildCareerPlayerNameText(longName.c_str(), L"Normal", output));
    assert(std::wstring(1024, L'X') + L" / Normal" == output);
    assert(!ConvertCareerPlayerNameToUnicode("\xe4\xb8", output));
    assert(output.empty());
    assert(!ConvertCareerPlayerNameToUnicode(nullptr, output));
    assert(output.empty());
    assert(!BuildCareerPlayerNameText("name", nullptr, output));
    assert(output.empty());
}
