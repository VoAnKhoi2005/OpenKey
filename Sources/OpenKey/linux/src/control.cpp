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
    app_indicator_set_icon_full(app->indicator, "input-keyboard", enabled ? "OpenKey: Tiếng Việt" : "OpenKey: Tiếng Anh");
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
    GtkWidget* enabled = gtk_check_menu_item_new_with_label("Bật tiếng Việt");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(enabled), g_settings_get_boolean(app->settings.get(), "enabled"));
    g_signal_connect(enabled, "toggled", G_CALLBACK(set_enabled), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), enabled);
    GtkWidget* settings = gtk_menu_item_new_with_label("Bảng điều khiển…");
    g_signal_connect(settings, "activate", G_CALLBACK(show_settings), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings);
    GtkWidget* exit = gtk_menu_item_new_with_label("Thoát");
    g_signal_connect(exit, "activate", G_CALLBACK(quit), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), exit);
    gtk_widget_show_all(menu);
    app_indicator_set_menu(app->indicator, GTK_MENU(menu));

    app->window = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(app->window), "Bảng điều khiển OpenKey");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 480, 520);
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 20);
    GtkWidget* tabs = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(app->window), tabs);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(box), 16);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), box, gtk_label_new("Kiểu gõ"));
    GtkWidget* title = gtk_label_new("<b>OpenKey – Bộ gõ tiếng Việt</b>");
    gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    app->enabled = gtk_check_button_new_with_label("Bật gõ tiếng Việt");
    g_signal_connect(app->enabled, "toggled", G_CALLBACK(toggle_changed), app);
    gtk_box_pack_start(GTK_BOX(box), app->enabled, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Kiểu gõ"), FALSE, FALSE, 0);
    GtkWidget* method = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "Telex");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "VNI");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "Simple Telex 1");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "Simple Telex 2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(method), "Tự định nghĩa");
    gtk_combo_box_set_active(GTK_COMBO_BOX(method), g_settings_get_int(app->settings.get(), "input-type"));
    g_signal_connect(method, "changed", G_CALLBACK(combo_changed), const_cast<char*>("input-type"));
    gtk_box_pack_start(GTK_BOX(box), method, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Phím tự định nghĩa: sắc, huyền, hỏi, ngã, nặng, â, ô, ê, ơ/ư, đ, bỏ dấu"), FALSE, FALSE, 0);
    GtkWidget* custom_keys = gtk_entry_new();
    gchar* saved_keys = g_settings_get_string(app->settings.get(), "custom-input-keys");
    gtk_entry_set_text(GTK_ENTRY(custom_keys), saved_keys);
    g_free(saved_keys);
    gtk_entry_set_max_length(GTK_ENTRY(custom_keys), 11);
    gtk_entry_set_placeholder_text(GTK_ENTRY(custom_keys), "sfrxjaoewdz");
    g_signal_connect(custom_keys, "changed", G_CALLBACK(custom_keys_changed), nullptr);
    gtk_box_pack_start(GTK_BOX(box), custom_keys, FALSE, FALSE, 0);
    setting_switch(app, box, "Kiểm tra chính tả", "spell-check");
    setting_switch(app, box, "Dấu kiểu mới (oà, uý)", "modern-orthography");
    setting_switch(app, box, "Gõ tắt Telex", "quick-telex");
    setting_switch(app, box, "Khôi phục từ sai", "restore-invalid");
    setting_switch(app, box, "Bật gõ tắt", "use-macro");
    setting_switch(app, box, "Viết hoa đầu câu", "uppercase-first");
    GtkWidget* macros = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(macros), 16);
    gtk_box_pack_start(GTK_BOX(macros), gtk_label_new("Macros"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(macros), gtk_label_new("Macro expansion is enabled from the Typing tab. A full macro list editor will be added here."), FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), macros, gtk_label_new("Macros"));
    GtkWidget* system = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(system), 16);
    gtk_box_pack_start(GTK_BOX(system), gtk_label_new("System integration"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(system), gtk_label_new("Use the VI / EN indicator in the top bar to change modes. Add OpenKey to Startup Applications to launch it on login."), FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), system, gtk_label_new("System"));
    GtkWidget* about = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(about), 16);
    gtk_box_pack_start(GTK_BOX(about), gtk_label_new("OpenKey for Linux\nVietnamese IBus input method\nGPL-3.0-or-later"), FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), about, gtk_label_new("About"));
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
