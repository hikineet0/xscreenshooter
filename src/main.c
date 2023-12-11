#include <gtk/gtk.h>
#include "xscreenshooter_globals.h"
#include "xscreenshooter_debug.h"
#include "xscreenshooter_pre_capture_dialog.h"
#include "xscreenshooter_post_capture_dialog.h"
#include "xscreenshooter_capture_utils.h"
#include "xscreenshooter_post_capture_utils.h"

static void xscreenshooter_pre_capture(CaptureData *capture_data);
static void xscreenshooter_post_capture(CaptureData *capture_data);

static int cb_capture(CaptureData *capture_data)
{
	xscreenshooter_capture(capture_data);
	xscreenshooter_post_capture(capture_data);
    return G_SOURCE_REMOVE;
}

static void cb_pre_capture_dialog_response(GtkWidget *self, gint response, CaptureData *capture_data)
{
	gtk_widget_destroy(self);
	if (response != GTK_RESPONSE_ACCEPT)
		gtk_main_quit();

	if (capture_data->capture_type == SELECT)
		// because with SELECT the delay will be added after the selection is done
		g_idle_add((GSourceFunc)cb_capture, capture_data);
	else
	{
		gint interval;
		// default 200ms delay because else it will capture the screenshooter dialog
		interval = (gint)(capture_data->delay == 0 ? 200 : capture_data->delay * 1000);
		g_timeout_add(interval, (GSourceFunc)cb_capture, capture_data);
	}
}

static void cb_post_capture_dialog_response(GtkWidget *self, gint response, CaptureData *capture_data)
{
	gtk_widget_destroy(self);

	if (response == GTK_RESPONSE_ACCEPT)
    {
        switch (capture_data->action_type)
        {
            case SAVE:
                xscreenshooter_save_to_file(capture_data);
                break;
            case CLIPBOARD:
                xscreenshooter_copy_to_clipboard(capture_data->capture_pixbuf);
                break;
            case OPEN:
                xscreenshooter_open_with(capture_data);
                break;
            case UPLOAD:
                xscreenshooter_upload_to(capture_data);
                break;
        }
    }
    gtk_main_quit();
}

static gboolean cb_pre_capture_dialog_return_pressed(GtkWidget *self, GdkEventKey *event, CaptureData *capture_data)
{
    if (event->keyval == GDK_KEY_Return)
    {
        cb_pre_capture_dialog_response(self, GTK_RESPONSE_ACCEPT, capture_data);
        return TRUE;
    }
    return FALSE;
}

static gboolean cb_post_capture_dialog_return_pressed(GtkWidget *self, GdkEventKey *event, CaptureData *capture_data)
{
    if (event->keyval == GDK_KEY_Return)
    {
        cb_post_capture_dialog_response(self, GTK_RESPONSE_ACCEPT, capture_data);
        return TRUE;
    }
    return FALSE;
}

static void xscreenshooter_pre_capture(CaptureData *capture_data)
{
	GtkWidget *dialog;
	dialog = xscreenshooter_create_pre_capture_dialog(capture_data);
	g_signal_connect(dialog, "key_press_event", G_CALLBACK(cb_pre_capture_dialog_return_pressed), capture_data);
	g_signal_connect(dialog, "response", G_CALLBACK(cb_pre_capture_dialog_response), capture_data);
	gtk_widget_show_all(dialog);
}

static void xscreenshooter_post_capture(CaptureData *capture_data)
{
	GtkWidget *dialog;
	dialog = xscreenshooter_create_post_capture_dialog(capture_data);
	g_signal_connect(dialog, "key_press_event", G_CALLBACK(cb_post_capture_dialog_return_pressed), capture_data);
    g_signal_connect(dialog, "response", G_CALLBACK(cb_post_capture_dialog_response), capture_data);
	gtk_widget_show_all(dialog);
}

static void xscreenshooter_start_gui(CaptureData *capture_data)
{
	xscreenshooter_pre_capture(capture_data);
	gtk_main();
	// g_timeout_add(300, (GSourceFunc)xscreenshooter_post_capture, capture_data);
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	CaptureData capture_data;
	capture_data.delay = 0;
	capture_data.capture_type = ENTIRE;
	capture_data.action_type = SAVE;
	capture_data.is_show_cursor = FALSE;
    capture_data.save_location = NULL;
	capture_data.app = NULL;
	capture_data.url = NULL;
	capture_data.keys = NULL;
	capture_data.values = NULL;
	capture_data.file_key = NULL;
	capture_data.time_key = NULL;
	capture_data.time_option = NULL;
	xscreenshooter_start_gui(&capture_data);
	return 0;
}
