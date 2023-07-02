#include <gtk/gtk.h>
#include "xscreenshooter_globals.h"
#include "xscreenshooter_debug.h"
#include "xscreenshooter_pre_capture_dialog.h"

void xscreenshooter_pre_capture(CaptureData *capture_data)
{
	gint response;
	GtkWidget *dialog;
	dialog = xscreenshooter_create_pre_capture_dialog(capture_data);
	gtk_widget_show_all(dialog);
	response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response != GTK_RESPONSE_ACCEPT)
		return;
	
	if (capture_data->capture_type == SELECT)
		// because with SELECT the delay will be added when selecting?
		// try adding it here first, then if that does not work in the select phase.
		g_idle_add(cb_capture, user_data);
	else
	{
		interval = capture_data->delay == 0 ? 200 : capture_data->delay * 1000;
		g_timeout_add(interval, (GSourceFunc)cb_capture, user_data);
	}

}

void xscreenshooter_post_capture(gpointer *user_data)
{
	CaptureData capture_data = (CaptureData *) user_data;
	if (!capture_data->captured)
		return

	GtkWidget *dialog;
	dialog = xscreenshooter_create_post_capture_dialog(capture_data);
	gtk_widget_show_all(dialog);
	response = gtk_dialog_run(GTK_DIALOG(dialog));

	if (response != GTK_RESPONSE_ACCEPT)
		return G_SOURCE_REMOVE;

}

void xscreenshooter_start_gui(CaptureData *capture_data)
{
	xscreenshooter_pre_capture(capture_data);
	g_timeout_add(300, (GSourceFunc)xscreenshooter_post_capture, capture_data);
}

int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	CaptureData capture_data;
	capture_data.captured = FALSE;
	capture_data.delay = 0;
	capture_data.capture_TYPE = ENTIRE;
	capture_data.is_show_cursor = FALSE;
	capture_data.app = NULL;
	capture_data.url = NULL;
	capture_data.keys = NULL;
	capture_data.values = NULL;
	capture_data.file_key = NULL;
	capture_data.time_key = NULL;
	capture_data.time_option = NULL;
	xscreenshooter_start_gui(&capture_data);
	gtk_main();
	return 0;
}
