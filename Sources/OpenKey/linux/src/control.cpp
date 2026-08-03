#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include <string>
#include <vector>
#include "config.h"

struct App {
    OpenKeySettings settings;
    AppIndicator* indicator{};
    GtkWidget* window{};
    GtkWidget* enabled{};
    GtkWidget* macro_list{};
    GtkWidget* macro_trigger{};
    GtkWidget* macro_replacement{};
};

static void load_macro_list(App* app) {
    GList* children = gtk_container_get_children(GTK_CONTAINER(app->macro_list));
    for (GList* item = children; item; item = item->next) gtk_widget_destroy(GTK_WIDGET(item->data));
    g_list_free(children);
    gchar** entries = g_settings_get_strv(app->settings.get(), "macros");
    for (gchar** entry = entries; entry && *entry; ++entry) {
        gchar** pair = g_strsplit(*entry, "\t", 2);
        if (pair[0] && pair[1]) {
            GtkWidget* row = gtk_list_box_row_new();
            std::string label = std::string(pair[0]) + "  →  " + pair[1];
            gtk_container_add(GTK_CONTAINER(row), gtk_label_new(label.c_str()));
            g_object_set_data_full(G_OBJECT(row), "trigger", g_strdup(pair[0]), g_free);
            g_object_set_data_full(G_OBJECT(row), "replacement", g_strdup(pair[1]), g_free);
            gtk_container_add(GTK_CONTAINER(app->macro_list), row);
        }
        g_strfreev(pair);
    }
    g_strfreev(entries);
    gtk_widget_show_all(app->macro_list);
}

static void save_macro(App* app, const char* remove_trigger = nullptr) {
    const char* trigger = gtk_entry_get_text(GTK_ENTRY(app->macro_trigger));
    const char* replacement = gtk_entry_get_text(GTK_ENTRY(app->macro_replacement));
    std::vector<std::string> entries;
    gchar** current = g_settings_get_strv(app->settings.get(), "macros");
    for (gchar** entry = current; entry && *entry; ++entry) {
        gchar** pair = g_strsplit(*entry, "\t", 2);
        bool keep = pair[0] && (!remove_trigger || g_strcmp0(pair[0], remove_trigger) != 0) && g_strcmp0(pair[0], trigger) != 0;
        if (keep) entries.emplace_back(*entry);
        g_strfreev(pair);
    }
    g_strfreev(current);
    if (*trigger && *replacement && !remove_trigger) entries.emplace_back(std::string(trigger) + "\t" + replacement);
    std::vector<const gchar*> raw;
    for (const auto& entry : entries) raw.push_back(entry.c_str());
    raw.push_back(nullptr);
    g_settings_set_strv(app->settings.get(), "macros", raw.data());
    if (!remove_trigger) { gtk_entry_set_text(GTK_ENTRY(app->macro_trigger), ""); gtk_entry_set_text(GTK_ENTRY(app->macro_replacement), ""); }
    load_macro_list(app);
}

static void macro_add(GtkButton*, gpointer data) { save_macro(static_cast<App*>(data)); }
static void macro_delete(GtkButton*, gpointer data) {
    auto* app = static_cast<App*>(data);
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->macro_list));
    if (row) save_macro(app, static_cast<const char*>(g_object_get_data(G_OBJECT(row), "trigger")));
}
static void macro_selected(GtkListBox*, GtkListBoxRow* row, gpointer data) {
    if (!row) return;
    auto* app = static_cast<App*>(data);
    gtk_entry_set_text(GTK_ENTRY(app->macro_trigger), static_cast<const char*>(g_object_get_data(G_OBJECT(row), "trigger")));
    gtk_entry_set_text(GTK_ENTRY(app->macro_replacement), static_cast<const char*>(g_object_get_data(G_OBJECT(row), "replacement")));
}

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

static void set_menu_choice(GtkCheckMenuItem* item, gpointer data) {
    if (!gtk_check_menu_item_get_active(item)) return;
    const char* key = static_cast<const char*>(g_object_get_data(G_OBJECT(item), "openkey-key"));
    GSettings* settings = g_settings_new("org.openkey.Linux");
    g_settings_set_int(settings, key, GPOINTER_TO_INT(data));
    g_object_unref(settings);
}

static void add_choice(GtkWidget* menu, GSList** group, const char* label, const char* key, int value, int selected) {
    GtkWidget* item = gtk_radio_menu_item_new_with_label(*group, label);
    *group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
    g_object_set_data(G_OBJECT(item), "openkey-key", const_cast<char*>(key));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), value == selected);
    g_signal_connect(item, "toggled", G_CALLBACK(set_menu_choice), GINT_TO_POINTER(value));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
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
    GtkWidget* type_menu = gtk_menu_new();
    GtkWidget* type_root = gtk_menu_item_new_with_label("Kiểu gõ");
    GSList* type_group = nullptr;
    const char* type_names[] = {"Telex", "VNI", "Simple Telex 1", "Simple Telex 2", "Tự định nghĩa"};
    int selected_type = g_settings_get_int(app->settings.get(), "input-type");
    for (int i = 0; i < 5; ++i) add_choice(type_menu, &type_group, type_names[i], "input-type", i, selected_type);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(type_root), type_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), type_root);
    GtkWidget* code_menu = gtk_menu_new();
    GtkWidget* code_root = gtk_menu_item_new_with_label("Bảng mã");
    GSList* code_group = nullptr;
    const char* code_names[] = {"Unicode dựng sẵn", "TCVN3 (ABC)", "VNI Windows", "Unicode tổ hợp", "Vietnamese Locale CP1258"};
    int selected_code = g_settings_get_int(app->settings.get(), "code-table");
    for (int i = 0; i < 5; ++i) add_choice(code_menu, &code_group, code_names[i], "code-table", i, selected_code);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(code_root), code_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), code_root);
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
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Bảng mã"), FALSE, FALSE, 0);
    GtkWidget* charset = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(charset), "Unicode dựng sẵn");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(charset), "TCVN3 (ABC)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(charset), "VNI Windows");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(charset), "Unicode tổ hợp");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(charset), "Vietnamese Locale CP1258");
    gtk_combo_box_set_active(GTK_COMBO_BOX(charset), g_settings_get_int(app->settings.get(), "code-table"));
    g_signal_connect(charset, "changed", G_CALLBACK(combo_changed), const_cast<char*>("code-table"));
    gtk_box_pack_start(GTK_BOX(box), charset, FALSE, FALSE, 0);
    setting_switch(app, box, "Kiểm tra chính tả", "spell-check");
    setting_switch(app, box, "Dấu kiểu mới (oà, uý)", "modern-orthography");
    setting_switch(app, box, "Gõ tắt Telex", "quick-telex");
    setting_switch(app, box, "Khôi phục từ sai", "restore-invalid");
    setting_switch(app, box, "Bật gõ tắt", "use-macro");
    setting_switch(app, box, "Viết hoa đầu câu", "uppercase-first");
    GtkWidget* macros = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(macros), 16);
    GtkWidget* macro_title = gtk_label_new("<b>Gõ tắt</b>");
    gtk_label_set_use_markup(GTK_LABEL(macro_title), TRUE);
    gtk_box_pack_start(GTK_BOX(macros), macro_title, FALSE, FALSE, 0);
    setting_switch(app, macros, "Bật gõ tắt", "use-macro");
    setting_switch(app, macros, "Dùng gõ tắt khi ở chế độ tiếng Anh", "macro-in-english");
    setting_switch(app, macros, "Tự viết hoa phần thay thế", "auto-caps-macro");
    app->macro_list = gtk_list_box_new();
    gtk_widget_set_vexpand(app->macro_list, TRUE);
    g_signal_connect(app->macro_list, "row-selected", G_CALLBACK(macro_selected), app);
    gtk_box_pack_start(GTK_BOX(macros), app->macro_list, TRUE, TRUE, 0);
    app->macro_trigger = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->macro_trigger), "Gõ tắt, ví dụ: dc");
    gtk_box_pack_start(GTK_BOX(macros), app->macro_trigger, FALSE, FALSE, 0);
    app->macro_replacement = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->macro_replacement), "Nội dung thay thế");
    gtk_box_pack_start(GTK_BOX(macros), app->macro_replacement, FALSE, FALSE, 0);
    GtkWidget* macro_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* add_macro = gtk_button_new_with_label("Thêm / Cập nhật");
    GtkWidget* delete_macro = gtk_button_new_with_label("Xoá mục chọn");
    g_signal_connect(add_macro, "clicked", G_CALLBACK(macro_add), app);
    g_signal_connect(delete_macro, "clicked", G_CALLBACK(macro_delete), app);
    gtk_box_pack_start(GTK_BOX(macro_buttons), add_macro, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(macro_buttons), delete_macro, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(macros), macro_buttons, FALSE, FALSE, 0);
    load_macro_list(app);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), macros, gtk_label_new("Gõ tắt"));
    GtkWidget* system = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(system), 16);
    gtk_box_pack_start(GTK_BOX(system), gtk_label_new("Tuỳ chọn nâng cao"), FALSE, FALSE, 0);
    setting_switch(app, system, "Cho phép phụ âm đầu Z, F, W, J", "allow-zfwj");
    setting_switch(app, system, "Gõ tắt phụ âm đầu", "quick-start-consonant");
    setting_switch(app, system, "Gõ tắt phụ âm cuối", "quick-end-consonant");
    gtk_box_pack_start(GTK_BOX(system), gtk_label_new("Dùng biểu tượng VI / EN trên thanh trên cùng để chuyển chế độ. Có thể thêm OpenKey vào Ứng dụng khởi động để chạy cùng Ubuntu."), FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), system, gtk_label_new("Hệ thống"));
    GtkWidget* about = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(about), 16);
    gtk_box_pack_start(GTK_BOX(about), gtk_label_new("OpenKey cho Linux\nBộ gõ tiếng Việt dùng IBus\nGiấy phép GPL-3.0-or-later"), FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), about, gtk_label_new("Giới thiệu"));
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
