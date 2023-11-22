#ifndef XSCREENSHOOTER_GLOBALS
#define XSCREENSHOOTER_GLOBALS

#define THUMB_X_SIZE 200
#define THUMB_Y_SIZE 140

typedef enum
{
	ENTIRE, ACTIVE, SELECT
} CaptureType;

typedef enum
{
	SAVE, CLIPBOARD, OPEN, UPLOAD
} ActionType;

typedef enum
{
    UPLOAD_HOST_CB = 85726766,
    OPEN_WITH_CB = 79876766,
    TIME_LIMIT_CB = 84766766
} ComboBox;

typedef struct
{
	// pre capture
	gint delay;
	CaptureType capture_type;
	gboolean is_show_cursor;

	// post capture
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
