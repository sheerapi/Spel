#include "gfx/gfx_context.h"
#include "gfx/gfx_types.h"
#include "core/types.h"
#include "gfx/gfx_internal.h"
#include "backends/gl/gfx_vtable_gl.h"

void spel_gfx_context_conf(spel_gfx_backend backend)
{
	switch (backend)
	{
	case SPEL_GFX_BACKEND_OPENGL:
		spel_gfx_context_conf_gl();
		break;
	}
}

spel_gfx_context spel_gfx_context_create(spel_gfx_context_desc* desc)
{
	spel_gfx_context ctx = (spel_gfx_context)malloc(sizeof(spel_gfx_context_t));
	ctx->backend = desc->backend;
	ctx->debug = desc->debug;

	switch (ctx->backend)
	{
	case SPEL_GFX_BACKEND_OPENGL:
		spel_gfx_context_create_gl(ctx);
		break;
	}

	return ctx;
}

void spel_gfx_context_destroy(spel_gfx_context ctx)
{
	ctx->vt->ctx_destroy(ctx);
}