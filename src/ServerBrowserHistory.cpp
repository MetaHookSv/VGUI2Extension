#include <metahook.h>
#include <IBaseKeyValues.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <locale.h>

#include "plugins.h"
#include "privatefuncs.h"
#include "ServerBrowserHistory.h"

namespace
{
// Public GoldSrc IBaseKeyValues ABI (MetaHook SDK Interface/IBaseKeyValues.h).
// Only the table address is module-private; never use the plugin's KeyValues table.
constexpr int     kSetStringSlot      = 17;
constexpr size_t  kDateBufferCapacity = 64;
constexpr char    kDateFormat[]       = "%a %d %b %I:%M%p";
constexpr wchar_t kWideDateFormat[]   = L"%a %d %b %I:%M%p";

constexpr std::array<const char*, 2> kDateCallsites = {
    "CBaseGamesPage_ServerResponded_to_strftime_callsite_0",
    "CBaseGamesPage_ApplyGameFilters_to_strftime_callsite_0",
};

using StrftimeFn  = size_t(__cdecl*)(char*, size_t, const char*, const std::tm*);
using SetStringFn = void(__fastcall*)(IBaseKeyValues*, int, const char*, const char*);

struct PendingDate
{
    const char*                              buffer     = nullptr;
    unsigned int                             generation = 0;
    std::array<wchar_t, kDateBufferCapacity> text       = {};
};

thread_local PendingDate                      s_PendingDate;
std::atomic<unsigned int>                     s_Generation{0};
std::array<hook_t*, kDateCallsites.size()>    s_DateHooks         = {};
std::array<StrftimeFn, kDateCallsites.size()> s_OriginalStrftime  = {};
hook_t*                                       s_SetStringHook     = nullptr;
SetStringFn                                   s_OriginalSetString = nullptr;
_locale_t                                     s_TimeLocale        = nullptr;
bool                                          s_Installed         = false;

template <size_t Index>
size_t __cdecl HistoryStrftime(char* buffer, size_t capacity, const char* format, const std::tm* time)
{
    // Invalidate even on failure, so a reused stack address cannot inherit the
    // previous row's date. The tm storage may belong to the native CRT.
    s_PendingDate                 = {};
    const unsigned int generation = s_Generation.load(std::memory_order_relaxed);
    const std::tm      savedTime  = time ? *time : std::tm{};

    // Preserve both the native bytes and return value. Native _strlwr / tolower
    // still operates on this narrow buffer; it never sees our wide copy.
    const size_t result      = s_OriginalStrftime[Index](buffer, capacity, format, time);
    const int    nativeErrno = errno;

    PendingDate pending;
    if (result && result < capacity && buffer && time && format &&
        !std::strcmp(format, kDateFormat) && s_TimeLocale)
    {
        const size_t length = _wcsftime_l(pending.text.data(), pending.text.size(), kWideDateFormat, &savedTime, s_TimeLocale);
        if (length && length < pending.text.size())
        {
            // Preserve the native English am/pm presentation without applying
            // a fixed byte-tail lowercase operation to a multibyte string.
            if (length >= 2 && pending.text[length - 1] == L'M' &&
                (pending.text[length - 2] == L'A' || pending.text[length - 2] == L'P'))
            {
                pending.text[length - 2] += L'a' - L'A';
                pending.text[length - 1] = L'm';
            }
            pending.buffer     = buffer;
            pending.generation = generation;
        }
    }
    s_PendingDate = pending;
    errno         = nativeErrno;
    return result;
}

void __fastcall HistorySetString(IBaseKeyValues* self, int dummy, const char* keyName, const char* value)
{
    if (keyName && !std::strcmp(keyName, "LastPlayed"))
    {
        const PendingDate pending = s_PendingDate;
        // Consume before dispatching: SetWString may reenter other UI hooks.
        s_PendingDate = {};
        if (value && pending.buffer == value && pending.text[0] &&
            pending.generation == s_Generation.load(std::memory_order_relaxed))
        {
            self->SetWString(keyName, pending.text.data());
            return;
        }
    }

    s_OriginalSetString(self, dummy, keyName, value);
}
} // namespace

void ServerBrowserHistory_InstallHooks()
{
    if (s_Installed)
        return;

    const auto moduleBase = g_ServerBrowserDllInfo.ImageBase;
    auto       table      = static_cast<void**>(GamedataResolveIfAvailable(moduleBase, "serverbrowser",
                                                                           {"KeyValues", "vgui2::KeyValues"}, MH_GAMESYMBOL_KIND_VTABLE));
    if (!table)
    {
        Sys_Error("Could not resolve the ServerBrowser KeyValues vtable.\nEngine buildnum: %d", g_dwEngineBuildnum);
        return;
    }

    std::array<void*, kDateCallsites.size()> sites = {
        GamedataResolvePtr(moduleBase, "serverbrowser", kDateCallsites[0], MH_GAMESYMBOL_KIND_PATCH),
        // Older filter loops never write LastPlayed. Their catalogs deliberately
        // omit this callsite; the response callsite remains required.
        GamedataResolvePtrIfAvailable(moduleBase, "serverbrowser", kDateCallsites[1], MH_GAMESYMBOL_KIND_PATCH),
    };
    if (!sites[0])
        return;

    // ServerBrowser's VGui_InitInterfacesList uses setlocale(LC_TIME, "").
    // Its CRT can be statically linked, so use an explicit equivalent locale
    // rather than the plugin CRT's global locale or a hardcoded ANSI code page.
    s_TimeLocale = _create_locale(LC_TIME, "");
    if (!s_TimeLocale)
    {
        Sys_Error("Could not create the ServerBrowser history time locale.\nEngine buildnum: %d", g_dwEngineBuildnum);
        return;
    }

    s_Generation.fetch_add(1, std::memory_order_relaxed);
    s_PendingDate   = {};
    s_SetStringHook = g_pMetaHookAPI->VFTHookEx(table, kSetStringSlot, reinterpret_cast<void*>(HistorySetString),
                                                reinterpret_cast<void**>(&s_OriginalSetString));
    if (!s_SetStringHook)
    {
        ServerBrowserHistory_UninstallHooks();
        Sys_Error("Could not hook ServerBrowser KeyValues::SetString.\nEngine buildnum: %d", g_dwEngineBuildnum);
        return;
    }

    const std::array<StrftimeFn, kDateCallsites.size()> replacements = {HistoryStrftime<0>, HistoryStrftime<1>};
    for (size_t i = 0; i < sites.size(); ++i)
    {
        if (!sites[i])
            continue;

        s_DateHooks[i] = g_pMetaHookAPI->InlinePatchRedirectBranch(sites[i], reinterpret_cast<void*>(replacements[i]),
                                                                   reinterpret_cast<void**>(&s_OriginalStrftime[i]));
        if (!s_DateHooks[i])
        {
            ServerBrowserHistory_UninstallHooks();
            Sys_Error("Could not redirect gamedata patch: %s (module serverbrowser)\nEngine buildnum: %d", kDateCallsites[i], g_dwEngineBuildnum);
            return;
        }
    }
    s_Installed = true;
}

void ServerBrowserHistory_UninstallHooks()
{
    // Stop capturing before restoring the writer. UnHook restores the original
    // call instruction bytes, including an import-indirect CALL's sixth byte.
    for (size_t i = s_DateHooks.size(); i > 0; --i)
    {
        if (s_DateHooks[i - 1])
        {
            g_pMetaHookAPI->UnHook(s_DateHooks[i - 1]);
            s_DateHooks[i - 1] = nullptr;
        }
    }
    if (s_SetStringHook)
    {
        g_pMetaHookAPI->UnHook(s_SetStringHook);
        s_SetStringHook = nullptr;
    }

    s_Generation.fetch_add(1, std::memory_order_relaxed);
    s_PendingDate       = {};
    s_OriginalStrftime  = {};
    s_OriginalSetString = nullptr;
    if (s_TimeLocale)
    {
        _free_locale(s_TimeLocale);
        s_TimeLocale = nullptr;
    }
    s_Installed = false;
}
