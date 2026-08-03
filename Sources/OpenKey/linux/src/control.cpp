#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include <string>
#include <vector>
#include <glib/gstdio.h>
#include "config.h"
#include "ConvertTool.h"

struct App {
    OpenKeySettings settings;
    AppIndicator* indicator{};
    GtkWidget* window{};
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

static void export_macros(GtkButton*, gpointer data) {
    auto* app = static_cast<App*>(data);
    GtkWidget* chooser = gtk_file_chooser_dialog_new("Xuất gõ tắt", GTK_WINDOW(app->window), GTK_FILE_CHOOSER_ACTION_SAVE, "Huỷ", GTK_RESPONSE_CANCEL, "Lưu", GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), "openkey-macros.txt");
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        GString* text = g_string_new(";Compatible OpenKey Macro Data file for UniKey*** version=1 ***\n");
        gchar** entries = g_settings_get_strv(app->settings.get(), "macros");
        for (gchar** entry = entries; entry && *entry; ++entry) { gchar** pair = g_strsplit(*entry, "\t", 2); if (pair[0] && pair[1]) g_string_append_printf(text, "%s:%s\n", pair[0], pair[1]); g_strfreev(pair); }
        g_strfreev(entries); g_file_set_contents(path, text->str, text->len, nullptr); g_string_free(text, TRUE); g_free(path);
    }
    gtk_widget_destroy(chooser);
}

static void import_macros(GtkButton*, gpointer data) {
    auto* app = static_cast<App*>(data);
    GtkWidget* chooser = gtk_file_chooser_dialog_new("Nhập gõ tắt", GTK_WINDOW(app->window), GTK_FILE_CHOOSER_ACTION_OPEN, "Huỷ", GTK_RESPONSE_CANCEL, "Mở", GTK_RESPONSE_ACCEPT, nullptr);
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser)); gchar* contents = nullptr; gsize length = 0;
        if (g_file_get_contents(path, &contents, &length, nullptr)) {
            std::vector<std::string> entries; gchar** lines = g_strsplit(contents, "\n", -1);
            for (gchar** line = lines; line && *line; ++line) { if (**line == ';' || **line == '\0') continue; gchar* colon = strchr(*line, ':'); if (colon && colon != *line) { *colon = '\0'; entries.emplace_back(std::string(*line) + "\t" + (colon + 1)); } }
            std::vector<const gchar*> raw; for (const auto& entry : entries) raw.push_back(entry.c_str()); raw.push_back(nullptr);
            g_settings_set_strv(app->settings.get(), "macros", raw.data()); g_strfreev(lines); g_free(contents); load_macro_list(app);
        }
        g_free(path);
    }
    gtk_widget_destroy(chooser);
}

static void update_indicator(App* app) {
    app_indicator_set_icon_full(app->indicator, "input-keyboard", "OpenKey settings");
    app_indicator_set_label(app->indicator, "OK", "OpenKey");
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

static void startup_changed(GtkSwitch*, gboolean enabled, gpointer data) {
    auto* app = static_cast<App*>(data);
    gchar* directory = g_build_filename(g_get_user_config_dir(), "autostart", nullptr);
    gchar* path = g_build_filename(directory, "openkey-control.desktop", nullptr);
    if (enabled) {
        g_mkdir_with_parents(directory, 0700);
        const char* desktop = "[Desktop Entry]\nType=Application\nName=OpenKey\nExec=openkey-control\nX-GNOME-Autostart-enabled=true\nNoDisplay=true\n";
        g_file_set_contents(path, desktop, -1, nullptr);
    } else {
        g_remove(path);
    }
    g_settings_set_boolean(app->settings.get(), "run-on-startup", enabled);
    g_free(path); g_free(directory);
}

static void startup_switch(App* app, GtkWidget* parent) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12); GtkWidget* label = gtk_label_new("Chạy OpenKey khi đăng nhập"); GtkWidget* toggle = gtk_switch_new();
    gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_widget_set_hexpand(label, TRUE);
    gtk_switch_set_active(GTK_SWITCH(toggle), g_settings_get_boolean(app->settings.get(), "run-on-startup"));
    g_signal_connect(toggle, "state-set", G_CALLBACK(startup_changed), app);
    gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 0); gtk_box_pack_end(GTK_BOX(row), toggle, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(parent), row, FALSE, FALSE, 0);
}

static void show_settings(GtkMenuItem*, gpointer data) {
    auto* app = static_cast<App*>(data);
    gtk_widget_show_all(app->window);
    gtk_window_present(GTK_WINDOW(app->window));
}

static void quit(GtkMenuItem*, gpointer) { gtk_main_quit(); }

static void convert_text(GtkButton*, gpointer data) {
    GtkWidget* dialog = GTK_WIDGET(data);
    GtkTextBuffer* input = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(dialog), "input"));
    GtkTextBuffer* output = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(dialog), "output"));
    GtkComboBox* from = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(dialog), "from"));
    GtkComboBox* to = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(dialog), "to"));
    convertToolFromCode = gtk_combo_box_get_active(from);
    convertToolToCode = gtk_combo_box_get_active(to);
    convertToolToAllCaps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "all-caps")));
    convertToolToAllNonCaps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "all-lower")));
    convertToolToCapsFirstLetter = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "first-caps")));
    convertToolToCapsEachWord = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "word-caps")));
    convertToolRemoveMark = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(dialog), "remove-mark")));
    GtkTextIter begin, end;
    gtk_text_buffer_get_bounds(input, &begin, &end);
    gchar* source = gtk_text_buffer_get_text(input, &begin, &end, FALSE);
    std::string result = convertUtil(source);
    gtk_text_buffer_set_text(output, result.c_str(), -1);
    g_free(source);
}

static void reverse_codes(GtkButton*, gpointer data) {
    GtkWidget* dialog = GTK_WIDGET(data);
    GtkComboBox* from = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(dialog), "from"));
    GtkComboBox* to = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(dialog), "to"));
    int old_from = gtk_combo_box_get_active(from);
    gtk_combo_box_set_active(from, gtk_combo_box_get_active(to));
    gtk_combo_box_set_active(to, old_from);
}

static void clipboard_to_input(GtkButton*, gpointer data) {
    GtkWidget* dialog = GTK_WIDGET(data);
    GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gchar* text = gtk_clipboard_wait_for_text(clipboard);
    if (text) { gtk_text_buffer_set_text(GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(dialog), "input")), text, -1); g_free(text); }
}

static void output_to_clipboard(GtkButton*, gpointer data) {
    GtkWidget* dialog = GTK_WIDGET(data);
    GtkTextBuffer* output = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(dialog), "output"));
    GtkTextIter begin, end; gtk_text_buffer_get_bounds(output, &begin, &end);
    gchar* text = gtk_text_buffer_get_text(output, &begin, &end, FALSE);
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text, -1);
    g_free(text);
}

static void show_convert(GtkMenuItem*, gpointer data) {
    auto* app = static_cast<App*>(data);
    GtkWidget* dialog = gtk_dialog_new_with_buttons("Công cụ chuyển mã", GTK_WINDOW(app->window), GTK_DIALOG_DESTROY_WITH_PARENT, "Đóng", GTK_RESPONSE_CLOSE, nullptr);
    GtkWidget* area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* from = gtk_combo_box_text_new(); GtkWidget* to = gtk_combo_box_text_new();
    const char* names[] = {"Unicode", "TCVN3 (ABC)", "VNI Windows", "Unicode tổ hợp", "CP1258"};
    for (const char* name : names) { gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(from), name); gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(to), name); }
    gtk_combo_box_set_active(GTK_COMBO_BOX(from), 0); gtk_combo_box_set_active(GTK_COMBO_BOX(to), 0);
    gtk_box_pack_start(GTK_BOX(controls), gtk_label_new("Từ:"), FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(controls), from, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls), gtk_label_new("Sang:"), FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(controls), to, TRUE, TRUE, 0);
    GtkWidget* reverse = gtk_button_new_with_label("Đổi chiều"); gtk_box_pack_start(GTK_BOX(controls), reverse, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(area), controls, FALSE, FALSE, 8);
    GtkWidget* input = gtk_text_view_new(); GtkWidget* output = gtk_text_view_new(); gtk_text_view_set_editable(GTK_TEXT_VIEW(output), FALSE);
    gtk_widget_set_size_request(input, 480, 130); gtk_widget_set_size_request(output, 480, 130);
    gtk_box_pack_start(GTK_BOX(area), gtk_label_new("Văn bản nguồn:"), FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(area), input, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(area), gtk_label_new("Kết quả:"), FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(area), output, TRUE, TRUE, 0);
    GtkWidget* button = gtk_button_new_with_label("Chuyển mã"); gtk_box_pack_start(GTK_BOX(area), button, FALSE, FALSE, 8);
    GtkWidget* options = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    const char* option_names[] = {"IN HOA", "in thường", "Hoa đầu câu", "Hoa mỗi từ", "Bỏ dấu"};
    const char* option_keys[] = {"all-caps", "all-lower", "first-caps", "word-caps", "remove-mark"};
    for (int i = 0; i < 5; ++i) { GtkWidget* option = gtk_check_button_new_with_label(option_names[i]); gtk_box_pack_start(GTK_BOX(options), option, FALSE, FALSE, 0); g_object_set_data(G_OBJECT(dialog), option_keys[i], option); }
    gtk_box_pack_start(GTK_BOX(area), options, FALSE, FALSE, 0);
    GtkWidget* clipboard = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* paste = gtk_button_new_with_label("Dán từ clipboard"); GtkWidget* copy = gtk_button_new_with_label("Chép kết quả");
    gtk_box_pack_start(GTK_BOX(clipboard), paste, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(clipboard), copy, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(area), clipboard, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(dialog), "input", gtk_text_view_get_buffer(GTK_TEXT_VIEW(input)));
    g_object_set_data(G_OBJECT(dialog), "output", gtk_text_view_get_buffer(GTK_TEXT_VIEW(output)));
    g_object_set_data(G_OBJECT(dialog), "from", from); g_object_set_data(G_OBJECT(dialog), "to", to);
    g_signal_connect(button, "clicked", G_CALLBACK(convert_text), dialog);
    g_signal_connect(reverse, "clicked", G_CALLBACK(reverse_codes), dialog);
    g_signal_connect(paste, "clicked", G_CALLBACK(clipboard_to_input), dialog);
    g_signal_connect(copy, "clicked", G_CALLBACK(output_to_clipboard), dialog);
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
    gtk_widget_show_all(dialog);
}

static void activate(GtkApplication* application, gpointer data) {
    auto* app = static_cast<App*>(data);
    app->indicator = app_indicator_new("openkey", "input-keyboard", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    GtkWidget* menu = gtk_menu_new();
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
    GtkWidget* convert = gtk_menu_item_new_with_label("Công cụ chuyển mã…");
    g_signal_connect(convert, "activate", G_CALLBACK(show_convert), app);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), convert);
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
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Chuyển Việt/Anh bằng Input Sources của Ubuntu."), FALSE, FALSE, 0);

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
    GtkWidget* import_button = gtk_button_new_with_label("Nhập…"); GtkWidget* export_button = gtk_button_new_with_label("Xuất…");
    g_signal_connect(import_button, "clicked", G_CALLBACK(import_macros), app); g_signal_connect(export_button, "clicked", G_CALLBACK(export_macros), app);
    gtk_box_pack_start(GTK_BOX(macro_buttons), import_button, FALSE, FALSE, 0); gtk_box_pack_start(GTK_BOX(macro_buttons), export_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(macros), macro_buttons, FALSE, FALSE, 0);
    load_macro_list(app);
    gtk_notebook_append_page(GTK_NOTEBOOK(tabs), macros, gtk_label_new("Gõ tắt"));
    GtkWidget* system = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(system), 16);
    gtk_box_pack_start(GTK_BOX(system), gtk_label_new("Tuỳ chọn nâng cao"), FALSE, FALSE, 0);
    setting_switch(app, system, "Cho phép phụ âm đầu Z, F, W, J", "allow-zfwj");
    setting_switch(app, system, "Gõ tắt phụ âm đầu", "quick-start-consonant");
    setting_switch(app, system, "Gõ tắt phụ âm cuối", "quick-end-consonant");
    startup_switch(app, system);
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
