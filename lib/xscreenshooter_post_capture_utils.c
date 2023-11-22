#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>

#include "xscreenshooter_debug.h"
#include "xscreenshooter_globals.h"
#include "xscreenshooter_post_capture_utils.h"


static gchar* get_default_filename()
{
    GDateTime *date_time;
    int length;
    gint64 timestamp;
    gchar *timestamp_s = malloc(length+1);
    gchar *filename;

    date_time = g_date_time_new_now_local();
    timestamp = g_date_time_to_unix(date_time);
    length = snprintf(NULL, 0, "%d", timestamp);
    snprintf(timestamp_s, length+1, "%d", timestamp);
    filename = g_strconcat("Screenshot_", timestamp_s, ".png", NULL);
    free(timestamp_s);
    return filename;
}

gchar *create_temp_file(gchar *filename)
{
    GFile *gfile = g_file_new_for_path(g_get_tmp_dir());
    gchar *gfile_path = g_file_get_path(gfile);
    gchar *save_location = g_build_filename(gfile_path, filename, NULL);

    g_object_unref(gfile);
    g_free(gfile_path);

    return save_location;
}


void xscreenshooter_save_to_file(CaptureData *capture_data)
{
    GdkPixbuf *pixbuf = capture_data->capture_pixbuf;

    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    GtkFileFilter *filter;
    gint res;
    gchar *save_location;
    gchar *filename = get_default_filename();

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filter);

    dialog = gtk_file_chooser_dialog_new ("Save File",
                                    NULL,
                                    action,
                                    "_Cancel",
                                    GTK_RESPONSE_CANCEL,
                                    "_Save",
                                    GTK_RESPONSE_ACCEPT,
                                    NULL);

    chooser = GTK_FILE_CHOOSER(GTK_DIALOG(dialog));
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
    gtk_file_chooser_set_current_name(chooser, filename);
    gtk_file_chooser_add_filter(chooser, filter);

    res = gtk_dialog_run(GTK_DIALOG(dialog));

    if (res == GTK_RESPONSE_ACCEPT)
    {
        save_location = gtk_file_chooser_get_filename(chooser);
        if (!gdk_pixbuf_save(pixbuf, save_location, "png", NULL, "compression", "9", NULL))
            log_s("Failed to save temp file.");
    }
    g_free(filename);
}

void xscreenshooter_copy_to_clipboard(GdkPixbuf *pixbuf)
{
    GtkClipboard *clipboard;
    clipboard = gtk_clipboard_get_for_display(gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_image(clipboard, pixbuf);
}

void xscreenshooter_open_with(CaptureData *capture_data)
{
    gchar *filename = get_default_filename();
    gchar *save_location = create_temp_file(filename);

    g_free(filename);

    if (gdk_pixbuf_save(capture_data->capture_pixbuf, save_location, "png", NULL, "compression", "9", NULL))
    {
        gchar *application = capture_data->app;
        GAppInfo *app_info = capture_data->app_info;
        if (!g_str_equal(application, "none"))
        {
            gboolean success;
            GError *error = NULL;
            // Get segfault for some reason, wont bother finding out as the other method works fine.
/*             if (app_info != NULL)
            {
                GList *files = g_list_append(NULL, save_location);
                log_d(3);
                success = g_app_info_launch(app_info, files, NULL, &error);
                log_d(1);
                g_list_free_full(files, g_object_unref);
                log_d(2);
            }
            else*/ if (application != NULL)
            {
                gchar *command;
                command = g_strconcat(application, " ", "\"", save_location, "\"", NULL);
                success = g_spawn_command_line_async(command, &error);
                g_free(command);
            }
            if (!success && error != NULL)
            {
                log_s("The application could not be launched.");
                log_s(error->message);
                g_free(error);
            }
        }
    }
    else
        log_s("Failed to save temp file.");
    g_free(save_location);
}

void xscreenshooter_upload_to(CaptureData *capture_data)
{
    log_s(capture_data->url);
    log_s(capture_data->file_key);
    log_s(capture_data->time_key);
    log_s(capture_data->time_option);
    gtk_main_quit();
}
