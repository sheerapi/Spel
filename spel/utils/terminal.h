#ifndef SPEL_TERMINAL
#define SPEL_TERMINAL
#include "core/types.h"
#include <stdbool.h>
#include <stdio.h>

#define sp_terminal(x) (spel.terminal_color ? (x) : "")

#define sp_terminal_reset sp_terminal("\x1b[0m")

#define sp_terminal_bold sp_terminal("\x1b[1m")
#define sp_terminal_dim sp_terminal("\x1b[2m")
#define sp_terminal_italic sp_terminal("\x1b[3m")
#define sp_terminal_underline sp_terminal("\x1b[4m")
#define sp_terminal_blink sp_terminal("\x1b[5m")
#define sp_terminal_invert sp_terminal("\x1b[7m")
#define sp_terminal_strike sp_terminal("\x1b[9m")

#define sp_terminal_no_bold "\x1b[22m"
#define sp_terminal_no_italic "\x1b[23m"
#define sp_terminal_no_underline "\x1b[24m"
#define sp_terminal_no_strike "\x1b[29m"

#define sp_terminal_black "\x1b[30m"
#define sp_terminal_red "\x1b[31m"
#define sp_terminal_green "\x1b[32m"
#define sp_terminal_yellow "\x1b[33m"
#define sp_terminal_blue "\x1b[34m"
#define sp_terminal_magenta "\x1b[35m"
#define sp_terminal_cyan "\x1b[36m"
#define sp_terminal_white "\x1b[37m"

#define sp_terminal_gray "\x1b[90m"
#define sp_terminal_bright_red "\x1b[91m"
#define sp_terminal_bright_green "\x1b[92m"
#define sp_terminal_bright_yellow "\x1b[93m"
#define sp_terminal_bright_blue "\x1b[94m"
#define sp_terminal_bright_magenta "\x1b[95m"
#define sp_terminal_bright_cyan "\x1b[96m"
#define sp_terminal_bright_white "\x1b[97m"

#define sp_terminal_bg_black "\x1b[40m"
#define sp_terminal_bg_red "\x1b[41m"
#define sp_terminal_bg_green "\x1b[42m"
#define sp_terminal_bg_yellow "\x1b[43m"
#define sp_terminal_bg_blue "\x1b[44m"
#define sp_terminal_bg_magenta "\x1b[45m"
#define sp_terminal_bg_cyan "\x1b[46m"
#define sp_terminal_bg_white "\x1b[47m"

#define sp_terminal_bg_gray "\x1b[100m"
#define sp_terminal_bg_bright_red "\x1b[101m"
#define sp_terminal_bg_bright_green "\x1b[102m"
#define sp_terminal_bg_bright_yellow "\x1b[103m"
#define sp_terminal_bg_bright_blue "\x1b[104m"
#define sp_terminal_bg_bright_magenta "\x1b[105m"
#define sp_terminal_bg_bright_cyan "\x1b[106m"
#define sp_terminal_bg_bright_white "\x1b[107m"

#define sp_terminal_header sp_terminal_bold sp_terminal_green
#define sp_terminal_label sp_terminal(sp_terminal_dim)
#define sp_terminal_value sp_terminal(sp_terminal_green)
#define sp_terminal_warning sp_terminal(sp_terminal_bold sp_terminal_yellow)
#define sp_terminal_error sp_terminal(sp_terminal_bold sp_terminal_red)
#define sp_terminal_success sp_terminal(sp_terminal_bold sp_terminal_green)
#define sp_terminal_muted sp_terminal(sp_terminal_gray sp_terminal_italic)

bool spel_terminal_detect_ansi();
#endif