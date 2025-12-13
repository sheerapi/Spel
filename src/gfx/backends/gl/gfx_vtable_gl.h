#ifndef SPEL_GFX_GL_VTABLE
#define SPEL_GFX_GL_VTABLE
#include "core/macros.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"

static spel_gfx_vtable_t GL_VTABLE;

sp_hidden void spel_gfx_context_destroy_gl(spel_gfx_context ctx);
sp_hidden void spel_gfx_context_conf_gl();

sp_hidden void spel_gfx_frame_begin_gl(spel_gfx_context ctx);
sp_hidden void spel_gfx_frame_end_gl(spel_gfx_context ctx);

sp_hidden void spel_gfx_debug_callback(unsigned int source, unsigned int type,
									   unsigned int id, unsigned int severity, int length,
									   const char* message, const void* userParam);

// command lists
sp_hidden spel_gfx_cmdlist spel_gfx_cmdlist_create_gl(spel_gfx_context ctx);
sp_hidden void spel_gfx_cmdlist_destroy_gl(spel_gfx_cmdlist cmd);
sp_hidden void spel_gfx_cmdlist_submit_gl(spel_gfx_cmdlist cmd);

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

static spel_gfx_vtable_t GL_VTABLE = {.ctx_destroy = spel_gfx_context_destroy_gl,

									  .frame_begin = spel_gfx_frame_begin_gl,
									  .frame_end = spel_gfx_frame_end_gl,

									  .cmdlist_create = spel_gfx_cmdlist_create_gl,
									  .cmdlist_destroy = spel_gfx_cmdlist_destroy_gl,
									  .cmdlist_submit = spel_gfx_cmdlist_submit_gl,
									  .cmdlist_alloc = spel_gfx_cmdlist_alloc_gl,

									  .buffer_create = spel_gfx_buffer_create_gl,
									  .buffer_destroy = spel_gfx_buffer_destroy_gl,
									  .buffer_update = spel_gfx_buffer_update_gl,
									  .buffer_map = spel_gfx_buffer_map_gl,
									  .buffer_unmap = spel_gfx_buffer_unmap_gl,
									  .buffer_flush = spel_gfx_buffer_flush_gl};

#endif