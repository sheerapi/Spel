#include "core/memory.h"
#include "core/log.h"
#include "core/types.h"
#include <stdio.h>

typedef struct spel_alloc_header
{
	size_t size;
	spel_memory_tag tag;
} spel_alloc_header;

void* spel_memory_malloc(size_t size, spel_memory_tag tag)
{
	size_t total = sizeof(spel_alloc_header) + size;
	spel_alloc_header* h = malloc(total);
	if (!h)
	{
		return NULL;
	}

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

void spel_memory_free(void* ptr)
{
	if (!ptr)
	{
		return;
	}

	spel_alloc_header* h = ((spel_alloc_header*)ptr) - 1;
	size_t size = h->size;
	spel_memory_tag tag = h->tag;

	spel.memory.current -= size;
	spel.memory.total_freed += size;
	spel.memory.free_count++;

	spel.memory.tags[tag].bytes_current -= size;

	free(h);
}

void* spel_memory_realloc(void* ptr, size_t newSize, spel_memory_tag tag)
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

	spel_memory_tag use_tag = (tag == SPEL_MEM_TAG_TEMP) ? old_tag : tag;

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

static const char* spel_mem_fmt_size(size_t bytes, char buf[32])
{
	const char* units[] = {"B", "KB", "MB", "GB"};
	double size = (double)bytes;
	int unit = 0;

	while (size >= 1024.0 && unit < 3)
	{
		size /= 1024.0;
		unit++;
	}

	snprintf(buf, 32, "%.2f %s", size, units[unit]);
	return buf;
}

static const char* spel_mem_tag_names[SPEL_MEM_TAG_COUNT] = {
	[SPEL_MEM_TAG_CORE] = "core",
	[SPEL_MEM_TAG_GFX] = "gfx",
	[SPEL_MEM_TAG_MISC] = "misc",
	[SPEL_MEM_TAG_TEMP] = "temp",
};

void spel_memory_dump()
{
#if !DEBUG
	log_info("spël memory management disabled.");
	return;
#else
	char cur[32];
	char peak[32];
	char total[32];
	char freed[32];

	log_info("<=== spël memory dump ===>");

	log_info("global:");
	log_info("  current: %s", spel_mem_fmt_size(spel.memory.current, cur));

	log_info("  peak:    %s", spel_mem_fmt_size(spel.memory.peak, peak));

	log_info("  total allocated: %s (%s freed)",
			 spel_mem_fmt_size(spel.memory.total_allocated, total),
			 spel_mem_fmt_size(spel.memory.total_freed, freed));

	log_info("  allocs: %u  frees: %u (%u still alive)", spel.memory.alloc_count,
			 spel.memory.free_count, spel.memory.alloc_count - spel.memory.free_count);

	log_info("");

	log_info("by tag:");

	for (int i = 0; i < SPEL_MEM_TAG_COUNT; ++i)
	{
		const spel_memory_tag_stats* ts = &spel.memory.tags[i];

		if (ts->alloc_count == 0)
		{
			continue;
		}

		char tag_cur[32];
		char tag_peak[32];

		log_info("  %-10s  %s (peak %s)  allocs %u", spel_mem_tag_names[i],
				 spel_mem_fmt_size(ts->bytes_current, tag_cur),
				 spel_mem_fmt_size(ts->bytes_peak, tag_peak), ts->alloc_count);
	}

	log_info("</=======================>");
#endif
}