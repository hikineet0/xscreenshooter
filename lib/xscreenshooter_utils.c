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
