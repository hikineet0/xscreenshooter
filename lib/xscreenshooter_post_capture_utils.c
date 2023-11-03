#include <gdk/gdk.h>

#include "xscreenshooter_debug.h"
#include "xscreenshooter_globals.h"
#include "xscreenshooter_post_capture_utils.h"


void xscreenshooter_save_to_file(CaptureData *capture_data)
{
    GdkPixbuf *pixbuf = capture_data->capture_pixbuf;
    gdk_pixbuf_save(pixbuf, "/home/arch/Desktop/image.jpg", "jpeg", NULL, "quality", "100", NULL);
}
