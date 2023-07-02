#ifndef XSCREENSHOOTER_CAPTURE_UTILS
#define XSCREENSHOOTER_CAPTURE_UTILS

gboolean capture(gpointer user_data);
GdkPixbuf xscreenshooter_capture_window(GdkWindow *window, gint delay, gboolean is_capture_cursor);
GdkPixbuf xscreenshooter_get_pixbuf_from_window(GdkWindow *window, gint x, gint y, gint width, gint height);
GdkWindow *xscreenshooter_get_active_window();

#endif
