#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <gio/gio.h>
#include <curl/curl.h>

#include "xscreenshooter_debug.h"
#include "xscreenshooter_globals.h"
#include "xscreenshooter_post_capture_utils.h"
#include "xscreenshooter_utils.h"


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

static gchar *create_temp_file(gchar *filename)
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


typedef struct ResData {
    char *data;
    size_t size;
} ResData;

static void res_data_init(ResData *res_data)
{
    res_data->size = 0;
    res_data->data = malloc(res_data->size + 1);
    if (res_data->data == NULL)
    {
        log_s("ResData malloc() failed.");
        return;
    }
    res_data->data[0] = '\0';

}

static size_t write_function(char *ptr, size_t size, size_t nmemb, ResData *res_data)
{
    size_t new_size = res_data->size + size*nmemb;
    res_data->data = realloc(res_data->data, new_size+1);
    if (res_data->data == NULL)
    {
        log_s("ResData realloc() failed.");
        return CURLE_WRITE_ERROR;
    }
    memcpy(res_data->data + res_data->size, ptr, size*nmemb);
    res_data->data[new_size] = '\0';
    res_data->size = new_size;

    return size*nmemb;
}

void xscreenshooter_upload_to(CaptureData *capture_data)
{
    CURL *handle;
    CURLcode res;
    curl_mime *mime;
    curl_mimepart *part;
    ResData res_data;

    gchar *filename;
    gchar *save_location;

    GList *keys = capture_data->keys;
    GList *values = capture_data->values;
    gchar *key, *value;

    filename = get_default_filename();
    save_location = create_temp_file(filename);
    if (!gdk_pixbuf_save(capture_data->capture_pixbuf, save_location, "png", NULL, "compression", "9", NULL))
    {
        xscreenshooter_create_modal_dialog(NULL, "ERROR", "OK", "Error saving to temp file.");
        return;
    }

   handle = curl_easy_init();
    if (handle)
    {
        res_data_init(&res_data);
        curl_easy_setopt(handle, CURLOPT_URL, capture_data->url);
        // Setting options
        curl_easy_setopt(handle, CURLOPT_BUFFERSIZE, 102400L);
        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(handle, CURLOPT_USERAGENT, "curl/8.4.0");
        curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
        curl_easy_setopt(handle, CURLOPT_FTP_SKIP_PASV_IP, 1L);
        curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_function);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &res_data);

        mime = curl_mime_init(handle);
        part = curl_mime_addpart(mime);
        curl_mime_name(part, capture_data->file_key);
        curl_mime_filedata(part, save_location);

        while (g_list_length(keys))
        {
            key = g_list_first(keys)->data;
            value = g_list_first(values)->data;
            keys = g_list_remove(keys, key);
            values = g_list_remove(values, value);

            part = curl_mime_addpart(mime);
            curl_mime_name(part, key);
            curl_mime_data(part, value, CURL_ZERO_TERMINATED);
        }

        if (g_strcmp0(capture_data->time_key, NULL))
        {
            part = curl_mime_addpart(mime);
            curl_mime_name(part, capture_data->time_key);
            curl_mime_data(part, capture_data->time_option, CURL_ZERO_TERMINATED);
        }

        curl_easy_setopt(handle, CURLOPT_MIMEPOST, mime);
        res = curl_easy_perform(handle);

        curl_easy_cleanup(handle);
        curl_mime_free(mime);

        xscreenshooter_create_modal_dialog(NULL, "Upload Successful!", "OK", res_data.data);
        free(res_data.data);
    }
    g_free(filename);
    g_free(save_location);
    if (keys != NULL)
    {
        g_free(key);
        g_free(value);
        g_list_free(keys);
        g_list_free(values);
    }
    return;
}
