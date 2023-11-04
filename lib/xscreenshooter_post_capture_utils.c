#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>

#include "xscreenshooter_debug.h"
#include "xscreenshooter_globals.h"
#include "xscreenshooter_post_capture_utils.h"


void xscreenshooter_save_to_file(CaptureData *capture_data)
{
    GdkPixbuf *pixbuf = capture_data->capture_pixbuf;

    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    GtkFileFilter *filter;
    gint res;
    char *filename;
    GDateTime *date_time;
    int length;
    gint64 timestamp;

    date_time = g_date_time_new_now_local();
    timestamp = g_date_time_to_unix(date_time);
    length = snprintf(NULL, 0, "%d", timestamp);
    char *timestamp_s = malloc(length+1);
    snprintf(timestamp_s, length+1, "%d", timestamp);
    filename = g_strconcat("Screenshot_", timestamp_s, ".png");

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
        filename = gtk_file_chooser_get_filename(chooser);
        gdk_pixbuf_save(pixbuf, filename, "png", NULL, "compression", "9", NULL);
    }
}
