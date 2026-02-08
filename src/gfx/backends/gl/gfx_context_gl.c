#include "SDL3/SDL_video.h"
#include "core/entry.h"
#include "core/log.h"
#include "core/types.h"
#include "core/window.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include "gfx_vtable_gl.h"
#include <signal.h>
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

sp_hidden void spel_gfx_context_create_gl(spel_gfx_context ctx)
{
	spel_gfx_context_gl* gl =
		(ctx->data = sp_malloc(sizeof(spel_gfx_context_gl), SPEL_MEM_TAG_GFX));

	gl->ctx = SDL_GL_CreateContext(spel.window.handle);
	if (!gl->ctx)
	{
		sp_error(SPEL_ERR_CONTEXT_FAILED, "failed to create an opengl context");
		ctx->vt = NULL;
		sp_free(gl);
		return;
	}

	ctx->vt = &GL_VTABLE;
	ctx->data = gl;
	ctx->vsync = spel.window.swapchain.vsync;
	ctx->debug = spel.env.debug;

	SDL_GL_MakeCurrent(spel.window.handle, gl->ctx);
	SDL_GL_SetSwapInterval(ctx->vsync);

	if (gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0)
	{
		sp_error(SPEL_ERR_CONTEXT_FAILED, "failed to load GL functions");
		SDL_GL_DestroyContext(gl->ctx);
		ctx->vt = NULL;
		ctx->data = NULL;
		sp_free(gl);
		return;
	}
	if (gladLoaderLoadGL() == 0)
	{
		sp_error(SPEL_ERR_CONTEXT_FAILED, "failed to finalize GL loader");
		SDL_GL_DestroyContext(gl->ctx);
		ctx->vt = NULL;
		ctx->data = NULL;
		sp_free(gl);
		return;
	}

	glEnable(GL_BLEND);

	if (ctx->debug)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(spel_gfx_debug_callback, ctx);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
							  0, NULL, GL_FALSE);
	}

	sp_debug("GL context created (vsync=%d, debug=%d)", ctx->vsync, ctx->debug);
}

sp_hidden void spel_gfx_context_conf_gl()
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_DEBUG_FLAG, (int)spel.env.debug);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, spel.window.swapchain.depth);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, spel.window.swapchain.stencil);
	const int MSAA_SAMPLES = spel.window.swapchain.msaa;
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, MSAA_SAMPLES > 0 ? 1 : 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, MSAA_SAMPLES > 0 ? MSAA_SAMPLES : 0);
}

sp_hidden void spel_gfx_context_destroy_gl(spel_gfx_context ctx)
{
	for (size_t i = 0; i < sp_array_size(ctx->shaders); i++)
	{
		if (ctx->shaders[i] == NULL)
		{
			continue;
		}

		ctx->shaders[i]->internal = false;
		spel_gfx_shader_destroy_gl(ctx->shaders[i]);
	}

	while (ctx->pipeline_cache.count)
	{
		for (uint32_t i = 0; i < ctx->pipeline_cache.capacity; ++i)
		{
			spel_gfx_pipeline_cache_entry* entry = &ctx->pipeline_cache.entries[i];
			if (entry->pipeline)
			{
				ctx->vt->pipeline_destroy(entry->pipeline);
				break;
			}
		}
	}

	for (uint32_t i = 0; i < ctx->sampler_cache.capacity; ++i)
	{
		spel_gfx_sampler_cache_entry* entry = &ctx->sampler_cache.entries[i];
		if (entry->sampler)
		{
			ctx->vt->sampler_destroy(entry->sampler);
		}
	}

	ctx->white_tex->internal = false;
	ctx->checkerboard->internal = false;

	spel_gfx_texture_destroy(ctx->white_tex);
	spel_gfx_texture_destroy(ctx->checkerboard);

	spel_gfx_cmdlist_destroy_gl(ctx->cmdlist);
	spel_gfx_context_gl* gl = (spel_gfx_context_gl*)ctx->data;
	gladLoaderUnloadGL();
	SDL_GL_DestroyContext(gl->ctx);
	sp_free(ctx->pipeline_cache.entries);
	sp_free(ctx->sampler_cache.entries);
	sp_free(gl);
	ctx->data = NULL;

	sp_debug("GL context destroyed");
}

sp_hidden void spel_gfx_frame_begin_gl(spel_gfx_context ctx)
{
	if (spel.window.occluded)
	{
		return;
	}

	glViewport(0, 0, ctx->fb_width, ctx->fb_height);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBindSampler(0, *(GLuint*)ctx->default_sampler->data);
	glBindTextureUnit(0, *(GLuint*)ctx->white_tex->data);
}

sp_hidden void spel_gfx_frame_end_gl(spel_gfx_context ctx)
{
	if (spel.window.occluded)
	{
		return;
	}
	
	// flush any remaining commands
	spel_gfx_cmdlist_submit_gl(ctx->cmdlist);
	SDL_GL_SwapWindow(spel.window.handle);
}

const static char* gl_source_to_string(GLenum source)
{
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		return "api";
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		return "windowing";
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		return "shaderc";
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		return "3rd";
	case GL_DEBUG_SOURCE_APPLICATION:
		return "app";
	case GL_DEBUG_SOURCE_OTHER:
		return "other";
	default:
		return "unk";
	}
}

const static char* gl_type_to_string(GLenum type)
{
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		return "error";
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return "depr";
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return "undef";
	case GL_DEBUG_TYPE_PORTABILITY:
		return "port";
	case GL_DEBUG_TYPE_PERFORMANCE:
		return "perf";
	case GL_DEBUG_TYPE_MARKER:
		return "mark";
	case GL_DEBUG_TYPE_PUSH_GROUP:
		return "push";
	case GL_DEBUG_TYPE_POP_GROUP:
		return "pop";
	case GL_DEBUG_TYPE_OTHER:
		return "other";
	default:
		return "unk";
	}
}

const static char* gl_severity_to_string(GLenum severity)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		return "high";
	case GL_DEBUG_SEVERITY_MEDIUM:
		return "medium";
	case GL_DEBUG_SEVERITY_LOW:
		return "low";
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		return "notify";
	default:
		return "unk";
	}
}

sp_hidden void spel_gfx_debug_callback(unsigned int source, unsigned int type,
									   unsigned int id, unsigned int severity, int length,
									   const char* message, const void* userParam)
{
	(void)length;
	(void)userParam;

	const char* src = gl_source_to_string(source);
	const char* typ = gl_type_to_string(type);
	const char* sev = gl_severity_to_string(severity);

	spel_gfx_backend_msg msg = {.source = src,
								.source_size = strlen(src),
								.type = typ,
								.type_size = strlen(typ),
								.severity = sev,
								.severity_size = strlen(sev),
								.msg = message,
								.msg_size = strlen(message),
								.id = id};

	if (severity == GL_DEBUG_SEVERITY_HIGH || type == GL_DEBUG_TYPE_ERROR)
	{
		sp_log(SPEL_SEV_ERROR, SPEL_ERR_CONTEXT_FAILED, &msg, SPEL_DATA_GFX_MSG,
			   sizeof(msg), "error from opengl (%s): %s", src, message);
	}
	else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
	{
		sp_log(SPEL_SEV_WARN, SPEL_ERR_NONE, &msg, SPEL_DATA_GFX_MSG, sizeof(msg),
			   "warning from opengl (%s): %s", src, message);
	}
	else if (severity == GL_DEBUG_SEVERITY_LOW)
	{
		sp_log(SPEL_SEV_DEBUG, SPEL_ERR_NONE, &msg, SPEL_DATA_GFX_MSG, sizeof(msg),
			   "debug info from opengl (%s): %s", src, message);
	}
	else
	{
		sp_log(SPEL_SEV_TRACE, SPEL_ERR_NONE, &msg, SPEL_DATA_GFX_MSG, sizeof(msg),
			   "trace info from opengl (%s): %s", src, message);
	}
}
