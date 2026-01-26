#include "core/panic.h"
#include "utils/terminal.h"
#include "utils/time.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

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

_Noreturn void spel_panic(spel_log_event evt)
{
	uint64_t uptime_ns = spel_time_now_ns() - spel.process.start_time_ns;
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

	fprintf(stderr,
			"%s had a (pretty serious) problem and crashed. spël generated a crash "
			"report at \"<empty>\"\n",
			spel.process.name);

	fprintf(stderr, "running on %s %s %s (\e[%sm%s\e[0;0m) under a %s session (%s)\n\n",
			spel.env.os_name, spel.env.arch, spel.env.os_release, spel.env.distro_color,
			spel.env.distro, spel.env.session_type, spel.env.display_server);

	fprintf(stderr, "error info:\n");
	fprintf(stderr, "    reason: %s\n", evt->message);
	fprintf(stderr, "    source: %s:%d\n", evt->file, evt->line);

	if (evt->code != SPEL_ERR_NONE)
	{
		fprintf(stderr, "    code: %03x\n", evt->code);
	}

	fprintf(stderr, "\n");

	fprintf(stderr, "runtime info:\n");
	fprintf(stderr, "    processor:  %s\n", spel.hardware.cpu_model);
	fprintf(stderr, "    core count: %d threads, ", spel.hardware.cpu_threads);
	if (spel.hardware.cpu_cores != 0)
	{
		fprintf(stderr, "%d cores\n", spel.hardware.cpu_cores);
	}
	else
	{
		fprintf(stderr, "unknown core count\n");
	}

	fprintf(stderr, "    memory:     %s / %s (%.2f%%)\n",
			spel_memory_fmt_size(spel.hardware.ram_total - spel.hardware.ram_available,
								 avail),
			spel_memory_fmt_size(spel.hardware.ram_total, total),
			(((float)spel.hardware.ram_total - (float)spel.hardware.ram_available) /
			 (float)spel.hardware.ram_total) *
				100);

	fprintf(
		stderr, "    features:   avx %s, avx2 %s, sse %s, neon %s\n",
		(int)spel.hardware.has_avx ? "✓" : "x", (int)spel.hardware.has_avx2 ? "✓" : "x",
		(int)spel.hardware.has_sse ? "✓" : "x", (int)spel.hardware.has_neon ? "✓" : "x");

	fprintf(stderr, "\n");

	fprintf(stderr, "environment:\n");
	fprintf(stderr, "    debug?      %s\n", (int)spel.env.debug ? "yes" : "no");

	fprintf(stderr, "    locale:     %s\n", spel.env.locale);
	fprintf(stderr, "    timezone:   GMT %+03ld:00\n", spel.env.timezone / 3600);
	fprintf(stderr, "    env vars:   %d defined\n", spel.env.envc);

	fprintf(stderr, "\n");
	fprintf(stderr, "process:\n");
	fprintf(stderr, "    name:       %s\n", spel.process.name);
	fprintf(stderr, "    pid:        %d\n", spel.process.pid);
	fprintf(stderr, "    work dir:   %s\n", spel.process.working_dir);
	fprintf(stderr, "    exec path:  %s\n", spel.process.exe_path);

	size_t len = spel_argv_join_length((int)spel.process.argc, spel.process.argv);
	char* cmdline = spel_memory_malloc(len, SPEL_MEM_TAG_TEMP);

	spel_argv_join((int)spel.process.argc, spel.process.argv, cmdline, len);
	fprintf(stderr, "    cmdline:    %s\n", cmdline);

	struct tm start_tm;

	localtime_r(&spel.process.start_time, &start_tm);

	char start_buf[32];
	start_buf[strftime(start_buf, sizeof(start_buf), "%H:%M:%S %d-%m-%y", &start_tm)] =
		'\0';

	fprintf(stderr, "    started at: %s\n", start_buf);

	spel_uptime_parts uptime;

	spel_uptime_split(uptime_ns, &uptime);

	fprintf(stderr, "    uptime:     %luh:%02um:%02us.%03ums (%03uus, %03uns)\n",
			uptime.hours, uptime.minutes, uptime.seconds, uptime.milliseconds,
			uptime.microseconds, uptime.nanoseconds);

	fprintf(stderr, "\n");
	/*fprintf(stderr, "build info:\n");
	fprintf(stderr, "    compiler:   %s %s (%s linker)\n", spel.build_info.compiler_id,
			spel.build_info.compiler_version, spel.build_info.linker_id);
	fprintf(stderr, "    compile args: %s\n", spel.build_info.compiler_args);

	if (strcmp(spel.build_info.linker_args, "") != 0)
	{
		fprintf(stderr, "    link  args: %s\n", spel.build_info.linker_args);
	}

	fprintf(stderr, "    target:     %s\n", spel.build_info.target_triple);
	fprintf(stderr, "    build type: %s\n", spel.build_info.build_type);
	fprintf(stderr, "    backend:    %s\n", spel.build_info.build_backend);

	fprintf(stderr, "\n");
	fprintf(stderr, "git info:\n");
	fprintf(stderr, "    commit:     %.7s\n", spel.build_info.vcs.commit);
	fprintf(stderr, "    branch:     %s\n", spel.build_info.vcs.branch);
	fprintf(stderr, "    diff:       %.7s (%zu changes)\n", spel.build_info.vcs.diff_hash,
			spel.build_info.vcs.dirty_count);*/

	fprintf(stderr, "%s%s========================%s\n", sp_terminal_bold,
			sp_terminal_bg_bright_red, sp_terminal_reset);
	fflush(stdout);
	abort();
}