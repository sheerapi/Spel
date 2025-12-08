#include "SDL3/SDL_video.h"
#include "core/entry.h"
#include "core/log.h"
#include "core/types.h"
#include "gfx/gfx.h"
#include "gfx/gfx_internal.h"
#define GLAD_GL_IMPLEMENTATION 1
#include "gfx/gl.h"

typedef struct spel_gfx_context_gl
{
	SDL_GLContext ctx;

	struct
	{
		uint8_t major;
		uint8_t minor;
	} version;
} spel_gfx_context_gl;

extern const spel_gfx_vtable GL_VTABLE;

void spel_gfx_context_create_gl(spel_gfx_context* ctx)
{
	spel_gfx_context_gl* gl = (ctx->data = malloc(sizeof(spel_gfx_context_gl)));

	gl->ctx = SDL_GL_CreateContext(spel.window.handle);
	if (!gl->ctx)
	{
		spel_error("Failed to create an opengl context");
		return;
	}

	ctx->vt = &GL_VTABLE;

	SDL_GL_MakeCurrent(spel.window.handle, gl->ctx);
	SDL_GL_SetSwapInterval(spel.window.swapchain.vsync);

	gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
	gladLoaderLoadGL();

	glEnable(GL_BLEND);
}

void spel_gfx_context_conf_gl()
{
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, spel.window.swapchain.depth != 0
											   ? spel.window.swapchain.depth
											   : 16);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, spel.window.swapchain.stencil);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,
						spel.window.swapchain.msaa != 0 ? spel.window.swapchain.msaa : 8);
}

void spel_gfx_context_destroy_gl(spel_gfx_context* ctx)
{
	spel_gfx_context_gl* gl = (spel_gfx_context_gl*)ctx->data;
	gladLoaderUnloadGL();
	SDL_GL_DestroyContext(gl->ctx);
}

void spel_gfx_frame_begin_gl(spel_gfx_context* ctx)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}

void spel_gfx_frame_end_gl(spel_gfx_context* ctx)
{
	SDL_GL_SwapWindow(spel.window.handle);
}

const spel_gfx_vtable GL_VTABLE = {.ctx_destroy = spel_gfx_context_destroy_gl,
								   .frame_begin = spel_gfx_frame_begin_gl,
								   .frame_end = spel_gfx_frame_end_gl};