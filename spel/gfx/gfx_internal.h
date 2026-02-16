#ifndef SPEL_GFX_INTERNAL
#define SPEL_GFX_INTERNAL
#include "gfx/gfx_framebuffer.h"
#include "gfx_buffer.h"
#include "gfx_commands.h"
#include "gfx_pipeline.h"
#include "gfx_shader.h"
#include "gfx_texture.h"
#include "gfx_types.h"
#include "gfx_uniform.h"
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

	spel_gfx_buffer* dirty_buffers;
	uint32_t dirty_buffer_count;
	uint32_t dirty_buffer_cap;
} spel_gfx_cmdlist_t;

#define sp_cmdlist_default_size (4 * 1024) // 4 KB

// buffers
typedef struct spel_gfx_buffer_t
{
	spel_gfx_context ctx;
	void* data;
	bool persistent;
	spel_gfx_buffer_type type;
} spel_gfx_buffer_t;

// shaders
typedef struct spel_gfx_shader_uniform
{
	char* name;
	uint8_t stage_mask;
	bool used;
	spel_gfx_uniform_type type;

	union
	{
		uint32_t location;
		struct
		{
			uint16_t set;
			uint16_t binding;
		};
	};

	uint32_t offset;
	uint32_t size;
	uint32_t array_count;

	uint32_t internal;
} spel_gfx_shader_uniform;

typedef struct spel_gfx_shader_block
{
	char* name;
	union
	{
		uint32_t location;
		struct
		{
			uint16_t set;
			uint16_t binding;
		};
	};

	uint32_t internal;

	uint32_t size;
	spel_gfx_buffer_type type;
	bool accessed;

	spel_gfx_shader_uniform* members;
	uint32_t member_count;
} spel_gfx_shader_block;

typedef struct spel_gfx_shader_reflection
{
	spel_gfx_shader_block* uniforms;
	uint32_t uniform_count;

	spel_gfx_shader_block* storage;
	uint32_t storage_count;

	spel_gfx_shader_uniform* samplers;
	uint32_t sampler_count;
} spel_gfx_shader_reflection;

typedef struct spel_gfx_shader_t
{
	spel_gfx_shader_stage type;
	char* entry;
	bool internal;
	spel_gfx_context ctx;
	spel_gfx_shader_reflection reflection;
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

	spel_gfx_shader vertex_shader;
	spel_gfx_shader fragment_shader;
	spel_gfx_shader geometry_shader;

	spel_gfx_shader_reflection reflection;
	uint64_t hash;

	void* data;
} spel_gfx_pipeline_t;

typedef struct
{
	uint64_t hash;
	spel_gfx_pipeline pipeline;
} spel_gfx_pipeline_cache_entry;

typedef struct
{
	spel_gfx_pipeline_cache_entry* entries;
	uint32_t capacity;
	uint32_t count;
} spel_gfx_pipeline_cache;

// textures & samplers
typedef struct spel_gfx_texture_t
{
	spel_gfx_context ctx;
	spel_gfx_texture_type type;
	bool internal;

	void* data;
} spel_gfx_texture_t;

typedef struct spel_gfx_sampler_t
{
	spel_gfx_context ctx;
	void* data;
} spel_gfx_sampler_t;

typedef struct
{
	uint64_t hash;
	spel_gfx_sampler sampler;
} spel_gfx_sampler_cache_entry;

typedef struct
{
	spel_gfx_sampler_cache_entry* entries;
	uint32_t capacity;
	uint32_t count;
} spel_gfx_sampler_cache;

// framebuffers
typedef struct spel_gfx_framebuffer_t
{
	spel_gfx_context ctx;
	spel_gfx_framebuffer_desc desc;
	void* data;
} spel_gfx_framebuffer_t;

typedef struct spel_gfx_render_pass_t
{
	spel_gfx_context ctx;
	spel_gfx_render_pass_desc desc;
	void* data;
} spel_gfx_render_pass_t;

// initialization
typedef struct spel_gfx_vtable_t* spel_gfx_vtable;

typedef struct spel_gfx_context_t
{
	spel_gfx_backend backend;
	spel_gfx_vtable vt;
	bool debug;
	int vsync;

	int fb_width;
	int fb_height;

	// default data
	spel_gfx_cmdlist cmdlist;
	spel_gfx_pipeline_cache pipeline_cache;
	spel_gfx_sampler_cache sampler_cache;
	spel_gfx_shader shaders[3];

	spel_gfx_sampler default_sampler;
	spel_gfx_texture white_tex;
	spel_gfx_texture checkerboard;

	spel_gfx_render_pass default_pass;

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
	void (*buffer_flush)(spel_gfx_buffer, size_t, size_t);

	spel_gfx_shader (*shader_create)(spel_gfx_context, spel_gfx_shader_desc*);
	void (*shader_destroy)(spel_gfx_shader);

	spel_gfx_pipeline (*pipeline_create)(spel_gfx_context, const spel_gfx_pipeline_desc*);
	void (*pipeline_destroy)(spel_gfx_pipeline);

	spel_gfx_texture (*texture_create)(spel_gfx_context, const spel_gfx_texture_desc*);
	void (*texture_destroy)(spel_gfx_texture);

	spel_gfx_sampler (*sampler_create)(spel_gfx_context, const spel_gfx_sampler_desc*);
	void (*sampler_destroy)(spel_gfx_sampler);

	spel_gfx_framebuffer (*framebuffer_create)(spel_gfx_context,
											   const spel_gfx_framebuffer_desc*);
	void (*framebuffer_destroy)(spel_gfx_framebuffer);
	void (*framebuffer_blit)(spel_gfx_framebuffer src, spel_rect srcRegion,
							 spel_gfx_framebuffer dst, spel_rect dstRegion,
							 uint8_t attachment, spel_gfx_sampler_filter filter);

	spel_gfx_render_pass (*render_pass_create)(spel_gfx_context,
											   const spel_gfx_render_pass_desc*);
	void (*render_pass_destroy)(spel_gfx_render_pass pass);
} spel_gfx_vtable_t;

sp_hidden extern void spel_gfx_context_create_gl(spel_gfx_context ctx);

sp_hidden extern spel_gfx_pipeline spel_gfx_pipeline_cache_get_or_create(
	spel_gfx_pipeline_cache* cache, uint64_t hash, spel_gfx_pipeline pipeline);

sp_hidden void spel_gfx_pipeline_cache_remove(spel_gfx_pipeline_cache* cache,
											  uint64_t hash, spel_gfx_pipeline pipeline);

sp_api extern bool spel_gfx_texture_validate(const spel_gfx_texture_desc* desc);

sp_hidden extern void spel_gfx_shader_reflect(spel_gfx_shader shader,
											  spel_gfx_shader_desc* desc);

sp_hidden extern void spel_gfx_pipeline_merge_reflections(spel_gfx_pipeline pipeline,
														  spel_gfx_shader* shaders,
														  uint32_t shaderCount);

sp_hidden extern void spel_gfx_shader_reflection_free(spel_gfx_shader shader);

#ifdef DEBUG
sp_hidden extern const char* spel_gfx_shader_type_str(spel_gfx_shader_stage stage);
#endif
// initialization

#endif