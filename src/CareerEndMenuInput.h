#pragma once

#include <cstring>

//ESC only switches GameUI. Keep the pending Career choice and music state
//intact when the completed match sends MATCH again.
template <typename Handler, typename KeyCode>
void ForwardCareerEndMenuKey(Handler original, void* self, int dummy, KeyCode key, KeyCode escapeKey)
{
    if (key != escapeKey)
        original(self, dummy, key);
}

template <typename Handler>
void ForwardCareerEndMenuCommand(Handler original, void* self, int dummy, const char* command)
{
    //Child dialogs can forward the same generic close commands as Frame.
    if (command && (!_stricmp(command, "Close") || !_stricmp(command, "Cancel")))
        return;

    original(self, dummy, command);
}
