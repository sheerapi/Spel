#include "gfx/gfx.h"
#include "core/types.h"
#include "gfx/gfx_internal.h"

extern void spel_gfx_context_create_gl(spel_gfx_context* ctx);
extern void spel_gfx_context_conf_gl();

void spel_gfx_context_conf(gfx_backend_type backend)
{
	switch (backend)
	{
	case GFX_BACKEND_OPENGL:
		spel_gfx_context_conf_gl();
		break;
	}
}

spel_gfx_context* spel_gfx_context_create(gfx_backend_type backend)
{
	spel_gfx_context* ctx = (spel_gfx_context*)malloc(sizeof(spel_gfx_context));
	ctx->backend = backend;
	ctx->debug = spel.debug;

	switch (backend)
	{
	case GFX_BACKEND_OPENGL:
		spel_gfx_context_create_gl(ctx);
		break;
	}

	return ctx;
}

void spel_gfx_context_destroy(spel_gfx_context* ctx)
{
	ctx->vt->ctx_destroy(ctx);
}

void spel_gfx_frame_begin(spel_gfx_context* ctx)
{
	ctx->vt->frame_begin(ctx);
}

void spel_gfx_frame_end(spel_gfx_context* ctx)
{
	ctx->vt->frame_end(ctx);
}