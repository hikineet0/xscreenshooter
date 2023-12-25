#include <limits.h> // for PATH_MAX
#include <wordexp.h>
#include <gtk/gtk.h>
#include <dirent.h>
#include <regex.h>
#include "xscreenshooter_globals.h"
#include "xscreenshooter_debug.h"
#include "xscreenshooter_post_capture_dialog.h"
#include "xscreenshooter_utils.h"

#define ICON_SIZE 16
#define STRING_MAX 250

static void populate_combo_box_time_limit(GtkComboBoxText *combo_box, gchar **time_options);
static void set_combo_box_default(GtkWidget *combo_box, ComboBox type);

typedef struct{
    gchar *file_key;
    gchar *time_key;
    gchar **time_options;
    GList *keys, *values; // Other miscellaneous keys and values required during POST to API
}Params;

typedef struct{
    gchar *url;
    Params *params;
}POSTData;

static POSTData *post_data_copy(POSTData *src)
{
    POSTData *dest = g_new(POSTData, 1);
    dest->url = g_strdup(src->url);
    dest->params->file_key = g_strdup(src->params->file_key);
    dest->params->time_key = g_strdup(src->params->time_key);
    dest->params->keys = g_list_copy_deep(src->params->keys, (GCopyFunc)g_strdup, NULL);
    dest->params->values = g_list_copy_deep(src->params->values, (GCopyFunc)g_strdup, NULL);
    return dest;
}

static void post_data_free(POSTData *post_data)
{
    g_free(post_data->url);
    g_free(post_data->params->file_key);
    g_free(post_data->params->time_key);
    g_list_free_full(post_data->params->keys, g_object_unref);
    g_list_free_full(post_data->params->values, g_object_unref);
}

typedef struct{
    GdkPixbuf *icon;
    gchar *name;
    gchar *file_path;
}HostInfo;

void cb_action_save_radio_button_toggled(GtkToggleButton *self, CaptureData *capture_data)
{
    if (gtk_toggle_button_get_active(self))
            capture_data->action_type = SAVE;
}

void cb_action_copy_to_clipboard_radio_button_toggled(GtkToggleButton *self, CaptureData *capture_data)
{
    if (gtk_toggle_button_get_active(self))
        capture_data->action_type = CLIPBOARD;
}

void cb_action_open_with_radio_button_toggled_2(GtkToggleButton *self, GtkWidget *combo_box)
{
    if (gtk_toggle_button_get_active(self))
        gtk_widget_set_sensitive(combo_box, TRUE);
    else
        gtk_widget_set_sensitive(combo_box, FALSE);
}

void cb_action_open_with_radio_button_toggled_1(GtkToggleButton *self, CaptureData *capture_data)
{
    if (gtk_toggle_button_get_active(self))
        capture_data->action_type = OPEN;
}

void cb_action_open_with_options_combo_box_changed(GtkComboBox *self, CaptureData *capture_data)
{
    GtkTreeModel *model = gtk_combo_box_get_model(self);
    GtkTreeIter iter;
    gchar *app = NULL;
    GAppInfo *app_info = NULL;

    gtk_combo_box_get_active_iter(self, &iter);
    gtk_tree_model_get(model, &iter, 2, &app, -1);
    gtk_tree_model_get(model, &iter, 3, &app_info, -1);

    g_free(capture_data->app);
    capture_data->app = app;
    capture_data->app_info = app_info;
}

void cb_action_upload_to_radio_button_toggled_2(GtkToggleButton *self, GtkWidget *combo_box)
{
    /* Enables/disables hosts combo box. */
    gtk_widget_set_sensitive(combo_box, gtk_toggle_button_get_active(self));
}

void cb_action_upload_to_radio_button_toggled_1(GtkToggleButton *self, CaptureData *capture_data)
{
    if (gtk_toggle_button_get_active(self))
        capture_data->action_type = UPLOAD;
}

void cb_action_upload_to_options_combo_box_changed_1(GtkComboBox *self, GtkWidget *combo_box)
{
    /* Enables/disables time limit combo box based on host selection. */
    GtkTreeModel *model = gtk_combo_box_get_model(self);
    GtkTreeIter iter;
    POSTData *post_data;

    gtk_combo_box_get_active_iter(self, &iter);
    gtk_tree_model_get(model, &iter, 2, &post_data, -1);

    if (g_strcmp0(post_data->params->time_key, NULL))
    {
        populate_combo_box_time_limit(GTK_COMBO_BOX_TEXT(combo_box), post_data->params->time_options);
        set_combo_box_default(combo_box, TIME_LIMIT_CB);
        gtk_widget_set_sensitive(combo_box, TRUE);
    }
    else
    {
        gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(combo_box));
        gtk_widget_set_sensitive(combo_box, FALSE);
    }
}

void cb_action_upload_to_options_combo_box_changed_2(GtkComboBox *self, CaptureData *capture_data)
{
    /* Gets user input for host selection. */
    GtkTreeIter iter;
    POSTData *post_data;
    GtkTreeModel *model = gtk_combo_box_get_model(self);

    gtk_combo_box_get_active_iter(self, &iter);
    gtk_tree_model_get(model, &iter, 2, &post_data, -1);
    capture_data->url = post_data->url;
    capture_data->file_key = post_data->params->file_key;
    capture_data->time_key = post_data->params->time_key;
    capture_data->keys = post_data->params->keys;
    capture_data->values = post_data->params->values;
}

void cb_action_upload_to_time_limit_combo_box_changed(GtkComboBox *self, CaptureData *capture_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_combo_box_get_model(self);

    if (gtk_combo_box_get_active_iter(self, &iter))
        // Because clearing a combo box repeatedly emits the changed signal, causing it to be
        // eimtted even when there is no items in the combo box, which causes the following
        // line to cause a segfault.
        gtk_tree_model_get(model, &iter, 0, &capture_data->time_option, -1);
}

static void add_item_apps(GAppInfo *app_info, GtkWidget *list_store)
{
    GtkTreeIter iter;
    const gchar *command = g_app_info_get_executable(app_info);
    const gchar *name = g_app_info_get_name(app_info);
    GIcon *icon = g_app_info_get_icon(app_info);
    GdkPixbuf *pixbuf = NULL;
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
    GtkIconInfo *icon_info;

    icon_info = gtk_icon_theme_lookup_by_gicon(icon_theme, icon, ICON_SIZE, GTK_ICON_LOOKUP_FORCE_SIZE);

    pixbuf = gtk_icon_info_load_icon(icon_info, NULL);

    if (pixbuf == NULL)
        pixbuf = gtk_icon_theme_load_icon(icon_theme, "exec", ICON_SIZE, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);

    gtk_list_store_append(GTK_LIST_STORE(list_store), &iter);

    gtk_list_store_set(GTK_LIST_STORE(list_store), &iter,
            0, pixbuf,
            1, name,
            2, command,
            3, g_app_info_dup(app_info),
            -1);

    g_object_unref(pixbuf);
    g_object_unref(icon);
    g_object_unref(icon_info);
}

static void populate_list_store_apps(GtkListStore *list_store)
{
    const gchar *content_type;
    GList *apps_list;

    content_type = "image/png";

    apps_list = g_app_info_get_all_for_type(content_type);

    if (apps_list != NULL)
    {
        g_list_foreach(apps_list, (GFunc)add_item_apps, list_store);
        g_list_free_full(apps_list, g_object_unref);
    }
}

static void add_item_hosts(GtkListStore *list_store, HostInfo *host_info)
{
    GtkTreeIter iter;
    // dont free because post_data is useful till program terminates
    POSTData *post_data = g_new(POSTData, 1);
     post_data->params = g_new(Params, 1);
     post_data->params->keys = NULL;
     post_data->params->values = NULL;
     post_data->params->time_key = NULL;
     post_data->params->time_options = NULL;
     gchar key[STRING_MAX], value[STRING_MAX];
     
     FILE *fp;
     char *line = NULL;
     size_t size = 0;

     fp = fopen(host_info->file_path, "r");

     while (getline(&line, &size, fp) != -1)
     {
         sscanf(line, "%s = %s", key, value);
         if (!g_strcmp0(key, "url"))
             post_data->url = g_strdup(value);
         else if (!g_strcmp0(value, "<file>"))
             post_data->params->file_key = g_strdup(key);
         else if (!g_strcmp0(value, "<time_option>"))
             post_data->params->time_key = g_strdup(key);
         else if (!g_strcmp0(key, "time_options"))
             post_data->params->time_options = g_strsplit(value, ",", -1);
         else
         {
             post_data->params->keys = g_list_prepend(post_data->params->keys, g_strdup(key));
             post_data->params->values = g_list_prepend(post_data->params->values, g_strdup(value));
         }
     }
     fclose(fp);

     gtk_list_store_append(list_store, &iter);
     gtk_list_store_set(
             list_store, &iter,
             0, host_info->icon,
             1, host_info->name,
             2, post_data,
             -1);
}

static void populate_list_store_hosts(GtkListStore *list_store)
{
    HostInfo host_info;
    DIR *dir;
    struct dirent *entry;
    regex_t regex;
    char *xscreenshooter_hosts_dir;
    wordexp_t exp_result;
    char *icon_path;

    const char *pattern = ".*\\.host"; // regex to match upload host files

    wordexp("~/.config/xscreenshooter/hosts", &exp_result, 0);
    xscreenshooter_hosts_dir = exp_result.we_wordv[0];
    wordfree(&exp_result);

    dir = opendir(xscreenshooter_hosts_dir);
    if (dir == NULL)
    {
        perror("Unable to open upload hosts directory.");
        return;
    }

    regcomp (&regex, pattern, REG_EXTENDED);
    
    while ((entry = readdir(dir)) != NULL)
    {
        if (!regexec(&regex, entry->d_name, 0, NULL, 0))
        {
            // removing ".host" from filename
            entry->d_name[strlen(entry->d_name) - 5] = 0;
            host_info.file_path = g_strconcat(xscreenshooter_hosts_dir, "/", entry->d_name, NULL);
            icon_path = g_strconcat(host_info.file_path, ".favi", NULL);
            host_info.file_path = g_strconcat(host_info.file_path, ".host", NULL);
            host_info.name = entry->d_name;
            host_info.icon = gdk_pixbuf_new_from_file_at_size(icon_path, ICON_SIZE, ICON_SIZE, NULL);
            add_item_hosts(list_store, &host_info);
        }
    }
    regfree(&regex);
    closedir(dir);
}

static void populate_combo_box_time_limit(GtkComboBoxText *combo_box, gchar **time_options)
{
    int i = 0;
    while (time_options[i] != NULL)
    {
        gtk_combo_box_text_append_text(combo_box, time_options[i]);
        i++;
    }
}

static void set_combo_box_default(GtkWidget *combo_box, ComboBox type)
{
    if (type == TIME_LIMIT_CB)
        // Time limit combo box always starts at 0.
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
    else
    {
        GtkTreeIter iter;
        GtkTreePath *path;

        path = gtk_tree_path_new_from_string("0:0");

        if (gtk_tree_model_get_iter(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box)), &iter, path))
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo_box), &iter);
        else
        {
            log_s("Failed to set default value for combo box:");
            log_d(type);
        }
        gtk_tree_path_free(path);
    }
}

static void load_previous_session_selection(GSList *group, int previous_session_selection)
{
    // Order of RadioButton group does not seem to depend on order of creation.
    switch (previous_session_selection)
    {
        case UPLOAD:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_slist_nth_data(group, 0)), TRUE);
            break;
        case OPEN:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_slist_nth_data(group, 1)), TRUE);
            break;
        case CLIPBOARD:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_slist_nth_data(group, 2)), TRUE);
            break;
        default: // SAVE
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_slist_nth_data(group, 3)), TRUE);
    }
}

GtkWidget *xscreenshooter_create_post_capture_dialog(CaptureData *capture_data)
{
	GtkWidget *dialog, *header_bar, *grid, *label, *box, *actions_grid,
		  *radio_button, *combo_box, *time_limit_combo_box,
		  *preview_image;
    GdkPixbuf *thumbnail;
    GtkListStore *list_store;
    GtkCellRenderer *renderer, *renderer_pixbuf;

	dialog = gtk_dialog_new_with_buttons(
			NULL,
			NULL,
			GTK_DIALOG_MODAL,
			"_Cancel", GTK_RESPONSE_REJECT,
			"_Next", GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_widget_set_size_request(dialog, 700, 300);

	// CREATING AND SETTING THE HEADER
	header_bar = xscreenshooter_get_header_bar();
	gtk_window_set_titlebar(GTK_WINDOW(dialog), header_bar);

	// CREATING MAIN GRID FOR THE TWO SIDES
	grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(grid), 100);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid, FALSE, FALSE, 0);
	gtk_widget_set_margin_top(grid, 10);

	// BOX FOR LEFT SIDE WIDGETS
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_start(box, 24);
	gtk_grid_attach(GTK_GRID(grid), box, 0, 0, 1, 1);

	label = gtk_label_new("");
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_set_valign(label, GTK_ALIGN_START);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Actions</b>");
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    // GRID FOR ACTION BUTTONS
    actions_grid = gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(box), actions_grid, TRUE, TRUE, 0);
    gtk_grid_set_row_spacing(GTK_GRID(actions_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(actions_grid), 6);
    gtk_container_set_border_width(GTK_CONTAINER(actions_grid), 0);

    // SAVE
    radio_button = gtk_radio_button_new_with_label(NULL, "Save");
    g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_action_save_radio_button_toggled), capture_data);
    gtk_grid_attach(GTK_GRID(actions_grid), radio_button, 0, 0, 1, 1);

    // COPY TO CLIPBOARD
    radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button), "Copy to clipboard");
    g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_action_copy_to_clipboard_radio_button_toggled), capture_data);
    gtk_grid_attach(GTK_GRID(actions_grid), radio_button, 0, 1, 5, 1);
   
    // OPEN WITH
    list_store = gtk_list_store_new(4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_APP_INFO);
    combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));
    renderer = gtk_cell_renderer_text_new();
    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer_pixbuf, FALSE);
    gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer, "text", 1, NULL);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer_pixbuf, "pixbuf", 0, NULL);
    gtk_widget_set_sensitive(combo_box, FALSE);
    g_signal_connect(combo_box, "changed", G_CALLBACK(cb_action_open_with_options_combo_box_changed), capture_data);
    gtk_grid_attach(GTK_GRID(actions_grid), combo_box, 1, 2, 1, 1);
    populate_list_store_apps(list_store);
    set_combo_box_default(combo_box, OPEN_WITH_CB);

    radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button), "Open with: ");
    g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_action_open_with_radio_button_toggled_2), combo_box);
    g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_action_open_with_radio_button_toggled_1), capture_data);
    gtk_grid_attach(GTK_GRID(actions_grid), radio_button, 0, 2, 1, 1);

    // UPLOAD TO
    radio_button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button), "Upload to: ");
    gtk_grid_attach(GTK_GRID(actions_grid), radio_button, 0, 3, 1, 1);

    //                                 icon,            name,          POSTData
    list_store = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
    combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));
    renderer = gtk_cell_renderer_text_new();
    renderer_pixbuf = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer_pixbuf, FALSE);
    gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer, "text", 1, NULL);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer_pixbuf, "pixbuf", 0, NULL);
    gtk_widget_set_sensitive(combo_box, FALSE);
    gtk_grid_attach(GTK_GRID(actions_grid), combo_box, 1, 3, 1, 1);

    //                                            time frame
    time_limit_combo_box = gtk_combo_box_text_new();
    gtk_widget_set_sensitive(time_limit_combo_box, FALSE);
    gtk_grid_attach(GTK_GRID(actions_grid), time_limit_combo_box, 2, 3, 1, 1);
    
    g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_action_upload_to_radio_button_toggled_2), combo_box);
    g_signal_connect(radio_button, "toggled", G_CALLBACK(cb_action_upload_to_radio_button_toggled_1), capture_data);
    g_signal_connect(combo_box, "changed", G_CALLBACK(cb_action_upload_to_options_combo_box_changed_1), time_limit_combo_box);
    g_signal_connect(combo_box, "changed", G_CALLBACK(cb_action_upload_to_options_combo_box_changed_2), capture_data);
    g_signal_connect(time_limit_combo_box, "changed", G_CALLBACK(cb_action_upload_to_time_limit_combo_box_changed), capture_data);

    populate_list_store_hosts(list_store);
    set_combo_box_default(combo_box, UPLOAD_HOST_CB);
    load_previous_session_selection(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button)), capture_data->action_type);

    // RIGHT SIDE
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 24);
    
    label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(label), "<b>Preview</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    thumbnail = xscreenshooter_get_thumbnail(capture_data->capture_pixbuf);
    preview_image = gtk_image_new_from_pixbuf(thumbnail);
    g_object_unref(thumbnail);
    gtk_box_pack_start(GTK_BOX(box), preview_image, TRUE, TRUE, 0);

    gtk_grid_attach(GTK_GRID(grid), box, 1, 0, 1, 1);

	return dialog;
}
