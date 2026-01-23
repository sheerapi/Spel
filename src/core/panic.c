#include "core/panic.h"
#include "utils/terminal.h"
#include <stdio.h>

_Noreturn void spel_panic(spel_log_event evt)
{
	static int in_panic = 0;

	if (in_panic++)
	{
		fputs("double trouble!? im getting out of here!\n", stderr);
		abort();
	}

	fprintf(stderr, "%s%s==== PANIC! =====%s\n", sp_terminal_bold,
			sp_terminal_bg_bright_red, sp_terminal_reset);
	fflush(stdout);
	abort();
}