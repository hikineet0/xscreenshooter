#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <cairo-xlib.h>
#include "xscreenshooter_globals.h"
#include "xscreenshooter_debug.h"
#include "xscreenshooter_capture_utils.h"


GdkPixbuf *xscreenshooter_capture_window(GdkWindow *window, gint delay, gboolean is_capture_cursor);
GdkPixbuf *xscreenshooter_get_pixbuf_from_window(GdkWindow *window, gint x, gint y, gint width, gint height);
GdkWindow *xscreenshooter_get_active_window();
Window xscreenshooter_get_active_window_from_xlib();

void xscreenshooter_capture(CaptureData *capture_data)
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
    GdkWindow *window, *window2;
    Window xwindow;
    GdkDisplay *display;

    //TRACE("Get the active window");

    display = gdk_display_get_default();
    xwindow = xscreenshooter_get_active_window_from_xlib();
    if (xwindow != None)
        window = gdk_x11_window_foreign_new_for_display(display, xwindow);
    else
        window = NULL;

    // If there is no active window, we fallback to the whole screen.
    if (window == NULL)
    {
  //      TRACE("No active window, fallback to the root window");

        window = gdk_get_default_root_window();
    }
    else if (gdk_window_is_destroyed(window))
    {
 //           TRACE("The active window is destroyed, fallback to the root window.");

            g_object_unref(window);
            window = gdk_get_default_root_window();
    }
    else if (gdk_window_get_type_hint(window) == GDK_WINDOW_TYPE_HINT_DESKTOP)
    {
        // If the active window is the desktop, grab the whole screen.
//        TRACE("The active window is the desktop, fallback to the root window");

        g_object_unref(window);

        window = gdk_get_default_root_window();
    }
//     else
//     {
//         // Else we find the toplevel window to grab the decorations.
//         // TRACE ("Active window is normal window, grab the toplevel window");
// 
//         window2 = gdk_window_get_toplevel(window);
// 
//         g_object_unref(window);
// 
//         window = window2;
//     }
    return window;
}

Window xscreenshooter_get_active_window_from_xlib()
{
    GdkDisplay *display;
    Display *dsp;
    Atom active_win, type;
    int status, format;
    unsigned long n_items, bytes_after;
    unsigned char *prop;
    Window window;

    display = gdk_display_get_default();
    dsp = gdk_x11_display_get_xdisplay(display);

    active_win = XInternAtom(dsp, "_NET_ACTIVE_WINDOW", True);
    if (active_win == None)
        return None;

    gdk_x11_display_error_trap_push(display);

    status = XGetWindowProperty(dsp, DefaultRootWindow(dsp),
                                active_win, 0, G_MAXLONG, False,
                                XA_WINDOW, &type, &format, &n_items,
                                &bytes_after, &prop);

    if (status != Success || type != XA_WINDOW)
    {
        if (prop)
            XFree(prop);
        
        gdk_x11_display_error_trap_pop_ignored(display);
        return None;
    }

    if (gdk_x11_display_error_trap_pop(display) != Success)
    {
        if (prop)
            XFree(prop);

        return None;
    }

    window = *(Window *)(void *) prop;
    XFree(prop);
    return window;
}
