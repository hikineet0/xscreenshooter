#ifndef XSCREENSHOOTER_GLOBALS
#define XSCREENSHOOTER_GLOBALS

#define THUMB_X_SIZE 200
#define THUMB_Y_SIZE 200

typedef enum
{
	ENTIRE, ACTIVE, SELECT
} CaptureType;

typedef enum
{
	SAVE, CLIPBOARD, OPEN, UPLOAD
} ActionType;

typedef struct
{
	// pre capture
	gint delay;
	CaptureType capture_type;
	gboolean is_show_cursor;

	// post capture
	gboolean captured;
	GdkPixbuf *capture_pixbuf;
	ActionType action_type;
	gchar *app;
	GAppInfo *app_info;

	// POST data
	char *url;
	GList *keys, *values;
	char *file_key;
	char *time_key;
	char *time_option;
} CaptureData;

#endif
