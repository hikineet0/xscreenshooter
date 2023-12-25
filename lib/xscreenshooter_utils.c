#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <regex.h>
#include <wordexp.h>
#include <gtk/gtk.h>

#include "xscreenshooter_debug.h"
#include "xscreenshooter_globals.h"
#include "xscreenshooter_utils.h"

GtkWidget *xscreenshooter_get_header_bar()
{
	GtkWidget *header_bar = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "XScreenshooter");
	return header_bar;
}

GdkPixbuf *xscreenshooter_get_thumbnail(GdkPixbuf *image)
{
    gint w = gdk_pixbuf_get_width(image);
    gint h = gdk_pixbuf_get_height(image);
    gint width = THUMB_X_SIZE;
    gint height = THUMB_Y_SIZE;

    if (w >= h)
        height = width * h / w;
    else
        width = height * w / h;

    return gdk_pixbuf_scale_simple(image, width, height, GDK_INTERP_BILINEAR);
}

gint xscreenshooter_create_modal_dialog(GtkWindow *parent, gchar *title, gchar *button_text, gchar *label_text)                            
{                                                                                                                                          
    GtkWidget *window, *label, *header_bar;
    
    window = gtk_dialog_new_with_buttons(NULL, parent, GTK_DIALOG_MODAL, button_text, 999, NULL);
    gtk_widget_set_size_request(window, 300, 140);

    label = gtk_label_new(label_text);
    gtk_widget_set_size_request(label, 300, 140);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);

    header_bar = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), title);
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
    
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(window))), label, TRUE, TRUE, 0);
    gtk_widget_show_all(window);

    return gtk_dialog_run(GTK_DIALOG(window));
}  

gboolean xscreenshooter_load_previous_session(CaptureData *capture_data)
{
    char *file_location;
    wordexp_t exp_result;

    // size set with the longest possible line in the file; i.e. the the line for save location.
    unsigned int line_max = PATH_MAX + 14; // len("save_location=")
    char line[line_max];

    regex_t regex;
    regmatch_t pmatch[3];
    const char *pattern = "(.*)=(.*$)";
    char key[15], value[PATH_MAX + 1];
    int delay;

    regcomp(&regex, pattern, REG_EXTENDED);
    wordexp("~/.config/xscreenshooter/previous_session.ini", &exp_result, 0);
    file_location = exp_result.we_wordv[0];

    FILE *file;
    file = fopen(file_location, "r");
    if (file == NULL)
    {
        // file does not exist
        wordfree(&exp_result);
        return FALSE;
    }
    while (fgets(line, line_max, file))
    {
        regexec(&regex, line, 3, pmatch, 0);
        sprintf(key, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, line + pmatch[1].rm_so);
        sprintf(value, "%.*s",pmatch[2].rm_eo - pmatch[2].rm_so - 1, line + pmatch[2].rm_so); // length to remove newline character
        if (!g_strcmp0(key, "capture type"))
        {
            if (!g_strcmp0(value, "ENTIRE"))
                capture_data->capture_type = ENTIRE;
            else if (!g_strcmp0(value, "ACTIVE"))
                capture_data->capture_type = ACTIVE;
            else if (!g_strcmp0(value, "SELECT"))
                capture_data->capture_type = SELECT;
        }
        else if (!g_strcmp0(key, "delay"))
        {
            delay = strtol(value, NULL, 10);
            capture_data->delay = delay;
        }
        else if (!g_strcmp0(key, "capture cursor"))
        {
            if (!g_strcmp0(value, "TRUE"))
                capture_data->is_show_cursor = TRUE;
            else if (!g_strcmp0(value, "FALSE"))
                capture_data->is_show_cursor = FALSE;
        }
        else if (!g_strcmp0(key, "action type"))
        {
            if (!g_strcmp0(value, "SAVE"))
                capture_data->action_type = SAVE;
            else if (!g_strcmp0(value, "CLIPBOARD"))
                capture_data->action_type = CLIPBOARD;
            else if (!g_strcmp0(value, "OPEN"))
                capture_data->action_type = OPEN;
            else if (!g_strcmp0(value, "UPLOAD"))
                capture_data->action_type = UPLOAD;
        }
        else if (!g_strcmp0(key, "save location"))
        {
            if (g_strcmp0(value, "NULL"))
                capture_data->save_location = strdup(value);
        }
    }
    fclose(file);
    regfree(&regex);
    wordfree(&exp_result);
    return TRUE;
}

void xscreenshooter_save_session(CaptureData *capture_data)
{
    FILE *file;
    char *dir;
    char file_location[PATH_MAX];
    wordexp_t exp_result;

    wordexp("~/.config/xscreenshooter", &exp_result, 0);
    dir = exp_result.we_wordv[0];
    if (!mkdir(dir, S_IRWXU))
    {
        switch (errno)
        {
            case EEXIST:
                break;
            default:
                perror("Failed to create directory ~/.config/xscreenshooter");
        }
    }
    sprintf(file_location, "%s/%s", dir, "previous_session.ini");

    file = fopen(file_location, "w");
    fprintf(file, "capture type=");
    switch (capture_data->capture_type)
    {
        case ENTIRE:
            fprintf(file, "ENTIRE\n");
            break;
        case ACTIVE:
            fprintf(file, "ACTIVE\n");
            break;
        case SELECT:
            fprintf(file, "SELECT\n");
            break;
        default:
            fprintf(file, "NULL\n");
            break;
    }
    fprintf(file, "delay=%d\n", capture_data->delay);
    fprintf(file, "capture cursor=");
    switch (capture_data->is_show_cursor)
    {
        case TRUE:
            fprintf(file, "TRUE\n");
            break;
        case FALSE:
            fprintf(file, "FALSE\n");
            break;
        default:
            fprintf(file, "NULL\n");
            break;
    }
    fprintf(file, "action type=");
    switch (capture_data->action_type)
    {
        case SAVE:
            fprintf(file, "SAVE\n");
            break;
        case CLIPBOARD:
            fprintf(file, "CLIPBOARD\n");
            break;
        case OPEN:
            fprintf(file, "OPEN\n");
            break;
        case UPLOAD:
            fprintf(file, "UPLOAD\n");
            break;
        default:
            fprintf(file, "NULL\n");
            break;
    }
    if (capture_data->save_location)
        fprintf(file, "save location=%s\n", capture_data->save_location);
    else
        fprintf(file, "save location=NULL\n");

    wordfree(&exp_result);
    fclose(file);
}
