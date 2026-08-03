#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include "config.h"

struct App {
    OpenKeySettings settings;
    AppIndicator* indicator{};
    GtkWidget* window{};
    GtkWidget* enabled{};
};

static void update_indicator(App* app) {
    bool enabled = g_settings_get_boolean(app->settings.get(), "enabled");
    app_indicator_set_icon_full(app->indicator, "input-keyboard", enabled ? "OpenKey: Vietnamese" : "OpenKey: English");
    app_indicator_set_label(app->indicator, enabled ? "VI" : "EN", "OpenKey");
    if (app->enabled) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->enabled), enabled);
}

static void set_enabled(GtkCheckMenuItem* item, gpointer data) {
    auto* app = static_cast<App*>(data);
    g_settings_set_boolean(app->settings.get(), "enabled", gtk_check_menu_item_get_active(item));
    update_indicator(app);
}

static void toggle_changed(GtkToggleButton* button, gpointer data) {
    auto* app = static_cast<App*>(data);
    g_settings_set_boolean(app->settings.get(), "enabled", gtk_toggle_button_get_active(button));
    update_indicator(app);
}

static void combo_changed(GtkComboBox* box, gpointer data) {
    const char* key = static_cast<const char*>(data);
    GSettings* settings = g_settings_new("org.openkey.Linux");
    g_settings_set_int(settings, key, gtk_combo_box_get_active(box));
    g_object_unref(settings);
}

static void custom_keys_changed(GtkEditable* entry, gpointer) {
    const char* keys = gtk_entry_get_text(GTK_ENTRY(entry));
    if (g_utf8_strlen(keys, -1) != 11) return;
    GSettings* settings = g_settings_new("org.openkey.Linux");
    g_settings_set_string(settings, "custom-input-keys", keys);
    g_object_unref(settings);
}

static GtkWidget* setting_switch(App* app, GtkWidget* parent, const char* label, const char* key) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget* text = gtk_label_new(label);
    GtkWidget* control = gtk_switch_new();
    gtk_widget_set_halign(text, GTK_ALIGN_START);
    gtk_widget_set_hexpand(text, TRUE);
    gtk_switch_set_active(GTK_SWITCH(control), g_settings_get_boolean(app->settings.get(), key));
    g_signal_connect(control, "state-set", G_CALLBACK(+[](GtkSwitch*, gboolean state, gpointer data) -> gboolean {
        GSettings* settings = g_settings_new("org.openkey.Linux");
        g_settings_set_boolean(settings, static_cast<const char*>(data), state);
        g_object_unref(settings);
        return FALSE;
    }), const_cast<char*>(key));
    gtk_box_pack_start(GTK_BOX(row), text, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(row), control, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(parent), row, FALSE, FALSE, 0);
    return control;
}

static void show_settings(GtkMenuItem*, gpointer data) {
    auto* app = static_cast<App*>(data);
    gtk_widget_show_all(app->window);
    gtk_window_present(GTK_WINDOW(app->window));
}

static void quit(GtkMenuItem*, gpointer) { gtk_main_quit(); }

static void activate(GtkApplication* application, gpointer data) {
    auto* app = static_cast<App*>(data);
    app->indicator = app_indicator_new("openkey", "input-keyboard", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    GtkWidget* menu = gtk_menu_new();
    GtkWidget* enabled = gtk_check_menu_item_new_with_label("Vietnamese typing");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(enabled), g_settings_get_boolean(app->settings.get(), "enabled"));
    g_signal_connect(enabled, "toggled", G_CALLBACK(set_enabled), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), enabled);
    GtkWidget* settings = gtk_menu_item_new_with_label("Settings…");
    g_signal_connect(settings, "activate", G_CALLBACK(show_settings), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings);
    GtkWidget* exit = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(exit, "activate", G_CALLBACK(quit), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), exit);
    gtk_widget_show_all(menu);
    app_indicator_set_menu(app->indicator, GTK_MENU(menu));

    app->window = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(app->window), "OpenKey Settings");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 480, 520);
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 20);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(app->window), box);
    GtkWidget* title = gtk_label_new("<b>OpenKey Vietnamese input</b>");
    gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    app->enabled = gtk_check_button_new_with_label("Enable Vietnamese typing");
    g_signal_connect(app->enabled, "toggled", G_CALLBACK(toggle_changed), app);
    gtk_box_pack_start(GTK_BOX(box), app->enabled, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Typing method"), FALSE, FALSE, 0);
    GtkWidget* method = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "Telex");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "VNI");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "Simple Telex 1");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "Simple Telex 2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "Custom");
    gtk_combo_box_set_active(GTK_COMBO_BOX(method), g_settings_get_int(app->settings.get(), "input-type"));
    g_signal_connect(method, "changed", G_CALLBACK(combo_changed), const_cast<char*>("input-type"));
    gtk_box_pack_start(GTK_BOX(box), method, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Custom keys: sắc, huyền, hỏi, ngã, nặng, â, ô, ê, ơ/ư, đ, bỏ dấu"), FALSE, FALSE, 0);
    GtkWidget* custom_keys = gtk_entry_new();
    gchar* saved_keys = g_settings_get_string(app->settings.get(), "custom-input-keys");
    gtk_entry_set_text(GTK_ENTRY(custom_keys), saved_keys);
    g_free(saved_keys);
    gtk_entry_set_max_length(GTK_ENTRY(custom_keys), 11);
    gtk_entry_set_placeholder_text(GTK_ENTRY(custom_keys), "sfrxjaoewdz");
    g_signal_connect(custom_keys, "changed", G_CALLBACK(custom_keys_changed), nullptr);
    gtk_box_pack_start(GTK_BOX(box), custom_keys, FALSE, FALSE, 0);
    setting_switch(app, box, "Spell check", "spell-check");
    setting_switch(app, box, "Modern orthography (oà, uý)", "modern-orthography");
    setting_switch(app, box, "Quick Telex", "quick-telex");
    setting_switch(app, box, "Restore invalid words", "restore-invalid");
    setting_switch(app, box, "Enable macros", "use-macro");
    setting_switch(app, box, "Auto-capitalize first character", "uppercase-first");
    update_indicator(app);
    gtk_widget_show_all(app->window);
}

int main(int argc, char** argv) {
    App app;
    GtkApplication* application = gtk_application_new("org.openkey.Linux.Control", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(application, "activate", G_CALLBACK(activate), &app);
    int status = g_application_run(G_APPLICATION(application), argc, argv);
    if (app.indicator) g_object_unref(app.indicator);
    g_object_unref(application);
    return status;
}
