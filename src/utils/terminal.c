#include "utils/terminal.h"
#include "core/types.h"

#if !defined(_WIN32)
#	include <unistd.h>
#endif

bool spel_terminal_detect_ansi()
{
#if defined(_WIN32)
	spel.terminal_color = true;
#else
	spel.terminal_color = (isatty(STDERR_FILENO) != 0);
#endif
	return spel.terminal_color;
}