#ifndef XSCREENSHOOTER_POST_CAPTURE_UTILS
#define XSCREENSHOOTER_POST_CAPTURE_UTILS

void xscreenshooter_save_to_file(CaptureData *capture_data);
void xscreenshooter_copy_to_clipboard(CaptureData *capture_data);
void xscreenshooter_open_with(CaptureData *capture_data);
void xscreenshooter_upload_to(CaptureData *capture_data);

#endif
