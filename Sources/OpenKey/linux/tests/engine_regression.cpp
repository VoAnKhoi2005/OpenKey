#include "Engine.h"
#include <cstdio>

int main() {
    vLanguage = 1;
    vInputType = vTelex;
    vCodeTable = 0;
    vCheckSpelling = 1;
    auto* state = static_cast<vKeyHookState*>(vKeyInit());
    const Uint16 word[] = {KEY_D, KEY_U, KEY_N, KEY_G, KEY_F};
    for (Uint16 key : word) vKeyHandleEvent(Keyboard, KeyDown, key, 0, false);
    // The last F must be consumed and replace existing text; otherwise an IBus
    // frontend would leave "dungf" or duplicate the word in a terminal.
    if (state->code != vWillProcess || state->backspaceCount == 0 || state->newCharCount == 0) {
        std::fprintf(stderr, "Telex replacement regression: code=%d erase=%d output=%d\n", state->code, state->backspaceCount, state->newCharCount);
        return 1;
    }
    return 0;
}
