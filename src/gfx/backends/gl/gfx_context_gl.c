#include "SDL3/SDL_video.h"
#include "core/entry.h"
#include "core/types.h"
#include "gfx/gfx_internal.h"
#include "gfx_vtable_gl.h"
#define GLAD_GL_IMPLEMENTATION 1
#include "gl.h"

typedef struct spel_gfx_context_gl
{
	SDL_GLContext ctx;

	struct
	{
		uint8_t major;
		uint8_t minor;
	} version;
} spel_gfx_context_gl;

void spel_gfx_context_create_gl(spel_gfx_context ctx)
{
	spel_gfx_context_gl* gl = (ctx->data = malloc(sizeof(spel_gfx_context_gl)));

	gl->ctx = SDL_GL_CreateContext(spel.window.handle);
	if (!gl->ctx)
	{
		spel_error("Failed to create an opengl context");
		return;
	}

	ctx->vt = &GL_VTABLE;
	ctx->data = gl;

	SDL_GL_MakeCurrent(spel.window.handle, gl->ctx);
	SDL_GL_SetSwapInterval(ctx->vsync);

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

void spel_gfx_context_destroy_gl(spel_gfx_context ctx)
{
	spel_gfx_context_gl* gl = (spel_gfx_context_gl*)ctx->data;
	gladLoaderUnloadGL();
	SDL_GL_DestroyContext(gl->ctx);
}