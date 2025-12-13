#ifndef SPEL_GFX_INTERNAL
#define SPEL_GFX_INTERNAL
#include "gfx/gfx_buffer.h"
#include "gfx/gfx_types.h"
#include <stddef.h>
#include <stdint.h>

// command lists
typedef struct spel_gfx_cmdlist_t
{
	uint8_t* buffer;
	uint64_t offset;
	uint64_t capacity;

	spel_gfx_context ctx;
	void* data;
} spel_gfx_cmdlist_t;

typedef struct spel_gfx_cmd_header
{
	uint16_t type;
	uint16_t size;
} spel_gfx_cmd_header;

#define sp_cmdlist_default_size (4 * 1024) // 4 KB

// buffers
typedef struct spel_gfx_buffer_t
{
	spel_gfx_context ctx;
	void* data;
	bool persistent;
} spel_gfx_buffer_t;

// initialization
typedef struct spel_gfx_vtable_t* spel_gfx_vtable;

typedef struct spel_gfx_context_t
{
	spel_gfx_backend backend;
	spel_gfx_vtable vt;
	bool debug;
	int vsync;

	void* data;
} spel_gfx_context_t;

typedef struct spel_gfx_vtable_t
{
	void (*ctx_destroy)(spel_gfx_context);

	void (*frame_begin)(spel_gfx_context);
	void (*frame_end)(spel_gfx_context);

	spel_gfx_cmdlist (*cmdlist_create)(spel_gfx_context);
	void (*cmdlist_destroy)(spel_gfx_cmdlist);
	void (*cmdlist_submit)(spel_gfx_cmdlist);

	spel_gfx_buffer (*buffer_create)(spel_gfx_context, const spel_gfx_buffer_desc*);
	void (*buffer_destroy)(spel_gfx_buffer);
	void (*buffer_update)(spel_gfx_buffer, const void*, size_t, size_t);
	void* (*buffer_map)(spel_gfx_buffer, size_t, size_t, spel_gfx_access);
	void (*buffer_unmap)(spel_gfx_buffer);
	void (*buffer_flush)(spel_gfx_buffer buf, size_t offset, size_t size);
} spel_gfx_vtable_t;

extern void spel_gfx_context_create_gl(spel_gfx_context ctx);
// initialization

#endif