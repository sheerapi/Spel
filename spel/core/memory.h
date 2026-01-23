#ifndef SPEL_MEMORY
#define SPEL_MEMORY
#include "core/macros.h"
#include <stdint.h>
#include <stddef.h>

typedef enum
{
	SPEL_MEM_TAG_CORE,
	SPEL_MEM_TAG_GFX,
	SPEL_MEM_TAG_MISC,
	SPEL_MEM_TAG_TEMP, // temporal / ephemeral
	SPEL_MEM_TAG_COUNT
} spel_memory_tag;

typedef struct spel_memory_tag_stats
{
	size_t bytes_current;
	size_t bytes_peak;
	uint64_t alloc_count;
} spel_memory_tag_stats;

typedef struct spel_memory
{
	size_t current;
	size_t peak;
	size_t total_allocated;
	size_t total_freed;

	uint64_t alloc_count;
	uint64_t free_count;

	spel_memory_tag_stats tags[SPEL_MEM_TAG_COUNT];
} spel_memory;

sp_api void* spel_memory_malloc(size_t size, spel_memory_tag tag);
sp_api void spel_memory_free(void* ptr);
sp_api void* spel_memory_realloc(void* ptr, size_t newSize, spel_memory_tag tag);

sp_api void spel_memory_render_terminal();

#endif