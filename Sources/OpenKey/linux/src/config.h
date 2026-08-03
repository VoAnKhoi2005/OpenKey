#pragma once

#include <gio/gio.h>

// The portable OpenKey engine declares these values.  The Linux frontend owns
// them and keeps them synchronized with GSettings.
extern int vLanguage;
extern int vInputType;
extern int vFreeMark;
extern int vCodeTable;
extern int vCheckSpelling;
extern int vUseModernOrthography;
extern int vQuickTelex;
extern int vSwitchKeyStatus;
extern int vRestoreIfWrongSpelling;
extern int vFixRecommendBrowser;
extern int vUseMacro;
extern int vUseMacroInEnglishMode;
extern int vAutoCapsMacro;
extern int vUseSmartSwitchKey;
extern int vUpperCaseFirstChar;
extern int vTempOffSpelling;
extern int vAllowConsonantZFWJ;
extern int vQuickStartConsonant;
extern int vQuickEndConsonant;
extern int vRememberCode;
extern int vOtherLanguage;
extern int vTempOffOpenKey;

class OpenKeySettings {
public:
    OpenKeySettings();
    ~OpenKeySettings();
    GSettings* get() const { return settings_; }
    void load();
private:
    static void changed(GSettings*, gchar*, gpointer);
    GSettings* settings_;
};
