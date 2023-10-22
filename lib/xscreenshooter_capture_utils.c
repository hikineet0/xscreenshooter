#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <cairo-xlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include "xscreenshooter_globals.h"
#include "xscreenshooter_debug.h"
#include "xscreenshooter_capture_utils.h"


typedef struct {
    gboolean is_cancelled;
    gboolean is_button_pressed;
    // The starting point of the drag and draw operation.
    int x1, y1;
    GdkRectangle rectangle;
    GC *gc;
} RbData;


static GdkPixbuf *xscreenshooter_capture_window(GdkWindow *window, gint delay, gboolean is_capture_cursor);
static GdkPixbuf *xscreenshooter_get_pixbuf_from_window(GdkWindow *window, gint x, gint y, gint width, gint height);

static GdkWindow *xscreenshooter_get_active_window();
static Window xscreenshooter_get_active_window_from_xlib();
static GdkWindow* xscreenshooter_get_area_selection();
static GdkFilterReturn mask(GdkXEvent *xevent, GdkEvent *event, RbData *rbdata);


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
        case SELECT:
			window = xscreenshooter_get_area_selection();
            break;
	}
	capture_pixbuf = xscreenshooter_capture_window(window, is_show_cursor, delay);
	capture_data->capture_pixbuf = capture_pixbuf;
}

static GdkPixbuf *xscreenshooter_capture_window(GdkWindow *window, gint delay, gboolean is_capture_cursor)
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
 static GdkPixbuf *xscreenshooter_get_pixbuf_from_window(GdkWindow *window, gint x, gint y, gint width, gint height)
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

static GdkWindow *xscreenshooter_get_active_window()
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

static Window xscreenshooter_get_active_window_from_xlib()
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

static GdkFilterReturn mask(GdkXEvent *xevent, GdkEvent *event, RbData *rbdata)
{
    int x2 = 0, y2 = 0;
    int key;

    XEvent *x_event  = (XEvent*) xevent;
    gint event_type;
    XIDeviceEvent *device_event;
    Display *display;
    Window root_window;

    event_type = x_event->type;
    display = gdk_x11_get_default_xdisplay();
    root_window = gdk_x11_get_default_root_xwindow();

    if (event_type == GenericEvent)
        event_type = x_event->xgeneric.evtype;

    switch (event_type){
        case XI_ButtonPress:
            rbdata->is_button_pressed = TRUE;
            device_event = (XIDeviceEvent*) x_event->xcookie.data;
            rbdata->rectangle.x = rbdata->x1 = device_event->root_x;
            rbdata->rectangle.y = rbdata->y1 = device_event->root_y;
            return GDK_FILTER_REMOVE;
            break;

        case XI_ButtonRelease:

            rbdata->is_button_pressed = FALSE;
            device_event = (XIDeviceEvent*) x_event->xcookie.data;
            if (rbdata->rectangle.width > 0 && rbdata->rectangle.height > 0)
            {
                // Remove previously drawn rectangle
                // Makes use of GXxor GC function
                XDrawRectangle(display,
                        root_window,
                        *rbdata->gc,
                        rbdata->rectangle.x,
                        rbdata->rectangle.y,
                        rbdata->rectangle.width,
                        rbdata->rectangle.height);
            }
            gtk_main_quit();
            return GDK_FILTER_REMOVE;
            break;

        case XI_Motion:
            if (rbdata->is_button_pressed)
            {
                if (rbdata->rectangle.width > 0 && rbdata->rectangle.height > 0)
                {
                    // Remove previously drawn rectangle
                    // Makes use of GXxor GC function
                    XDrawRectangle(display,
                            root_window,
                            *rbdata->gc,
                            rbdata->rectangle.x,
                            rbdata->rectangle.y,
                            rbdata->rectangle.width,
                            rbdata->rectangle.height);
                }

                device_event = (XIDeviceEvent*) x_event->xcookie.data;
                x2 = device_event->root_x;
                y2 = device_event->root_y;
                rbdata->rectangle.x = MIN(rbdata->x1, x2);
                rbdata->rectangle.y = MIN(rbdata->y1, y2);
                rbdata->rectangle.height = ABS(y2 - rbdata->y1);
                rbdata->rectangle.width = ABS(x2 - rbdata->x1);

                if (rbdata->rectangle.width > 0 && rbdata->rectangle.height > 0)
                {
                    XDrawRectangle(display,
                            root_window,
                            *rbdata->gc,
                            rbdata->rectangle.x,
                            rbdata->rectangle.y,
                            rbdata->rectangle.width,
                            rbdata->rectangle.height);
                }
            }
            return GDK_FILTER_REMOVE;
            break;

        case XI_KeyPress:
            device_event = (XIDeviceEvent*) x_event->xcookie.data;
            key = device_event->detail;

            if (key == XKeysymToKeycode(gdk_x11_get_default_xdisplay(), XK_Escape))
            {
                if (rbdata->is_button_pressed)
                {
                    if (rbdata->rectangle.width > 0 && rbdata->rectangle.height > 0)
                        {
                            // Remove the rectangle previously drawn
                            // Makes use of the GXxor GC function
                            XDrawRectangle(display,
                                    root_window,
                                    *rbdata->gc,
                                    rbdata->rectangle.x,
                                    rbdata->rectangle.y,
                                    rbdata->rectangle.width,
                                    rbdata->rectangle.height);
                        }
                }
                rbdata->is_cancelled = TRUE;
                gtk_main_quit();
                return GDK_FILTER_REMOVE;
            }
            break;
    }
}

static GdkWindow* xscreenshooter_get_area_selection(CaptureData *capture_data)
{
    GdkWindow *window;

    XGCValues gc_values;
    GC gc;
    gint value_mask;
    Display *display;
    int screen;

    GdkSeat *seat;
    GdkCursor *cursor;
    RbData rbdata;

    display = gdk_x11_get_default_xdisplay();
    screen = gdk_x11_get_default_screen();

    gc_values.function = GXxor;
    gc_values.line_width = 2;
    gc_values.line_style = LineOnOffDash;
    gc_values.fill_style = FillSolid;
    gc_values.graphics_exposures = FALSE;
    gc_values.subwindow_mode = IncludeInferiors;
    gc_values.background = XBlackPixel(display, screen);
    gc_values.foreground = XWhitePixel(display, screen);

    value_mask = GCFunction | GCLineWidth | GCLineStyle |
        GCFillStyle | GCGraphicsExposures | GCSubwindowMode |
        GCBackground | GCForeground;

    gc = XCreateGC(display,
            gdk_x11_get_default_root_xwindow(),
            value_mask,
            &gc_values);

    rbdata.x1 = 0;
    rbdata.x1 = 0;
    rbdata.rectangle.width = 0;
    rbdata.rectangle.height = 0;
    rbdata.is_button_pressed = FALSE;
    rbdata.is_cancelled = FALSE;
    rbdata.gc = &gc;

    window = gdk_get_default_root_window();
    cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_CROSSHAIR);
    seat = gdk_display_get_default_seat(gdk_display_get_default());

    gdk_window_show_unraised(window);

    gdk_seat_grab(seat, window, GDK_SEAT_CAPABILITY_ALL, FALSE, cursor, NULL,NULL, NULL);

    gdk_window_add_filter(window, (GdkFilterFunc) mask, &rbdata);

    gtk_main();

    gdk_window_remove_filter(window,
            (GdkFilterFunc) mask,
            &rbdata);

    gdk_seat_ungrab(seat);

    if (gc != NULL)
        XFreeGC(display, gc);

    g_object_unref(cursor);

    if (!rbdata.is_cancelled)
    {
        // get window
        // from rectangle
    }
    else
        gtk_main_quit();

    return window;
}
