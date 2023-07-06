#include <gtk/gtk.h>
#include "xscreenshooter_globals.h"
#include "xscreenshooter_utils.h"

GtkWidget *xscreenshooter_get_header_bar()
{
	GtkWidget *header_bar = gtk_header_bar_new();
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
	gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "XScreenshooter");
	return header_bar;
}
