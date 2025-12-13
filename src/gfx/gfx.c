#include "backends/gl/gfx_vtable_gl.h"
#include "core/types.h"
#include "gfx/gfx_buffer.h"
#include "gfx/gfx_context.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"

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

void spel_gfx_frame_begin(spel_gfx_context ctx)
{
	ctx->vt->frame_begin(ctx);
}

void spel_gfx_frame_present(spel_gfx_context ctx)
{
	ctx->vt->frame_end(ctx);
}

spel_gfx_cmdlist spel_gfx_cmdlist_create(spel_gfx_context ctx)
{
	return ctx->vt->cmdlist_create(ctx);
}

void spel_gfx_cmdlist_destroy(spel_gfx_cmdlist cmdlist)
{
	cmdlist->ctx->vt->cmdlist_destroy(cmdlist);
}

void spel_gfx_cmdlist_submit(spel_gfx_cmdlist cmdlist)
{
	cmdlist->ctx->vt->cmdlist_submit(cmdlist);
}

spel_gfx_buffer spel_gfx_buffer_create(spel_gfx_context ctx,
									   const spel_gfx_buffer_desc* desc)
{
	return ctx->vt->buffer_create(ctx, desc);
}

void spel_gfx_buffer_destroy(spel_gfx_buffer buf)
{
	buf->ctx->vt->buffer_destroy(buf);
}

void spel_gfx_buffer_update(spel_gfx_buffer buf, const void* data, size_t size,
							size_t offset)
{
	buf->ctx->vt->buffer_update(buf, data, size, offset);
}

void* spel_gfx_buffer_map(spel_gfx_buffer buf, size_t offset, size_t size,
						  spel_gfx_buffer_access access)
{
	return buf->ctx->vt->buffer_map(buf, offset, size, access);
}

void spel_gfx_buffer_unmap(spel_gfx_buffer buf)
{
	buf->ctx->vt->buffer_unmap(buf);
}