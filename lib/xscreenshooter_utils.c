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
