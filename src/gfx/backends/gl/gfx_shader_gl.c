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

	spel_memory_free(shader->entry);

	spel_gfx_shader_reflection* refl = &shader->reflection;

	for (uint32_t j = 0; j < refl->uniform_count; j++)
	{
		spel_gfx_shader_block* block = &refl->uniforms[j];
		spel_memory_free(block->name);
	}

	for (uint32_t j = 0; j < refl->storage_count; j++)
	{
		spel_gfx_shader_block* block = &refl->storage[j];
		spel_memory_free(block->name);
	}

	for (uint32_t j = 0; j < refl->sampler_count; j++)
	{
		spel_gfx_shader_uniform* sampler = &refl->samplers[j];
		spel_memory_free(sampler->name);
	}

	spel_memory_free(refl->samplers);
	spel_memory_free(refl->uniforms);
	spel_memory_free(refl->storage);

	sp_debug("destroyed GL shader %d", (*(spel_gfx_shader_gl*)shader->data).shader);
	glDeleteShader((*(spel_gfx_shader_gl*)shader->data).shader);
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

	sp_debug(
		"created %s GL shader %u (%s, %lu bytes)", spel_gfx_shader_type_str(shader->type),
		(*(spel_gfx_shader_gl*)shader->data).shader, desc->debug_name, desc->source_size);

	return shader;
}