#include "config.h"

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
