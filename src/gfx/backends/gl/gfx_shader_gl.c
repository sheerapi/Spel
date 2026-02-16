#include "core/entry.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_shader.h"
#include "gfx/gfx_types.h"
#include "gfx_vtable_gl.h"
#include "gl.h"
#include "utils/internal/spirv_reflect.h"
#include "utils/internal/xxhash.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

spel_gfx_shader spel_gfx_shader_create_spirv_gl(spel_gfx_shader shader,
												spel_gfx_shader_desc* desc);

void spel_gfx_shader_patch_gl(spel_gfx_shader shader, spel_gfx_shader_desc* desc);

spel_gfx_shader spel_gfx_shader_create_gl(spel_gfx_context ctx,
										  spel_gfx_shader_desc* desc)
{
	spel_gfx_shader shader =
		(spel_gfx_shader)sp_malloc(sizeof(*shader), SPEL_MEM_TAG_GFX);

	shader->ctx = ctx;
	spel_gfx_shader_reflect(shader, desc);
	spel_gfx_shader_patch_gl(shader, desc);

	return spel_gfx_shader_create_spirv_gl(shader, desc);
}

void spel_gfx_shader_destroy_gl(spel_gfx_shader shader)
{
	if (shader->internal)
	{
		sp_error(SPEL_ERR_INVALID_RESOURCE, "you can't delete an internal shader!");
		return;
	}

	spel_gfx_shader_reflection_free(shader);
	spel_memory_free(shader->entry);

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

spel_gfx_shader spel_gfx_shader_create_spirv_gl(spel_gfx_shader shader,
												spel_gfx_shader_desc* desc)
{
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

	GLint compiled = GL_FALSE;
	glGetShaderiv((*(spel_gfx_shader_gl*)shader->data).shader, GL_COMPILE_STATUS,
				  &compiled);
	if (compiled != GL_TRUE)
	{
		char info_log[1024];
		GLsizei info_log_size = 0;
		glGetShaderInfoLog((*(spel_gfx_shader_gl*)shader->data).shader,
						   (GLsizei)sizeof(info_log), &info_log_size, info_log);

		spel_gfx_shader_log log = {.name = desc->debug_name,
								   .name_size =
									   desc->debug_name ? strlen(desc->debug_name) : 0,
								   .log = info_log,
								   .log_size = (size_t)info_log_size};

		sp_log(SPEL_SEV_ERROR, SPEL_ERR_SHADER_FAILED, &log, SPEL_DATA_SHADER_LOG,
			   sizeof(log), "shader specialization failed: %s", desc->debug_name);

		spel_gfx_shader_reflection_free(shader);
		spel_memory_free(shader->entry);
		glDeleteShader((*(spel_gfx_shader_gl*)shader->data).shader);
		sp_free(shader->data);
		sp_free(shader);
		return NULL;
	}

	spel_memory_free(desc->source);

	sp_debug(
		"created %s GL shader %u (%s, %lu bytes)", spel_gfx_shader_type_str(shader->type),
		(*(spel_gfx_shader_gl*)shader->data).shader, desc->debug_name, desc->source_size);

	return shader;
}

void spel_gfx_shader_patch_gl(spel_gfx_shader shader, spel_gfx_shader_desc* desc)
{
	SpvReflectShaderModule module;

	if (spvReflectCreateShaderModule(desc->source_size, desc->source, &module) !=
		SPV_REFLECT_RESULT_SUCCESS)
	{
		sp_error(SPEL_ERR_SHADER_REFLECTION_FAILED,
				 "shader reflection failed for shader %s", desc->debug_name);
		return;
	}

	uint32_t binding_count = 0;
	if (spvReflectEnumerateDescriptorBindings(&module, &binding_count, NULL) !=
		SPV_REFLECT_RESULT_SUCCESS)
	{
		sp_error(SPEL_ERR_SHADER_REFLECTION_FAILED,
				 "shader reflection failed for shader %s", desc->debug_name);
		return;
	}

	SpvReflectDescriptorBinding** bindings =
		(SpvReflectDescriptorBinding**)spel_memory_malloc(
			binding_count * sizeof(*bindings), SPEL_MEM_TAG_GFX);

	if (spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings) !=
		SPV_REFLECT_RESULT_SUCCESS)
	{
		sp_error(SPEL_ERR_SHADER_REFLECTION_FAILED,
				 "shader reflection failed for shader %s", desc->debug_name);
		return;
	}

	uint32_t ubo_idx = 0;
	uint32_t ssbo_idx = 0;
	uint32_t sampler_idx = 0;
	uint32_t block_idx = 0;

	for (uint32_t i = 0; i < binding_count; i++)
	{
		SpvReflectDescriptorBinding* binding = bindings[i];

		if (binding->accessed != true)
		{
			sp_warn("shader uniform/block %s is never accessed (%s)", binding->name,
					desc->debug_name);
		}

		switch (binding->descriptor_type)
		{
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		{
			spel_gfx_shader_block* block = &shader->reflection.uniforms[ubo_idx++];
			spvReflectChangeDescriptorBindingNumbers(&module, binding, block_idx, 0);
			block->internal = block_idx;
			break;
		}
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		{
			spel_gfx_shader_block* block = &shader->reflection.uniforms[ssbo_idx++];
			spvReflectChangeDescriptorBindingNumbers(&module, binding, block_idx, 0);
			block->internal = block_idx;
			break;
		}
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		{
			spel_gfx_shader_uniform* sampler =
				&shader->reflection.samplers[sampler_idx++];
			spvReflectChangeDescriptorBindingNumbers(&module, binding, sampler_idx - 1,
													 0);
			sampler->internal = sampler_idx;
			break;
		}
		default:
			break;
		}
	}

	void* newBuf = spel_memory_malloc(spvReflectGetCodeSize(&module), SPEL_MEM_TAG_GFX);
	memcpy(newBuf, spvReflectGetCode(&module), desc->source_size);

	if (desc->shader_source != SPEL_GFX_SHADER_STATIC)
	{
		spel_memory_free(desc->source);
	}

	desc->source = newBuf;

	spel_memory_free((void*)bindings);
	spvReflectDestroyShaderModule(&module);
}