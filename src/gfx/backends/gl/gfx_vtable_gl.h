#ifndef SPEL_GFX_GL_VTABLE
#define SPEL_GFX_GL_VTABLE
#include "core/macros.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"

static spel_gfx_vtable_t GL_VTABLE;

sp_hidden void spel_gfx_context_destroy_gl(spel_gfx_context ctx);
sp_hidden void spel_gfx_context_conf_gl();

static spel_gfx_vtable_t GL_VTABLE = {.ctx_destroy = spel_gfx_context_destroy_gl};

#endif