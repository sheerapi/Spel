#include "SDL3/SDL_video.h"
#include "core/entry.h"
#include "core/log.h"
#include "core/types.h"
#include "gfx/gfx_internal.h"
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

	if (ctx->debug)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(spel_gfx_debug_callback, ctx);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
							  0, NULL, GL_FALSE);
	}
}

void spel_gfx_context_conf_gl()
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_DEBUG_FLAG, (int)spel.debug);
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

void spel_gfx_frame_begin_gl(spel_gfx_context ctx)
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void spel_gfx_frame_end_gl(spel_gfx_context ctx)
{
	glFlush();
	SDL_GL_SwapWindow(spel.window.handle);
}

static const char* gl_source_to_string(GLenum source)
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

static const char* gl_type_to_string(GLenum type)
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

static const char* gl_severity_to_string(GLenum severity)
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

void spel_gfx_debug_callback(unsigned int source, unsigned int type, unsigned int id,
							 unsigned int severity, int length, const char* message,
							 const void* userParam)
{
	(void)length;
	(void)userParam;

	const char* src = gl_source_to_string(source);
	const char* typ = gl_type_to_string(type);
	const char* sev = gl_severity_to_string(severity);

	if (severity == GL_DEBUG_SEVERITY_HIGH || type == GL_DEBUG_TYPE_ERROR)
	{
		log_error("[gl][%s][%s][%s][id=%u]\n    %s", sev, src, typ, id, message);
	}
	else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
	{
		log_warn("[gl][%s][%s][%s][id=%u]\n    %s", sev, src, typ, id, message);
	}
	else if (severity == GL_DEBUG_SEVERITY_LOW)
	{
		log_debug("[gl][%s][%s][%s][id=%u]\n    %s", sev, src, typ, id, message);
	}
	else
	{
		log_trace("[GL][%s][%s][%s][id=%u]\n    %s", sev, src, typ, id, message);
	}

#ifdef DEBUG
	if (severity == GL_DEBUG_SEVERITY_HIGH || type == GL_DEBUG_TYPE_ERROR)
	{
		raise(SIGTRAP);
	}
#endif
}