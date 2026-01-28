#define _GNU_SOURCE
#include "core/panic.h"
#include "core/memory.h"
#include "utils/internal/execinfo.h"
#include "utils/internal/stacktraverse.h"
#include "utils/terminal.h"
#include "utils/time.h"
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define D10(x) ceil(log10(((x) == 0) ? 2 : ((x) + 1)))

static bool spel_arg_needs_quotes(const char* s)
{
	for (; *s; s++)
	{
		switch (*s)
		{
		case ' ':
		case '\f':
		case '\n':
		case '\t':
		case '\r':
		case '\v':
		case '"':
		case '\'':
		case '\\':
		case '$':
		case '&':
		case ';':
		case '|':
		case '<':
		case '>':
		case '*':
		case '?':
		case '[':
		case ']':
		case '{':
		case '}':
		case '(':
		case ')':
		case '!':
		case '#':
			return true;
		default:
			continue;
		}
	}
	return false;
}

static size_t spel_arg_formatted_len(const char* arg)
{
	size_t len = 0;
	bool quote = spel_arg_needs_quotes(arg);

	if (!quote)
	{
		return strlen(arg);
	}

	len += 2;

	for (; *arg; arg++)
	{
		if (*arg == '"' || *arg == '\\' || *arg == '$')
		{
			len++;
		}
		len++;
	}

	return len;
}

size_t spel_argv_join_length(int argc, const char* const argv[])
{
	size_t len = 0;

	for (int i = 0; i < argc; i++)
	{
		len += spel_arg_formatted_len(argv[i]);
		if (i + 1 < argc)
		{
			len += 1;
		}
	}

	return len + 1;
}

static void spel_append_arg(char* out, size_t* pos, size_t max, const char* arg)
{
	bool quote = spel_arg_needs_quotes(arg);

	if (!quote)
	{
		while (*arg && *pos + 1 < max)
		{
			out[(*pos)++] = *arg++;
		}
		return;
	}

	if (*pos + 1 < max)
	{
		out[(*pos)++] = '"';
	}

	for (; *arg && *pos + 2 < max; arg++)
	{
		if (*arg == '"' || *arg == '\\' || *arg == '$')
		{
			out[(*pos)++] = '\\';
		}

		out[(*pos)++] = *arg;
	}

	if (*pos + 1 < max)
	{
		out[(*pos)++] = '"';
	}
}

void spel_argv_join(int argc, const char* const argv[], char* out, size_t max)
{
	size_t pos = 0;

	for (int i = 0; i < argc && pos + 1 < max; i++)
	{
		spel_append_arg(out, &pos, max, argv[i]);

		if (i + 1 < argc && pos + 1 < max)
		{
			out[pos++] = ' ';
		}
	}

	out[pos] = '\0';
}

typedef struct
{
	uint64_t hours;
	uint32_t minutes;
	uint32_t seconds;
	uint32_t milliseconds;
	uint32_t microseconds;
	uint32_t nanoseconds;
} spel_uptime_parts;

void spel_uptime_split(uint64_t ns, spel_uptime_parts* out)
{
	out->hours = ns / 3600000000000ULL;
	ns %= 3600000000000ULL;

	out->minutes = ns / 60000000000ULL;
	ns %= 60000000000ULL;

	out->seconds = ns / 1000000000ULL;
	ns %= 1000000000ULL;

	out->milliseconds = ns / 1000000ULL;
	ns %= 1000000ULL;

	out->microseconds = ns / 1000ULL;
	out->nanoseconds = ns % 1000ULL;
}

int spel_digits_count(int n)
{
	if (n == 0)
	{
		return 1;
	}
	if (n < 0)
	{
		n = -n;
	}

	return (int)floor(log10(n)) + 1;
}

void spel_backtrace_symbols_fd(void* const* buffer, int size, FILE* fd, bool colors)
{
	int i;
	Dl_info info;
	ptrdiff_t offset;

	if (size <= 0 || fd < 0)
		return;

	for (i = 0; i < size; i++)
	{
		void* addr = buffer[i];
		if (addr != NULL)
		{
			addr = (void*)((uintptr_t)addr - 1);
		}

		if (dladdr(addr, &info) != 0)
		{
			if (info.dli_sname == NULL)
			{
				info.dli_sname = "<unknown>";
			}
			if (info.dli_saddr == NULL)
			{
				info.dli_saddr = addr;
			}
			offset = (char*)addr - (char*)info.dli_saddr;

			int padding = 1 + spel_digits_count(i);

			if (colors)
			{
				char color[8];

				if (strncmp("spel", info.dli_sname, strlen("spel")) == 0)
				{
					strcpy(color, sp_terminal_green);
				}
				else if (strncmp("_", info.dli_sname, strlen("_")) == 0 ||
						 strcmp("<unknown>", info.dli_sname) == 0)
				{
					strcpy(color, sp_terminal_gray);
				}
				else if (strcmp(basename(info.dli_fname), basename(spel.process.name)) ==
						 0)
				{
					strcpy(color, sp_terminal_bright_magenta);
				}
				else
				{
					strcpy(color, sp_terminal_bright_cyan);
				}

				fprintf(fd, "    %s#%d  %s%s%s +0x%tx\n", sp_terminal_bright_blue, i,
						color, info.dli_sname, sp_terminal_reset, offset);
				fprintf(fd, "    %*s  %s%s %s%s[%p]%s\n", padding, "",
						sp_terminal_bright_cyan, basename(info.dli_fname),
						sp_terminal_gray, sp_terminal_italic, addr, sp_terminal_reset);
			}
			else
			{
				fprintf(fd, "    #%d  %s +0x%tx\n", i, info.dli_sname, offset);
				fprintf(fd, "    %*s  %s [%p]\n", padding, "", basename(info.dli_fname),
						addr);
			}
		}
		else
		{
			fprintf(fd, "%p\n", addr);
		}
	}
}

void spel_panic_stderr(spel_log_event evt)
{
	static int in_panic = 0;

	if (in_panic++)
	{
		fputs("double trouble!? im getting out of here!\n", stderr);
		abort();
	}

	char avail[32];
	char total[32];

	fprintf(stderr, "%s%s======== PANIC! ========%s\n\n", sp_terminal_bold,
			sp_terminal_bg_bright_red, sp_terminal_reset);

	fprintf(stderr, "you weren't supposed to see this...\n\n");

	fprintf(
		stderr,
		"%s had a %s%s(pretty serious)%s problem and crashed. %sspël%s generated a crash "
		"report at \"./crash.log\"\n",
		basename(spel.process.name), sp_terminal_gray, sp_terminal_italic,
		sp_terminal_reset, sp_terminal_green, sp_terminal_reset);

	fprintf(stderr,
			"running on %s%s %s%s %s%s%s (\e[%sm%s\e[0;0m) under a %s session (%s)\n\n",
			sp_terminal_bright_cyan, spel.env.os_name, sp_terminal_bright_blue,
			spel.env.arch, sp_terminal_bright_green, spel.env.os_release,
			sp_terminal_reset, spel.env.distro_color, spel.env.distro,
			spel.env.session_type, spel.env.display_server);

	fprintf(stderr, "error info:\n");
	if (evt->code == SPEL_ERR_ASSERTION_FAILED && evt->message != NULL)
	{
		const char* msg = evt->message;
		const char* p = strstr(msg, "(assertion");
		if (p)
		{
			fprintf(stderr, "    %sreason%s: ", sp_terminal_bright_blue,
					sp_terminal_reset);
			if (p > msg)
			{
				fwrite(msg, 1, (size_t)(p - msg), stderr);
			}

			const char* colon = strchr(p, ':');
			const char* closing = strchr(p, ')');

			if (colon && closing && colon < closing)
			{
				int len_before_colon = (int)(colon - p + 1);
				int len_expr = (int)(closing - (colon + 1));

				fprintf(stderr, "%s%s%.*s%s%.*s%s%c%s\n", sp_terminal_italic,
						sp_terminal_gray, len_before_colon, p, sp_terminal_bright_magenta,
						len_expr, colon + 1, sp_terminal_gray, *closing,
						sp_terminal_reset);
				if (*(closing + 1) != '\0')
				{
					fprintf(stderr, "    %s%s\n", sp_terminal_reset, closing + 1);
				}
			}
		}
		else
		{
			fprintf(stderr, "    %sreason%s: %s\n", sp_terminal_bright_blue,
					sp_terminal_reset, evt->message);
		}
	}
	else
	{
		fprintf(stderr, "    %sreason%s: %s\n", sp_terminal_bright_blue,
				sp_terminal_reset, evt->message);
	}

	fprintf(stderr, "    %ssource%s: %s%s:%s%d%s\n", sp_terminal_bright_blue,
			sp_terminal_reset, sp_terminal_bright_cyan, evt->file,
			sp_terminal_bright_magenta, evt->line, sp_terminal_reset);

	if (evt->code != SPEL_ERR_NONE)
	{
		fprintf(stderr, "    %scode%s: %s%03x%s\n", sp_terminal_bright_blue,
				sp_terminal_reset, sp_terminal_bright_red, evt->code, sp_terminal_reset);
	}

	fprintf(stderr, "\n");

	fprintf(stderr, "runtime info:\n");
	fprintf(stderr, "    %sprocessor%s:  %s\n", sp_terminal_bright_blue,
			sp_terminal_reset, spel.hardware.cpu_model);
	fprintf(stderr, "    %score count%s: %d threads, ", sp_terminal_bright_blue,
			sp_terminal_reset, spel.hardware.cpu_threads);
	if (spel.hardware.cpu_cores != 0)
	{
		fprintf(stderr, "%d cores\n", spel.hardware.cpu_cores);
	}
	else
	{
		fprintf(stderr, "unknown core count\n");
	}

	fprintf(stderr, "    %smemory%s:     %s / %s %s%s(%.2f%%)%s\n",
			sp_terminal_bright_blue, sp_terminal_reset,
			spel_memory_fmt_size(spel.hardware.ram_total - spel.hardware.ram_available,
								 avail, true),
			spel_memory_fmt_size(spel.hardware.ram_total, total, true), sp_terminal_gray,
			sp_terminal_italic,
			(((float)spel.hardware.ram_total - (float)spel.hardware.ram_available) /
			 (float)spel.hardware.ram_total) *
				100,
			sp_terminal_reset);

	fprintf(stderr, "    %sfeatures%s:   avx %s%s, avx2 %s%s, sse %s%s, neon %s%s\n",
			sp_terminal_bright_blue, sp_terminal_reset,
			(int)spel.hardware.has_avx ? sp_terminal_green "✓" : sp_terminal_red "x",
			sp_terminal_reset,
			(int)spel.hardware.has_avx2 ? sp_terminal_green "✓" : sp_terminal_red "x",
			sp_terminal_reset,
			(int)spel.hardware.has_sse ? sp_terminal_green "✓" : sp_terminal_red "x",
			sp_terminal_reset,
			(int)spel.hardware.has_neon ? sp_terminal_green "✓" : sp_terminal_red "x",
			sp_terminal_reset);

	fprintf(stderr, "\n");
	fprintf(stderr, "stack trace:\n");

	void* stack[32];
	int stack_count = backtrace(stack, 32);
	spel_backtrace_symbols_fd(stack + 2, stack_count - 2, stderr, true);

	fprintf(stderr, "\n");
	fprintf(stderr,
			"%s%sfor more information, see the core dump and/or run this command:\n",
			sp_terminal_gray, sp_terminal_italic);
	fprintf(stderr, "%saddr2line%s -e %s -pfisC ", sp_terminal_bright_cyan,
			sp_terminal_gray, spel.process.exe_path);

	Dl_info info;
	ptrdiff_t offset;

	for (int i = 0; i < stack_count - 2; i++)
	{
		void* addr = (stack + 2)[i];
		if (addr != NULL)
		{
			addr = (void*)((uintptr_t)addr - 1);
		}

		if (dladdr(addr, &info) != 0)
		{
			if (info.dli_sname != NULL)
			{
				if (strncmp("_", info.dli_sname, strlen("_")) == 0)
				{
					continue;
				}

				offset = (char*)addr - (char*)info.dli_saddr;
				fprintf(stderr, "%s+%td ", info.dli_sname, offset);
			}
		}
	}

	fprintf(stderr, "%s\n", sp_terminal_reset);

	fprintf(stderr, "%s%s========================%s\n", sp_terminal_bold,
			sp_terminal_bg_bright_red, sp_terminal_reset);
	fflush(stderr);
}

void spel_panic_file(spel_log_event evt)
{
	uint64_t uptime_ns = spel_time_now_ns() - spel.process.start_time_ns;
	FILE* file = fopen("crash.log", "w+");
	static int in_panic = 0;

	if (in_panic++)
	{
		fputs("double trouble!? im getting out of here!\n", stderr);
		abort();
	}

	char avail[32];
	char total[32];

	fprintf(file, "======== PANIC! ========\n\n");

	fprintf(file, "you weren't supposed to see this...\n\n");

	fprintf(file,
			"%s had a (pretty serious) problem and crashed. spël generated a crash "
			"report here\n",
			basename(spel.process.name));

	fprintf(file, "running on %s %s %s (%s) under a %s session (%s)\n\n",
			spel.env.os_name, spel.env.arch, spel.env.os_release, spel.env.distro,
			spel.env.session_type, spel.env.display_server);

	fprintf(file, "error info:\n");
	fprintf(file, "    reason: %s\n", evt->message);

	fprintf(file, "    source: %s:%d\n", evt->file, evt->line);

	if (evt->code != SPEL_ERR_NONE)
	{
		fprintf(file, "    code: %03x\n", evt->code);
	}

	fprintf(file, "\n");

	fprintf(file, "runtime info:\n");
	fprintf(file, "    processor:  %s\n", spel.hardware.cpu_model);
	fprintf(file, "    core count: %d threads, ", spel.hardware.cpu_threads);
	if (spel.hardware.cpu_cores != 0)
	{
		fprintf(file, "%d cores\n", spel.hardware.cpu_cores);
	}
	else
	{
		fprintf(file, "unknown core count\n");
	}

	fprintf(file, "    memory:     %s / %s (%.2f%%)\n",
			spel_memory_fmt_size(spel.hardware.ram_total - spel.hardware.ram_available,
								 avail, false),
			spel_memory_fmt_size(spel.hardware.ram_total, total, false),
			(((float)spel.hardware.ram_total - (float)spel.hardware.ram_available) /
			 (float)spel.hardware.ram_total) *
				100);

	fprintf(
		file, "    features:   avx %s, avx2 %s, sse %s, neon %s\n",
		(int)spel.hardware.has_avx ? "✓" : "x", (int)spel.hardware.has_avx2 ? "✓" : "x",
		(int)spel.hardware.has_sse ? "✓" : "x", (int)spel.hardware.has_neon ? "✓" : "x");

	fprintf(file, "\n");

	fprintf(file, "environment:\n");
	fprintf(file, "    debug?      %s\n", (int)spel.env.debug ? "yes" : "no");

	fprintf(file, "    locale:     %s\n", spel.env.locale);
	fprintf(file, "    timezone:   GMT %+03ld:00\n", spel.env.timezone / 3600);
	fprintf(file, "    env vars:   \n");

	for (size_t i = 0; i < spel.env.envc; i++)
	{
		if (strstr(spel.env.envars[i], "TOKEN") != nullptr ||
			strstr(spel.env.envars[i], "KEY") != nullptr ||
			strstr(spel.env.envars[i], "PASSWORD") != nullptr)
		{
			continue;
		}

		fprintf(file, "      - %s\n", spel.env.envars[i]);
	}

	fprintf(file, "\n");
	fprintf(file, "process:\n");
	fprintf(file, "    name:       %s\n", spel.process.name);
	fprintf(file, "    pid:        %d\n", spel.process.pid);
	fprintf(file, "    work dir:   %s\n", spel.process.working_dir);
	fprintf(file, "    exec path:  %s\n", spel.process.exe_path);

	size_t len = spel_argv_join_length((int)spel.process.argc, spel.process.argv);
	char* cmdline = spel_memory_malloc(len, SPEL_MEM_TAG_TEMP);

	spel_argv_join((int)spel.process.argc, spel.process.argv, cmdline, len);
	fprintf(file, "    cmdline:    %s\n", cmdline);

	struct tm start_tm;

	localtime_r(&spel.process.start_time, &start_tm);

	char start_buf[32];
	start_buf[strftime(start_buf, sizeof(start_buf), "%H:%M:%S %d-%m-%y", &start_tm)] =
		'\0';

	fprintf(file, "    started at: %s\n", start_buf);

	spel_uptime_parts uptime;

	spel_uptime_split(uptime_ns, &uptime);

	fprintf(file, "    uptime:     %luh:%02um:%02us.%03ums (%03uus, %03uns)\n",
			uptime.hours, uptime.minutes, uptime.seconds, uptime.milliseconds,
			uptime.microseconds, uptime.nanoseconds);

	fprintf(file, "\n");
	fprintf(file, "build info:\n");
	fprintf(file, "    compiler:   %s %s (%s linker)\n", spel.build_info.compiler_id,
			spel.build_info.compiler_version, spel.build_info.linker_id);
	fprintf(file, "    compile args: %s\n", spel.build_info.compiler_args);

	if (strcmp(spel.build_info.linker_args, "") != 0)
	{
		fprintf(file, "    link  args: %s\n", spel.build_info.linker_args);
	}

	fprintf(file, "    target:     %s\n", spel.build_info.target_triple);
	fprintf(file, "    build type: %s\n", spel.build_info.build_type);
	fprintf(file, "    backend:    %s\n", spel.build_info.build_backend);

	fprintf(file, "\n");
	fprintf(file, "git info:\n");
	fprintf(file, "    commit:     %.7s\n", spel.build_info.vcs.commit);
	fprintf(file, "    branch:     %s\n", spel.build_info.vcs.branch);
	fprintf(file, "    diff:       %.7s (%zu changes)\n", spel.build_info.vcs.diff_hash,
			spel.build_info.vcs.dirty_count);

	fprintf(file, "\n");
	fprintf(file, "stack trace:\n");

	void* stack[32];
	int stack_count = backtrace(stack, 32);
	spel_backtrace_symbols_fd(stack + 2, stack_count - 2, file, false);

	fprintf(file, "\n");
	fprintf(file, "for more information, see the core dump and/or run this command:\n");
	fprintf(file, "addr2line -e %s -pfisC ", spel.process.exe_path);

	Dl_info info;
	ptrdiff_t offset;

	for (int i = 0; i < stack_count - 2; i++)
	{
		void* addr = (stack + 2)[i];
		if (addr != NULL)
		{
			addr = (void*)((uintptr_t)addr - 1);
		}

		if (dladdr(addr, &info) != 0)
		{
			if (info.dli_sname != NULL)
			{
				if (strncmp("_", info.dli_sname, strlen("_")) == 0)
				{
					continue;
				}

				offset = (char*)addr - (char*)info.dli_saddr;
				fprintf(file, "%s+%td ", info.dli_sname, offset);
			}
		}
	}

	fprintf(file, "\n========================");
	fflush(file);
	fclose(file);
}

_Noreturn void spel_panic(spel_log_event evt)
{
	spel_panic_file(evt);
	spel_panic_stderr(evt);
	abort();
}