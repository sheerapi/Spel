#include "core/log.h"
#include "core/types.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void spel_log_callback_set(spel_log_fn fn, void* user)
{
	spel.log.function = fn;
	spel.log.user = user;
}

void spel_log_emit(spel_log_event evt)
{
	bool should_callback = (evt->severity > spel.log.severity && spel.log.function) != 0;
	if (should_callback)
	{
		spel.log.function(evt, spel.log.user);
	}

	if ((int)evt->message_owned && evt->message)
	{
		spel_memory_free(evt->message);
		evt->message = nullptr;
		evt->message_owned = false;
	}
}

void spel_log_filter(spel_severity severity)
{
	spel.log.severity = severity;
}

spel_log_event spel_log_fmt(spel_log_event evt, const char* fmt, ...)
{
	if (evt->severity <= spel.log.severity)
	{
		return evt;
	}

	va_list args;
	va_start(args, fmt);

	va_list args_copy;
	va_copy(args_copy, args);


	int len = vsnprintf(nullptr, 0, fmt, args);
	if (len < 0)
	{
		va_end(args);
		va_end(args_copy);
		return evt;
	}

	char* heap_buf = spel_memory_malloc((size_t)len + 1, SPEL_MEM_TAG_CORE);
	if (!heap_buf)
	{
		va_end(args);
		va_end(args_copy);
		return evt;
	}

	vsnprintf(heap_buf, (size_t)len + 1, fmt, args_copy);

	evt->message = heap_buf;
	evt->length = (uint32_t)len;
	evt->message_owned = true;

	va_end(args);
	va_end(args_copy);
	return evt;
}

void spel_log_assert(bool condition, spel_log_event evt)
{
	if (!condition)
	{
		spel_panic(evt);
	}
}

void spel_log_stderr_install()
{
	spel_log_callback_set(&spel_log_stderr, nullptr);
}

void spel_log_stderr(spel_log_event evt, void* user)
{
	static const char* level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m",
										 "\x1b[33m", "\x1b[31m", "\x1b[35m"};

	struct tm tm;
	localtime_r(&evt->wall_sec, &tm);

	char buf[16];
	buf[strftime(buf, sizeof(buf), "%H:%M:%S", &tm)] = '\0';

	if (isatty(STDERR_FILENO))
	{
		fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf,
				level_colors[evt->severity], spel_log_sev_to_string(evt->severity),
				evt->file, evt->line);
	}
	else
	{
		fprintf(stderr, "%s %-5s %s:%d: ", buf, spel_log_sev_to_string(evt->severity),
				evt->file, evt->line);
	}

	fputs(evt->message, stderr);
	fputc('\n', stderr);
	fflush(stderr);
}

const char* spel_log_sev_to_string(spel_severity severity)
{
	switch (severity)
	{
	case SPEL_SEV_ERROR:
		return "error";
	case SPEL_SEV_FATAL:
		return "fatal";
	case SPEL_SEV_INFO:
		return "info";
	case SPEL_SEV_WARN:
		return "warn";
	case SPEL_SEV_DEBUG:
		return "debug";
	case SPEL_SEV_TRACE:
		return "trace";
	default:
		return "unk";
	}
}

void spel_panic(spel_log_event evt)
{
	printf("panic!\n");
	fflush(stdout);
	abort();
}
