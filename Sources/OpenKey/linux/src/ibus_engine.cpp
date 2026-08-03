#include <ibus.h>
#include <string>
#include <vector>
#include "Engine.h"
#include "config.h"

static void reload_macros(GSettings* settings) {
    initMacroMap(nullptr, 0);
    gchar** entries = g_settings_get_strv(settings, "macros");
    for (gchar** entry = entries; entry && *entry; ++entry) {
        gchar** pair = g_strsplit(*entry, "\t", 2);
        if (pair[0] && pair[1] && *pair[0]) addMacro(pair[0], pair[1]);
        g_strfreev(pair);
    }
    g_strfreev(entries);
}

static void settings_changed(GSettings* settings, gchar* key, gpointer) {
    if (g_str_equal(key, "macros")) reload_macros(settings);
}

typedef struct _OpenKeyEngine {
    IBusEngine parent;
    vKeyHookState* hook;
    std::string preedit;
} OpenKeyEngine;
typedef struct _OpenKeyEngineClass { IBusEngineClass parent; } OpenKeyEngineClass;

G_DEFINE_TYPE(OpenKeyEngine, openkey_engine, IBUS_TYPE_ENGINE)

static guint16 key_for_ascii(gunichar c) {
    switch (g_ascii_tolower(c)) {
        case 'a': return KEY_A; case 'b': return KEY_B; case 'c': return KEY_C; case 'd': return KEY_D;
        case 'e': return KEY_E; case 'f': return KEY_F; case 'g': return KEY_G; case 'h': return KEY_H;
        case 'i': return KEY_I; case 'j': return KEY_J; case 'k': return KEY_K; case 'l': return KEY_L;
        case 'm': return KEY_M; case 'n': return KEY_N; case 'o': return KEY_O; case 'p': return KEY_P;
        case 'q': return KEY_Q; case 'r': return KEY_R; case 's': return KEY_S; case 't': return KEY_T;
        case 'u': return KEY_U; case 'v': return KEY_V; case 'w': return KEY_W; case 'x': return KEY_X;
        case 'y': return KEY_Y; case 'z': return KEY_Z;
        case '0': return KEY_0; case '1': return KEY_1; case '2': return KEY_2; case '3': return KEY_3;
        case '4': return KEY_4; case '5': return KEY_5; case '6': return KEY_6; case '7': return KEY_7;
        case '8': return KEY_8; case '9': return KEY_9;
        case '[': return KEY_LEFT_BRACKET; case ']': return KEY_RIGHT_BRACKET; case '.': return KEY_DOT;
        case '`': return KEY_BACKQUOTE; case '-': return KEY_MINUS; case '=': return KEY_EQUALS;
        case '\\': return KEY_BACK_SLASH; case ';': return KEY_SEMICOLON; case '\'': return KEY_QUOTE;
        case ',': return KEY_COMMA; case '/': return KEY_SLASH; case ' ': return KEY_SPACE;
        default: return KEY_EMPTY;
    }
}

static guint16 key_for_keyval(guint keyval) {
    if (keyval == IBUS_KEY_BackSpace) return KEY_DELETE;
    if (keyval == IBUS_KEY_Return) return KEY_RETURN;
    if (keyval == IBUS_KEY_Tab) return KEY_TAB;
    if (keyval == IBUS_KEY_Escape) return KEY_ESC;
    if (keyval == IBUS_KEY_Left) return KEY_LEFT;
    if (keyval == IBUS_KEY_Right) return KEY_RIGHT;
    if (keyval == IBUS_KEY_Up) return KEY_UP;
    if (keyval == IBUS_KEY_Down) return KEY_DOWN;
    return key_for_ascii(ibus_keyval_to_unicode(keyval));
}

static gunichar output_character(guint32 data) {
    guint32 value = getCharacterCode(data);
    if (value & CHAR_CODE_MASK) return value & CHAR_MASK;
    guint16 key = value & CHAR_MASK;
    // Convert the engine's physical Linux key codes back to their US symbols.
    for (char c = 'a'; c <= 'z'; ++c) if (key_for_ascii(c) == key) return (value & CAPS_MASK) ? g_ascii_toupper(c) : c;
    for (char c = '0'; c <= '9'; ++c) if (key_for_ascii(c) == key) return c;
    const char symbols[] = "`-=[]\\;',./ ";
    for (const char* p = symbols; *p; ++p) if (key_for_ascii(*p) == key) return *p;
    return 0;
}

static void update_preedit(OpenKeyEngine* engine) {
    IBusText* text = ibus_text_new_from_string(engine->preedit.c_str());
    const guint cursor = static_cast<guint>(g_utf8_strlen(engine->preedit.c_str(), -1));
    ibus_engine_update_preedit_text(IBUS_ENGINE(engine), text, cursor, !engine->preedit.empty());
}

static void commit_preedit(OpenKeyEngine* engine) {
    if (!engine->preedit.empty()) {
        IBusText* text = ibus_text_new_from_string(engine->preedit.c_str());
        ibus_engine_commit_text(IBUS_ENGINE(engine), text);
        engine->preedit.clear();
    }
    update_preedit(engine);
}

static void remove_preedit_characters(std::string& text, guint count) {
    while (count-- && !text.empty()) {
        gchar* start = g_utf8_find_prev_char(text.c_str(), text.c_str() + text.size());
        if (!start) {
            text.clear();
            return;
        }
        text.erase(static_cast<std::string::size_type>(start - text.c_str()));
    }
}

static bool is_composition_character(guint keyval) {
    gunichar character = ibus_keyval_to_unicode(keyval);
    return g_unichar_isalnum(character) || character == '-' || character == '[' || character == ']' ||
           character == '\\' || character == ';' || character == '\'' || character == ',' || character == '.' ||
           character == '/' || character == '`' || character == '=';
}

static gboolean process_key_event(IBusEngine* base, guint keyval, guint, guint state) {
    auto* engine = reinterpret_cast<OpenKeyEngine*>(base);
    if (state & IBUS_RELEASE_MASK || state & (IBUS_CONTROL_MASK | IBUS_MOD1_MASK | IBUS_SUPER_MASK)) return FALSE;

    // Do not leave uncommitted text behind when the user moves the cursor or
    // performs an action outside of normal text composition.
    if (keyval == IBUS_KEY_Left || keyval == IBUS_KEY_Right || keyval == IBUS_KEY_Up ||
        keyval == IBUS_KEY_Down || keyval == IBUS_KEY_Home || keyval == IBUS_KEY_End ||
        keyval == IBUS_KEY_Page_Up || keyval == IBUS_KEY_Page_Down || keyval == IBUS_KEY_Escape) {
        commit_preedit(engine);
        startNewSession();
        return FALSE;
    }
    guint16 key = key_for_keyval(keyval);
    if (key == KEY_EMPTY) return FALSE;

    if (!vLanguage) {
        if (vUseMacro && vUseMacroInEnglishMode) vEnglishMode(KeyDown, key, (state & IBUS_SHIFT_MASK) != 0, false);
        else return FALSE;
    } else {
        vKeyHandleEvent(Keyboard, KeyDown, key, (state & IBUS_SHIFT_MASK) ? 1 : 0, false);
    }

    vKeyHookState* hook = engine->hook;
    if (keyval == IBUS_KEY_BackSpace) {
        if (engine->preedit.empty()) return FALSE;
        remove_preedit_characters(engine->preedit, 1);
        update_preedit(engine);
        return TRUE;
    }

    if (hook->code == vWillProcess || hook->code == vRestore || hook->code == vRestoreAndStartNewSession || hook->code == vReplaceMaro) {
        // Keep the active word in IBus preedit.  Asking applications to delete
        // surrounding text is optional in IBus and terminals commonly decline
        // it, which used to yield duplicated text such as "dungùng".
        remove_preedit_characters(engine->preedit, hook->backspaceCount);
        GString* output = g_string_new(nullptr);
        const std::vector<Uint32>& characters = hook->code == vReplaceMaro ? hook->macroData : std::vector<Uint32>();
        if (hook->code == vReplaceMaro) {
            for (Uint32 c : characters) g_string_append_unichar(output, output_character(c));
        } else {
            for (int i = hook->newCharCount - 1; i >= 0; --i) {
                gunichar c = output_character(hook->charData[i]);
                if (c) g_string_append_unichar(output, c);
            }
        }
        if (output->len) engine->preedit += output->str;
        g_string_free(output, TRUE);
        if (hook->code == vRestoreAndStartNewSession) startNewSession();

        // A macro is normally expanded when its trailing separator (such as a
        // space) arrives. Commit the expanded word, then let that separator pass
        // through to the application normally.
        if (!is_composition_character(keyval)) {
            commit_preedit(engine);
            return FALSE;
        }
        update_preedit(engine);
        return TRUE;
    }

    if (hook->code == vDoNothing && is_composition_character(keyval)) {
        gunichar character = ibus_keyval_to_unicode(keyval);
        gchar encoded[7] = {0};
        const gint length = g_unichar_to_utf8(character, encoded);
        engine->preedit.append(encoded, length);
        update_preedit(engine);
        return TRUE;
    }

    // Word separators are committed by the client application itself after the
    // current preedit word is committed here.
    commit_preedit(engine);
    return FALSE;
}

static void openkey_engine_init(OpenKeyEngine* engine) { engine->hook = static_cast<vKeyHookState*>(vKeyInit()); }
static void openkey_engine_enable(IBusEngine* base) {
    auto* engine = reinterpret_cast<OpenKeyEngine*>(base);
    engine->preedit.clear();
    update_preedit(engine);
    vLanguage = 1;
    startNewSession();
}
static void openkey_engine_disable(IBusEngine* base) {
    auto* engine = reinterpret_cast<OpenKeyEngine*>(base);
    commit_preedit(engine);
    vLanguage = 0;
    startNewSession();
}
static void openkey_engine_class_init(OpenKeyEngineClass* klass) {
    IBUS_ENGINE_CLASS(klass)->process_key_event = process_key_event;
    IBUS_ENGINE_CLASS(klass)->enable = openkey_engine_enable;
    IBUS_ENGINE_CLASS(klass)->disable = openkey_engine_disable;
}

int main() {
    ibus_init();
    OpenKeySettings settings;
    reload_macros(settings.get());
    g_signal_connect(settings.get(), "changed", G_CALLBACK(settings_changed), nullptr);
    IBusBus* bus = ibus_bus_new();
    IBusFactory* factory = ibus_factory_new(ibus_bus_get_connection(bus));
    ibus_factory_add_engine(factory, "openkey", openkey_engine_get_type());
    ibus_bus_request_name(bus, "org.openkey.Linux", 0);
    g_signal_connect(bus, "disconnected", G_CALLBACK(ibus_quit), nullptr);
    ibus_main();
    g_object_unref(factory);
    g_object_unref(bus);
    return 0;
}
