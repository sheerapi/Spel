#ifndef SPEL_TERMINAL
#define SPEL_TERMINAL
#include "core/types.h"
#include <stdbool.h>
#include <stdio.h>

#define spel_terminal(x) (spel.terminal_color ? (x) : "")

#define spel_terminal_reset spel_terminal("\x1b[0m")

#define spel_terminal_bold spel_terminal("\x1b[1m")
#define spel_terminal_dim spel_terminal("\x1b[2m")
#define spel_terminal_italic spel_terminal("\x1b[3m")
#define spel_terminal_underline spel_terminal("\x1b[4m")
#define spel_terminal_blink spel_terminal("\x1b[5m")
#define spel_terminal_invert spel_terminal("\x1b[7m")
#define spel_terminal_strike spel_terminal("\x1b[9m")

#define spel_terminal_no_bold "\x1b[22m"
#define spel_terminal_no_italic "\x1b[23m"
#define spel_terminal_no_underline "\x1b[24m"
#define spel_terminal_no_strike "\x1b[29m"

#define spel_terminal_black "\x1b[30m"
#define spel_terminal_red "\x1b[31m"
#define spel_terminal_green "\x1b[32m"
#define spel_terminal_yellow "\x1b[33m"
#define spel_terminal_blue "\x1b[34m"
#define spel_terminal_magenta "\x1b[35m"
#define spel_terminal_cyan "\x1b[36m"
#define spel_terminal_white "\x1b[37m"

#define spel_terminal_gray "\x1b[90m"
#define spel_terminal_bright_red "\x1b[91m"
#define spel_terminal_bright_green "\x1b[92m"
#define spel_terminal_bright_yellow "\x1b[93m"
#define spel_terminal_bright_blue "\x1b[94m"
#define spel_terminal_bright_magenta "\x1b[95m"
#define spel_terminal_bright_cyan "\x1b[96m"
#define spel_terminal_bright_white "\x1b[97m"

#define spel_terminal_bg_black "\x1b[40m"
#define spel_terminal_bg_red "\x1b[41m"
#define spel_terminal_bg_green "\x1b[42m"
#define spel_terminal_bg_yellow "\x1b[43m"
#define spel_terminal_bg_blue "\x1b[44m"
#define spel_terminal_bg_magenta "\x1b[45m"
#define spel_terminal_bg_cyan "\x1b[46m"
#define spel_terminal_bg_white "\x1b[47m"

#define spel_terminal_bg_gray "\x1b[100m"
#define spel_terminal_bg_bright_red "\x1b[101m"
#define spel_terminal_bg_bright_green "\x1b[102m"
#define spel_terminal_bg_bright_yellow "\x1b[103m"
#define spel_terminal_bg_bright_blue "\x1b[104m"
#define spel_terminal_bg_bright_magenta "\x1b[105m"
#define spel_terminal_bg_bright_cyan "\x1b[106m"
#define spel_terminal_bg_bright_white "\x1b[107m"

#define spel_terminal_header spel_terminal_bold spel_terminal_green
#define spel_terminal_label spel_terminal(spel_terminal_dim)
#define spel_terminal_value spel_terminal(spel_terminal_green)
#define spel_terminal_warning spel_terminal(spel_terminal_bold spel_terminal_yellow)
#define spel_terminal_error spel_terminal(spel_terminal_bold spel_terminal_red)
#define spel_terminal_success spel_terminal(spel_terminal_bold spel_terminal_green)
#define spel_terminal_muted spel_terminal(spel_terminal_gray spel_terminal_italic)

bool spel_terminal_detect_ansi();
#endif