#ifndef SPEL_INPUT_PLATFORM
#define SPEL_INPUT_PLATFORM

typedef struct spel_cursor_t* spel_cursor;

typedef enum
{
	SPEL_CURSOR_DEFAULT,	 /**< Default cursor. Usually an arrow. */
	SPEL_CURSOR_TEXT,		 /**< Text selection. Usually an I-beam. */
	SPEL_CURSOR_WAIT,		 /**< Wait. Usually an hourglass or watch or spinning ball. */
	SPEL_CURSOR_CROSSHAIR,	 /**< Crosshair. */
	SPEL_CURSOR_PROGRESS,	 /**< Program is busy but still interactive. Usually it's
									WAIT with an arrow. */
	SPEL_CURSOR_NWSE_RESIZE, /**< Double arrow pointing northwest and southeast. */
	SPEL_CURSOR_NESW_RESIZE, /**< Double arrow pointing northeast and southwest. */
	SPEL_CURSOR_EW_RESIZE,	 /**< Double arrow pointing west and east. */
	SPEL_CURSOR_NS_RESIZE,	 /**< Double arrow pointing north and south. */
	SPEL_CURSOR_MOVE, /**< Four pointed arrow pointing north, south, east, and west.
					   */
	SPEL_CURSOR_NOT_ALLOWED, /**< Not permitted. Usually a slashed circle or
									  crossbones. */
	SPEL_CURSOR_POINTER,	 /**< Pointer that indicates a link. Usually a pointing
									hand. */
	SPEL_CURSOR_NW_RESIZE,	 /**< Window resize top-left. This may be a single arrow
									  or a double arrow like NWSE_RESIZE. */
	SPEL_CURSOR_N_RESIZE,	 /**< Window resize top. May be NS_RESIZE. */
	SPEL_CURSOR_NE_RESIZE,	 /**< Window resize top-right. May be NESW_RESIZE. */
	SPEL_CURSOR_E_RESIZE,	 /**< Window resize right. May be EW_RESIZE. */
	SPEL_CURSOR_SE_RESIZE,	 /**< Window resize bottom-right. May be NWSE_RESIZE. */
	SPEL_CURSOR_S_RESIZE,	 /**< Window resize bottom. May be NS_RESIZE. */
	SPEL_CURSOR_SW_RESIZE,	 /**< Window resize bottom-left. May be NESW_RESIZE. */
	SPEL_CURSOR_W_RESIZE,	 /**< Window resize left. May be EW_RESIZE. */
	SPEL_CURSOR_COUNT
} spel_cursor_type;

spel_cursor spel_input_cursor_create_standard(spel_cursor_type type);
spel_cursor spel_input_cursor_create_custom(const void* pixels, int w, int h, int hotX,
									  int hotY);
void spel_input_cursor_set(spel_cursor cursor);
void spel_input_cursor_destroy(spel_cursor cursor);

const char* spel_input_clipboard_text();
void spel_input_clipboard_text_set(const char* text);

#endif