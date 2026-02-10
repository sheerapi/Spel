#include "core/memory.h"
#include "SDL3/SDL_stdinc.h"
#include "core/log.h"
#include "core/types.h"
#include "utils/terminal.h"
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

typedef struct spel_alloc_header
{
	size_t size;
	spel_memory_tag tag;
} spel_alloc_header;

sp_api void* spel_memory_malloc(size_t size, spel_memory_tag tag)
{
	if ((int)tag < 0 || tag >= SPEL_MEM_TAG_COUNT)
	{
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "invalid memory tag %d", tag);
		raise(SIGTRAP);
		return NULL;
	}

	if (size > SIZE_MAX - sizeof(spel_alloc_header))
	{
		return NULL;
	}

	size_t total = sizeof(spel_alloc_header) + size;
	spel_alloc_header* h = malloc(total);
	if (!h)
	{
		return NULL;
	}

	memset(h, 0, total);

	h->size = size;
	h->tag = tag;

	spel.memory.current += size;
	spel.memory.total_allocated += size;
	spel.memory.alloc_count++;

	if (spel.memory.current > spel.memory.peak)
	{
		spel.memory.peak = spel.memory.current;
	}

	spel_memory_tag_stats* ts = &spel.memory.tags[tag];
	ts->bytes_current += size;
	ts->alloc_count++;

	if (ts->bytes_current > ts->bytes_peak)
	{
		ts->bytes_peak = ts->bytes_current;
	}

	return (void*)(h + 1);
}

sp_api void spel_memory_free(void* ptr)
{
	if (!ptr)
	{
		return;
	}

	spel_alloc_header* h = ((spel_alloc_header*)ptr) - 1;
	size_t size = h->size;
	spel_memory_tag tag = h->tag;

	if ((int)tag < 0 || tag >= SPEL_MEM_TAG_COUNT)
	{
		sp_error(SPEL_ERR_INVALID_STATE, "corrupted tag (%d) in free", tag);
		raise(SIGTRAP);
		return;
	}

	spel.memory.current -= size;
	spel.memory.total_freed += size;
	spel.memory.free_count++;

	spel.memory.tags[tag].bytes_current -= size;

	free(h);
}

sp_api void* spel_memory_realloc(void* ptr, size_t newSize, spel_memory_tag tag)
{

	if (!ptr)
	{
		return sp_malloc(newSize, tag);
	}

	if (newSize == 0)
	{
		sp_free(ptr);
		return NULL;
	}

	spel_alloc_header* old_h = ((spel_alloc_header*)ptr) - 1;
	size_t old_size = old_h->size;
	spel_memory_tag old_tag = old_h->tag;

	if ((int)old_tag < 0 || old_tag >= SPEL_MEM_TAG_COUNT)
	{
		sp_error(SPEL_ERR_INVALID_STATE, "corrupted tag (%d) in realloc", old_tag);
		raise(SIGTRAP);
		return NULL;
	}

	spel_memory_tag use_tag = (tag == SPEL_MEM_TAG_TEMP) ? old_tag : tag;

	if ((int)use_tag < 0 || use_tag >= SPEL_MEM_TAG_COUNT)
	{
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "invalid tag %d in realloc", use_tag);
		raise(SIGTRAP);
		return NULL;
	}

	if (newSize > SIZE_MAX - sizeof(spel_alloc_header))
	{
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "requested realloc too large (%zu)", newSize);
		raise(SIGTRAP);
		return NULL;
	}

	size_t total = sizeof(spel_alloc_header) + newSize;

	spel_alloc_header* new_h = realloc(old_h, total);
	if (!new_h)
	{
		return NULL;
	}

	if (newSize > old_size)
	{
		size_t delta = newSize - old_size;

		spel.memory.current += delta;
		spel.memory.total_allocated += delta;

		if (spel.memory.current > spel.memory.peak)
		{
			spel.memory.peak = spel.memory.current;
		}

		spel.memory.tags[use_tag].bytes_current += delta;
		if (spel.memory.tags[use_tag].bytes_current >
			spel.memory.tags[use_tag].bytes_peak)
		{
			spel.memory.tags[use_tag].bytes_peak =
				spel.memory.tags[use_tag].bytes_current;
		}
	}
	else if (newSize < old_size)
	{
		size_t delta = old_size - newSize;

		spel.memory.current -= delta;
		spel.memory.total_freed += delta;

		spel.memory.tags[use_tag].bytes_current -= delta;
	}

	new_h->size = newSize;
	new_h->tag = use_tag;

	return (void*)(new_h + 1);
}

const sp_api char* spel_memory_fmt_size(size_t bytes, char buf[32], bool colors)
{
	const char* units[] = {"B", "KB", "MB", "GB"};
	double size = (double)bytes;
	int unit = 0;

	while (size >= 1024.0 && unit < 3)
	{
		size /= 1024.0;
		unit++;
	}

	if (colors)
	{
		snprintf(buf, 32, "%s%.2f%s %s%s%s", sp_terminal_bright_green, size,
				 sp_terminal_bold, sp_terminal_bright_magenta, units[unit],
				 sp_terminal_reset);
	}
	else
	{
		snprintf(buf, 32, "%.2f %s", size, units[unit]);
	}
	return buf;
}

char* spel_mem_tag_names[SPEL_MEM_TAG_COUNT] = {
	[SPEL_MEM_TAG_CORE] = "core",
	[SPEL_MEM_TAG_GFX] = "gfx",
	[SPEL_MEM_TAG_MISC] = "misc",
	[SPEL_MEM_TAG_TEMP] = "temp",
};

sp_api void spel_memory_dump_terminal()
{
	char cur[32];
	char peak[32];
	char total[32];
	char freed[32];

	printf("%s%s==== spël memory dump ====%s\n", sp_terminal_bright_green,
		   sp_terminal_bold, sp_terminal_reset);

	printf("global:\n");
	printf("    %scurrent%s:  %s\n", sp_terminal_bright_blue, sp_terminal_reset,
		   spel_memory_fmt_size(spel.memory.current, cur, true));
	printf("    %speak%s:     %s\n", sp_terminal_bright_blue, sp_terminal_reset,
		   spel_memory_fmt_size(spel.memory.peak, peak, true));
	printf("    %stotal%s:    %s %s%s(%s%s%s freed)%s\n", sp_terminal_bright_blue,
		   sp_terminal_reset,
		   spel_memory_fmt_size(spel.memory.total_allocated, total, true),
		   sp_terminal_italic, sp_terminal_gray,
		   spel_memory_fmt_size(spel.memory.total_freed, freed, true), sp_terminal_gray,
		   sp_terminal_italic, sp_terminal_reset);

	printf("    %sallocs%s:   %s%zu%s\n    %sfrees%s:    %s%zu%s %s%s(%s%zu%s "
		   "still alive%s)%s\n",
		   sp_terminal_bright_blue, sp_terminal_reset, sp_terminal_bright_green,
		   spel.memory.alloc_count, sp_terminal_reset, sp_terminal_bright_blue,
		   sp_terminal_reset, sp_terminal_bright_green, spel.memory.free_count,
		   sp_terminal_reset, sp_terminal_italic, sp_terminal_gray,
		   sp_terminal_bright_yellow, spel.memory.alloc_count - spel.memory.free_count,
		   sp_terminal_italic, sp_terminal_gray, sp_terminal_reset);

	printf("\nby tag:\n");
	for (int i = 0; i < SPEL_MEM_TAG_COUNT; ++i)
	{
		const spel_memory_tag_stats* ts = &spel.memory.tags[i];
		if (ts->alloc_count == 0)
		{
			continue;
		}

		char tag_cur[32];
		char tag_peak[32];
		printf("    %s%-8s%s  %-10s %s%s(peak %10s%s%s)%s	 %s%sallocs: %s%zu%s\n",
			   sp_terminal_bright_blue, spel_mem_tag_names[i], sp_terminal_reset,
			   spel_memory_fmt_size(ts->bytes_current, tag_cur, true), sp_terminal_italic,
			   sp_terminal_gray, spel_memory_fmt_size(ts->bytes_peak, tag_peak, true),
			   sp_terminal_italic, sp_terminal_gray, sp_terminal_reset,
			   sp_terminal_bright_blue, sp_terminal_italic, sp_terminal_bright_green,
			   ts->alloc_count, sp_terminal_reset);
	}

	printf("%s%s===========================%s\n", sp_terminal_bright_green,
		   sp_terminal_bold, sp_terminal_reset);
}

void* sdl_spel_malloc(size_t size)
{
	return spel_memory_malloc(size, SPEL_MEM_TAG_MISC);
}

void* sdl_spel_calloc(size_t nmemb, size_t size)
{
	size_t total_size;
	if (__builtin_mul_overflow(nmemb, size, &total_size))
	{
		return NULL;
	}

	return spel_memory_malloc(total_size, SPEL_MEM_TAG_MISC);
}

void* sdl_spel_realloc(void* mem, size_t size)
{
	return spel_memory_realloc(mem, size, SPEL_MEM_TAG_MISC);
}

sp_hidden void spel_memory_sdl_setup()
{
	SDL_SetMemoryFunctions(sdl_spel_malloc, sdl_spel_calloc, sdl_spel_realloc,
						   spel_memory_free);
}

sp_api char* spel_memory_strdup(const char* src, spel_memory_tag tag)
{
	char* str;
	char* p;
	int len = 0;

	while (src[len]) {
		len++;
}
	str = spel_memory_malloc(len + 1, tag);
	p = str;
	while (*src) {
		*p++ = *src++;
}
	*p = '\0';
	return str;
}