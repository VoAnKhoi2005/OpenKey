#include "config.h"
#include "DataType.h"

Uint16 vCustomInputKeys[11] = {KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z};

static Uint16 custom_key(gchar key) {
    switch (g_ascii_tolower(key)) {
        case 'a': return KEY_A; case 'b': return KEY_B; case 'c': return KEY_C; case 'd': return KEY_D; case 'e': return KEY_E;
        case 'f': return KEY_F; case 'g': return KEY_G; case 'h': return KEY_H; case 'i': return KEY_I; case 'j': return KEY_J;
        case 'k': return KEY_K; case 'l': return KEY_L; case 'm': return KEY_M; case 'n': return KEY_N; case 'o': return KEY_O;
        case 'p': return KEY_P; case 'q': return KEY_Q; case 'r': return KEY_R; case 's': return KEY_S; case 't': return KEY_T;
        case 'u': return KEY_U; case 'v': return KEY_V; case 'w': return KEY_W; case 'x': return KEY_X; case 'y': return KEY_Y; case 'z': return KEY_Z;
        case '0': return KEY_0; case '1': return KEY_1; case '2': return KEY_2; case '3': return KEY_3; case '4': return KEY_4;
        case '5': return KEY_5; case '6': return KEY_6; case '7': return KEY_7; case '8': return KEY_8; case '9': return KEY_9;
        default: return KEY_EMPTY;
    }
}

int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 1;
int vQuickTelex = 0;
int vSwitchKeyStatus = 0;
int vRestoreIfWrongSpelling = 0;
int vFixRecommendBrowser = 0;
int vUseMacro = 1;
int vUseMacroInEnglishMode = 0;
int vAutoCapsMacro = 0;
int vUseSmartSwitchKey = 0;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 0;
int vOtherLanguage = 0;
int vTempOffOpenKey = 0;

OpenKeySettings::OpenKeySettings() : settings_(g_settings_new("org.openkey.Linux")) {
    load();
    g_signal_connect(settings_, "changed", G_CALLBACK(changed), this);
}

OpenKeySettings::~OpenKeySettings() { g_object_unref(settings_); }

void OpenKeySettings::changed(GSettings*, gchar*, gpointer self) {
    static_cast<OpenKeySettings*>(self)->load();
}

void OpenKeySettings::load() {
    vLanguage = g_settings_get_boolean(settings_, "enabled");
    vInputType = g_settings_get_int(settings_, "input-type");
    gchar* custom = g_settings_get_string(settings_, "custom-input-keys");
    if (g_utf8_strlen(custom, -1) == 11) {
        const gchar* p = custom;
        for (int i = 0; i < 11; ++i, p = g_utf8_next_char(p)) {
            Uint16 key = custom_key(*p);
            if (key != KEY_EMPTY) vCustomInputKeys[i] = key;
        }
    }
    g_free(custom);
    vCodeTable = g_settings_get_int(settings_, "code-table");
    vCheckSpelling = g_settings_get_boolean(settings_, "spell-check");
    vUseModernOrthography = g_settings_get_boolean(settings_, "modern-orthography");
    vQuickTelex = g_settings_get_boolean(settings_, "quick-telex");
    vRestoreIfWrongSpelling = g_settings_get_boolean(settings_, "restore-invalid");
    vUseMacro = g_settings_get_boolean(settings_, "use-macro");
    vUseMacroInEnglishMode = g_settings_get_boolean(settings_, "macro-in-english");
    vAutoCapsMacro = g_settings_get_boolean(settings_, "auto-caps-macro");
    vUpperCaseFirstChar = g_settings_get_boolean(settings_, "uppercase-first");
    vAllowConsonantZFWJ = g_settings_get_boolean(settings_, "allow-zfwj");
    vQuickStartConsonant = g_settings_get_boolean(settings_, "quick-start-consonant");
    vQuickEndConsonant = g_settings_get_boolean(settings_, "quick-end-consonant");
}
