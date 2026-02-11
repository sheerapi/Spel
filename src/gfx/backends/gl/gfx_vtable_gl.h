#ifndef SPEL_GFX_GL_VTABLE
#define SPEL_GFX_GL_VTABLE
#include "core/macros.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_shader.h"
#include "gfx/gfx_types.h"
#include "gl.h"
#include "gl_types.h"

sp_hidden void spel_gfx_context_destroy_gl(spel_gfx_context ctx);
sp_hidden void spel_gfx_context_conf_gl();

sp_hidden void spel_gfx_frame_begin_gl(spel_gfx_context ctx);
sp_hidden void spel_gfx_frame_end_gl(spel_gfx_context ctx);

sp_hidden void spel_gfx_debug_callback(unsigned int source, unsigned int type,
									   unsigned int id, unsigned int severity, int length,
									   const char* message, const void* userParam);

// command lists
sp_hidden spel_gfx_cmdlist spel_gfx_cmdlist_create_gl(spel_gfx_context ctx);
sp_hidden void spel_gfx_cmdlist_destroy_gl(spel_gfx_cmdlist cl);
sp_hidden void spel_gfx_cmdlist_submit_gl(spel_gfx_cmdlist cl);

sp_hidden void* spel_gfx_cmdlist_alloc_gl(spel_gfx_cmdlist cl, size_t size, size_t align);

// buffers
sp_hidden spel_gfx_buffer spel_gfx_buffer_create_gl(spel_gfx_context ctx,
													const spel_gfx_buffer_desc* desc);

sp_hidden void spel_gfx_buffer_destroy_gl(spel_gfx_buffer buf);
sp_hidden void spel_gfx_buffer_update_gl(spel_gfx_buffer buf, const void* data,
										 size_t size, size_t offset);

sp_hidden void* spel_gfx_buffer_map_gl(spel_gfx_buffer buf, size_t offset, size_t size,
									   spel_gfx_buffer_access access);
sp_hidden void spel_gfx_buffer_unmap_gl(spel_gfx_buffer buf);
sp_hidden void spel_gfx_buffer_flush_gl(spel_gfx_buffer buf, size_t offset, size_t size);

// shaders
sp_hidden spel_gfx_shader spel_gfx_shader_create_gl(spel_gfx_context ctx,
													spel_gfx_shader_desc* desc);

sp_hidden void spel_gfx_shader_destroy_gl(spel_gfx_shader shader);

// pipelines
sp_hidden spel_gfx_pipeline
spel_gfx_pipeline_create_gl(spel_gfx_context ctx, const spel_gfx_pipeline_desc* desc);

sp_hidden void spel_gfx_pipeline_destroy_gl(spel_gfx_pipeline pipeline);

// textures
sp_hidden spel_gfx_texture spel_gfx_texture_create_gl(spel_gfx_context ctx,
													  const spel_gfx_texture_desc* desc);

sp_hidden void spel_gfx_texture_destroy_gl(spel_gfx_texture texture);

sp_hidden spel_gfx_sampler spel_gfx_sampler_create_gl(spel_gfx_context ctx,
													  const spel_gfx_sampler_desc* desc);

sp_hidden void spel_gfx_sampler_destroy_gl(spel_gfx_sampler sampler);

extern spel_gfx_vtable_t GL_VTABLE;

#endif