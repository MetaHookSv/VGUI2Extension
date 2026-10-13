#include "../src/ComboBoxUnicode.h"

#include <cassert>
#include <cstring>
#include <string>

int main()
{
    std::vector<char> output;
    assert(ConvertComboBoxItemToUTF8(L"  \u96f6\u70b9\u884c\u52a8", output));
    assert(15 == output.size());
    assert(0 == std::strcmp("  \xe9\x9b\xb6\xe7\x82\xb9\xe8\xa1\x8c\xe5\x8a\xa8", output.data()));

    assert(ConvertComboBoxItemToUTF8(L"ASCII", output));
    assert(0 == std::strcmp("ASCII", output.data()));
    assert(ConvertComboBoxItemToUTF8(L"", output));
    assert(1 == output.size() && '\0' == output[0]);

    const std::wstring longText(512, L'\u96f6');
    assert(ConvertComboBoxItemToUTF8(longText.c_str(), output));
    const std::string expected = [] {
        std::string result;
        for (int i = 0; i < 512; ++i)
            result += "\xe9\x9b\xb6";
        return result;
    }();
    assert(expected == output.data());
    assert(expected.size() + 1 == output.size());

    assert(ConvertComboBoxItemToUTF8(L"\U0001f600", output));
    assert(0 == std::strcmp("\xf0\x9f\x98\x80", output.data()));
    assert(!ConvertComboBoxItemToUTF8(nullptr, output));
    assert(output.empty());
}
