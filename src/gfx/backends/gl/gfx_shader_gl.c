#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_shader.h"
#include "gfx/gfx_types.h"
#include "gfx_vtable_gl.h"
#include "gl.h"

spel_gfx_shader spel_gfx_shader_create_spirv_gl(spel_gfx_context ctx,
												const spel_gfx_shader_desc* desc);

spel_gfx_shader spel_gfx_shader_create_gl(spel_gfx_context ctx,
										  const spel_gfx_shader_desc* desc)
{
	return spel_gfx_shader_create_spirv_gl(ctx, desc);
}

void spel_gfx_shader_destroy_gl(spel_gfx_shader shader)
{
	glDeleteProgram((*(spel_gfx_shader_gl*)shader->data).program);
	sp_free(shader->data);
	sp_free(shader);
}

GLenum spel_gfx_shader_stage_to_gl(spel_gfx_shader_stage stage)
{
	switch (stage)
	{
	case SPEL_GFX_SHADER_VERTEX:
		return GL_VERTEX_SHADER;
		break;

	case SPEL_GFX_SHADER_FRAGMENT:
		return GL_FRAGMENT_SHADER;
		break;

	case SPEL_GFX_SHADER_GEOMETRY:
		return GL_GEOMETRY_SHADER;
		break;

	case SPEL_GFX_SHADER_COMPUTE:
		return GL_COMPUTE_SHADER;
		break;
	}

	return GL_VERTEX_SHADER;
}

spel_gfx_shader spel_gfx_shader_create_spirv_gl(spel_gfx_context ctx,
												const spel_gfx_shader_desc* desc)
{
	spel_gfx_shader shader =
		(spel_gfx_shader)sp_malloc(sizeof(*shader), SPEL_MEM_TAG_GFX);

	shader->ctx = ctx;
	shader->type = desc->stage;

	shader->data = sp_malloc(sizeof(spel_gfx_shader_gl), SPEL_MEM_TAG_GFX);
	(*(spel_gfx_shader_gl*)shader->data).shader =
		glCreateShader(spel_gfx_shader_stage_to_gl(desc->stage));

	glShaderBinary(1, &(*(spel_gfx_shader_gl*)shader->data).shader,
				   GL_SHADER_BINARY_FORMAT_SPIR_V, desc->source, desc->source_size);

	glSpecializeShader((*(spel_gfx_shader_gl*)shader->data).shader, desc->entry, 0,
					   nullptr, nullptr);

	(*(spel_gfx_shader_gl*)shader->data).program = glCreateProgram();

	glAttachShader((*(spel_gfx_shader_gl*)shader->data).program,
				   (*(spel_gfx_shader_gl*)shader->data).shader);

	glProgramParameteri((*(spel_gfx_shader_gl*)shader->data).program,
						GL_PROGRAM_SEPARABLE, GL_TRUE);

	glLinkProgram((*(spel_gfx_shader_gl*)shader->data).program);

	int status;
	glGetShaderiv((*(spel_gfx_shader_gl*)shader->data).shader, GL_COMPILE_STATUS,
				  &status);

	if (status != GL_TRUE)
	{
		char* info_log = nullptr;
		size_t info_log_size;
		glGetShaderInfoLog((*(spel_gfx_shader_gl*)shader->data).shader, 512,
						   &info_log_size, info_log);
		char buffer[256];
		snprintf(buffer, 256, "gfx: failed to compile shader %s: %s", desc->debug_name,
				 info_log);
		spel_error(buffer);
	}

	glDeleteShader((*(spel_gfx_shader_gl*)shader->data).shader);

	return shader;
}