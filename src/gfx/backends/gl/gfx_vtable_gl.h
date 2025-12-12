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

sp_hidden spel_gfx_cmdlist spel_gfx_cmdlist_create_gl(spel_gfx_context ctx);
sp_hidden void spel_gfx_cmdlist_destroy_gl(spel_gfx_cmdlist cmd);
sp_hidden void spel_gfx_cmdlist_submit_gl(spel_gfx_cmdlist cmd);

sp_hidden void* spel_gfx_cmdlist_alloc_gl(spel_gfx_cmdlist cl, size_t size, size_t align);

static spel_gfx_vtable_t GL_VTABLE = {.ctx_destroy = spel_gfx_context_destroy_gl,

									  .frame_begin = spel_gfx_frame_begin_gl,
									  .frame_end = spel_gfx_frame_end_gl,

									  .cmdlist_create = spel_gfx_cmdlist_create_gl,
									  .cmdlist_destroy = spel_gfx_cmdlist_destroy_gl,
									  .cmdlist_submit = spel_gfx_cmdlist_submit_gl};

#endif