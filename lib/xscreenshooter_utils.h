#ifndef XSCREENSHOOTER_UTILS
#define XSCREENSHOOTER_UTILS

GtkWidget *xscreenshooter_get_header_bar();
GdkPixbuf *xscreenshooter_get_thumbnail(GdkPixbuf *image);
gint xscreenshooter_create_modal_dialog(GtkWindow *parent, gchar *title, gchar *button_text, gchar *label_text);

#endif
