#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_shader.h"
#include "gfx/gfx_types.h"
#include "gfx_vtable_gl.h"
#include "gl.h"
#include "utils/internal/xxhash.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

spel_gfx_shader spel_gfx_shader_create_spirv_gl(spel_gfx_context ctx,
												spel_gfx_shader_desc* desc);

spel_gfx_shader spel_gfx_shader_create_gl(spel_gfx_context ctx,
										spel_gfx_shader_desc* desc)
{
	return spel_gfx_shader_create_spirv_gl(ctx, desc);
}

void spel_gfx_shader_destroy_gl(spel_gfx_shader shader)
{
	if (shader->internal)
	{
		sp_error(SPEL_ERR_INVALID_RESOURCE, "you can't delete an internal shader!");
		return;
	}

	sp_debug("destroyed GL shader %d", (*(spel_gfx_shader_gl*)shader->data).program);
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
												spel_gfx_shader_desc* desc)
{
	spel_gfx_shader shader =
		(spel_gfx_shader)sp_malloc(sizeof(*shader), SPEL_MEM_TAG_GFX);

	shader->ctx = ctx;

	spel_gfx_shader_reflect(shader, desc);

	XXH3_state_t* state = XXH3_createState();
	XXH3_64bits_reset(state);

	XXH3_64bits_update(state, desc->source, desc->source_size);

	shader->hash = XXH3_64bits_digest(state);
	XXH3_freeState(state);

	shader->data = sp_malloc(sizeof(spel_gfx_shader_gl), SPEL_MEM_TAG_GFX);
	(*(spel_gfx_shader_gl*)shader->data).shader =
		glCreateShader(spel_gfx_shader_stage_to_gl(shader->type));

	if (desc->source_size > (size_t)INT_MAX)
	{
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "shader binary too large");
		sp_free(shader->data);
		sp_free(shader);
		return NULL;
	}
	GLsizei src_size = (GLsizei)desc->source_size;

	glShaderBinary(1, &(*(spel_gfx_shader_gl*)shader->data).shader,
				   GL_SHADER_BINARY_FORMAT_SPIR_V, desc->source, src_size);

	glSpecializeShader((*(spel_gfx_shader_gl*)shader->data).shader, shader->entry, 0,
					   NULL, NULL);

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
		char info_log[512];
		GLsizei info_log_size = 0;
		glGetShaderInfoLog((*(spel_gfx_shader_gl*)shader->data).shader, sizeof(info_log),
						   &info_log_size, (GLchar*)info_log);

		spel_gfx_shader_log log = {.name = desc->debug_name,
								   .name_size = strlen(desc->debug_name),
								   .log = info_log,
								   .log_size = info_log_size};

		sp_log(SPEL_SEV_ERROR, SPEL_ERR_SHADER_FAILED, &log, SPEL_DATA_SHADER_LOG,
			   sizeof(log), "failed to compile shader %s: %s", desc->debug_name,
			   info_log);

		glDeleteShader((*(spel_gfx_shader_gl*)shader->data).shader);
		return NULL;
	}

	sp_debug("created %s GL shader %u (%s, %lu bytes)",
			 spel_gfx_shader_type_str(shader->type),
			 (*(spel_gfx_shader_gl*)shader->data).program, desc->debug_name,
			 desc->source_size);

	glDeleteShader((*(spel_gfx_shader_gl*)shader->data).shader);

	return shader;
}