#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <cairo-xlib.h>
#include <gdk/gdkx.h>
#include "xscreenshooter_globals.h"
#include "xscreenshooter_debug.h"
#include "xscreenshooter_capture_utils.h"


GdkPixbuf *xscreenshooter_capture_window(GdkWindow *window, gint delay, gboolean is_capture_cursor);
GdkPixbuf *xscreenshooter_get_pixbuf_from_window(GdkWindow *window, gint x, gint y, gint width, gint height);
GdkWindow *xscreenshooter_get_active_window();

gboolean xscreenshooter_capture(CaptureData *capture_data)
{
	GdkWindow *window;
	GdkPixbuf *capture_pixbuf;
	gint delay = capture_data->delay;
	gboolean is_show_cursor = capture_data->is_show_cursor;

	switch (capture_data->capture_type)
	{
		case ENTIRE:
			window = gdk_get_default_root_window();
			break;
		case ACTIVE:
			window = xscreenshooter_get_active_window();
			break;
	/*	case SELECT:
			window = xscreenshooter_get_area_selection();
			break;*/
	}
	capture_pixbuf = xscreenshooter_capture_window(window, is_show_cursor, delay);
	capture_data->capture_pixbuf = capture_pixbuf;
	return G_SOURCE_REMOVE;
}

GdkPixbuf *xscreenshooter_capture_window(GdkWindow *window, gint delay, gboolean is_capture_cursor)
{
	gint x, y, width, height;
	gdk_window_get_geometry(window, &x, &y, &width, &height);
	GdkPixbuf *capture_pixbuf = xscreenshooter_get_pixbuf_from_window(window, x, y, width, height);
	return capture_pixbuf;
}

/*
 * Replacement for gdk_pixbuf_get_from_window which only takes one (first) correct screenshot per execution.
 * For more details see:
 * https://gitlab.xfce.org/apps/xfce4-screenshooter/-/issues/89
 *
 * source: https://gitlab.xfce.org/apps/xfce4-screenshooter/-/blob/master/lib/screenshooter-utils.c
 *
 */
 GdkPixbuf *xscreenshooter_get_pixbuf_from_window(GdkWindow *window, gint x, gint y, gint width, gint height)
{
	gint scale_factor;
	cairo_surface_t *surface;
	GdkPixbuf *pixbuf;

	scale_factor = gdk_window_get_scale_factor(window);
	surface = cairo_xlib_surface_create(
			gdk_x11_display_get_xdisplay(gdk_window_get_display(window)),
			gdk_x11_window_get_xid(window),
			gdk_x11_visual_get_xvisual(gdk_window_get_visual(window)),
			gdk_window_get_width(window) * scale_factor,
			gdk_window_get_height(window) * scale_factor
			);

	pixbuf = gdk_pixbuf_get_from_surface(
			surface,
			x,
			y,
			width * scale_factor,
			height * scale_factor
			);
	cairo_surface_destroy(surface);
	return pixbuf;
}

GdkWindow *xscreenshooter_get_active_window()
{
	Display *x11_display;
	Window x11_focused_window;
	GdkDisplay *gdk_display;
	GdkWindow *gdk_focused_window;
	int revert_to;

	x11_display = XOpenDisplay(NULL);

	XGetInputFocus(x11_display, &x11_focused_window, &revert_to);
	XCloseDisplay(x11_display);

	gdk_display = gdk_display_get_default();
	gdk_focused_window = gdk_x11_window_foreign_new_for_display(gdk_display, x11_focused_window);
	return gdk_focused_window;
}
