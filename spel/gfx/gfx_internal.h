#ifndef SPEL_GFX_INTERNAL
#define SPEL_GFX_INTERNAL
#include "gfx/gfx_pipeline.h"
#include "gfx/gfx_shader.h"
#include "gfx_buffer.h"
#include "gfx_commands.h"
#include "gfx_types.h"
#include "shaderc/shaderc.h"
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

#define sp_cmdlist_default_size (8 * 1024) // 8 KB

// buffers
typedef struct spel_gfx_buffer_t
{
	spel_gfx_context ctx;
	void* data;
	bool persistent;
	spel_gfx_buffer_type type;
} spel_gfx_buffer_t;

// shaders
typedef struct spel_gfx_shader_t
{
	spel_gfx_shader_stage type;
	spel_gfx_context ctx;
	void* data;
	uint64_t hash;
} spel_gfx_shader_t;

// pipelines
typedef enum
{
	SPEL_GFX_PIPELINE_GRAPHIC,
	SPEL_GFX_PIPELINE_COMPUTE // NOT supported yet lol
} spel_gfx_pipeline_type;

typedef struct spel_gfx_pipeline_t
{
	spel_gfx_context ctx;
	spel_gfx_pipeline_type type;

	const char* name;
	uint64_t hash;

	void* data;
} spel_gfx_pipeline_t;

// initialization
typedef struct spel_gfx_vtable_t* spel_gfx_vtable;

typedef struct spel_gfx_context_t
{
	spel_gfx_backend backend;
	spel_gfx_vtable vt;
	bool debug;
	int vsync;

	spel_gfx_cmdlist cmdlist;
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
	void* (*cmdlist_alloc)(spel_gfx_cmdlist, size_t, size_t);

	spel_gfx_buffer (*buffer_create)(spel_gfx_context, const spel_gfx_buffer_desc*);
	void (*buffer_destroy)(spel_gfx_buffer);
	void (*buffer_update)(spel_gfx_buffer, const void*, size_t, size_t);
	void* (*buffer_map)(spel_gfx_buffer, size_t, size_t, spel_gfx_access);
	void (*buffer_unmap)(spel_gfx_buffer);
	void (*buffer_flush)(spel_gfx_buffer buf, size_t offset, size_t size);

	spel_gfx_shader (*shader_create)(spel_gfx_context, const spel_gfx_shader_desc* desc);
	void (*shader_destroy)(spel_gfx_shader);

	spel_gfx_pipeline (*pipeline_create)(spel_gfx_context,
										 const spel_gfx_pipeline_desc* desc);
	void (*pipeline_destroy)(spel_gfx_pipeline);
} spel_gfx_vtable_t;

extern void spel_gfx_context_create_gl(spel_gfx_context ctx);
// initialization

sp_hidden shaderc_shader_kind
spel_gfx_shader_state_to_shaderc(spel_gfx_shader_stage stage);

#endif