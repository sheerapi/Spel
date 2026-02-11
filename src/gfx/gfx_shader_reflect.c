#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx_internal.h"
#include "utils/internal/spirv_reflect.h"
#include <stdio.h>

#define SPIR_V_MAGIC 0x07230203
#define OP_DECORATE 71
#define OP_TYPE_POINTER 32
#define DECORATION_LOCATION 30

spel_gfx_shader_stage spel_gfx_spvreflect_stage_to_spel(
	SpvReflectShaderStageFlagBits bits)
{
	switch (bits)
	{
	case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
		return SPEL_GFX_SHADER_VERTEX;

	case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
		return SPEL_GFX_SHADER_GEOMETRY;

	case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
		return SPEL_GFX_SHADER_FRAGMENT;

	case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
		return SPEL_GFX_SHADER_COMPUTE;

	default:
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "unsupported shader type %x", bits);
		return SPEL_GFX_SHADER_VERTEX;
	}
}

sp_hidden void spel_gfx_reflect_fill_block(spel_gfx_shader_block* block,
										   SpvReflectDescriptorBinding* binding,
										   spel_gfx_buffer_type type,
										   const spel_gfx_shader_desc* desc,
										   spel_gfx_shader shader);
sp_hidden uint32_t spel_gfx_count_block_members(SpvReflectBlockVariable* block,
												const spel_gfx_shader_desc* desc);
sp_hidden void spel_gfx_flatten_block_members(SpvReflectBlockVariable* var,
											  const char* prefix,
											  spel_gfx_shader_uniform* uniforms,
											  uint32_t* index, spel_gfx_shader shader);

sp_hidden void spel_gfx_shader_reflect(spel_gfx_shader shader, spel_gfx_shader_desc* desc)
{
	SpvReflectShaderModule module;

	if (spvReflectCreateShaderModule(desc->source_size, desc->source, &module) !=
		SPV_REFLECT_RESULT_SUCCESS)
	{
		sp_error(SPEL_ERR_SHADER_REFLECTION_FAILED,
				 "shader reflection failed for shader %s", desc->debug_name);
		return;
	}

	shader->entry = spel_memory_strdup(module.entry_point_name, SPEL_MEM_TAG_GFX);
	shader->type = spel_gfx_spvreflect_stage_to_spel(module.shader_stage);

	uint32_t binding_count = 0;
	if (spvReflectEnumerateDescriptorBindings(&module, &binding_count, NULL) !=
		SPV_REFLECT_RESULT_SUCCESS)
	{
		sp_error(SPEL_ERR_SHADER_REFLECTION_FAILED,
				 "shader reflection failed for shader %s", desc->debug_name);
		return;
	}

	SpvReflectDescriptorBinding* bindings = spel_memory_malloc(
		binding_count * sizeof(SpvReflectDescriptorBinding), SPEL_MEM_TAG_GFX);

	if (spvReflectEnumerateDescriptorBindings(&module, &binding_count, &bindings) !=
		SPV_REFLECT_RESULT_SUCCESS)
	{
		sp_error(SPEL_ERR_SHADER_REFLECTION_FAILED,
				 "shader reflection failed for shader %s", desc->debug_name);
		return;
	}

	uint32_t ubo_count = 0;
	uint32_t ssbo_count = 0;
	uint32_t sampler_count = 0;

	for (uint32_t i = 0; i < binding_count; i++)
	{
		switch (bindings[i].descriptor_type)
		{
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			ubo_count++;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			ssbo_count++;
			break;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			sampler_count++;
			break;
		default:
			break;
		}
	}

	shader->reflection.uniform_count = ubo_count;
	shader->reflection.storage_count = ssbo_count;
	shader->reflection.sampler_count = sampler_count;

	if (ubo_count > 0)
	{
		shader->reflection.uniforms = spel_memory_malloc(
			ubo_count * sizeof(spel_gfx_shader_block), SPEL_MEM_TAG_GFX);
	}

	if (ssbo_count > 0)
	{
		shader->reflection.storage = spel_memory_malloc(
			ssbo_count * sizeof(spel_gfx_shader_block), SPEL_MEM_TAG_GFX);
	}

	if (sampler_count > 0)
	{
		shader->reflection.samplers = spel_memory_malloc(
			sampler_count * sizeof(spel_gfx_shader_uniform), SPEL_MEM_TAG_GFX);
	}

	uint32_t ubo_idx = 0;
	uint32_t ssbo_idx = 0;
	uint32_t sampler_idx = 0;

	uint32_t sampler_ids[sampler_count];
	uint32_t sampler_bindings[sampler_count];

	for (uint32_t i = 0; i < binding_count; i++)
	{
		SpvReflectDescriptorBinding* binding = &bindings[i];

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
			spel_gfx_reflect_fill_block(block, binding, SPEL_GFX_BUFFER_UNIFORM, desc,
										shader);
			break;
		}
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		{
			spel_gfx_shader_block* block = &shader->reflection.storage[ssbo_idx++];
			spel_gfx_reflect_fill_block(block, binding, SPEL_GFX_BUFFER_STORAGE, desc,
										shader);
			break;
		}
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		{
			spel_gfx_shader_uniform* sampler =
				&shader->reflection.samplers[sampler_idx++];
			sampler->name = spel_memory_strdup(binding->name, SPEL_MEM_TAG_GFX);
			sampler->binding = binding->binding;
			sampler->array_count = binding->count;
			sampler->stage_mask = shader->type;
			sampler->type = (spel_gfx_uniform_type)(binding->image.dim + 1);
			sampler_ids[sampler_idx - 1] = binding->spirv_id;
			sampler_bindings[sampler_idx - 1] = binding->binding;
			break;
		}
		default:
			break;
		}
	}

	spvReflectDestroyShaderModule(&module);
}

sp_hidden void spel_gfx_reflect_fill_block(spel_gfx_shader_block* block,
										   SpvReflectDescriptorBinding* binding,
										   spel_gfx_buffer_type type,
										   const spel_gfx_shader_desc* desc,
										   spel_gfx_shader shader)
{
	block->name =
		spel_memory_strdup(binding->type_description->type_name, SPEL_MEM_TAG_GFX);
	block->binding = binding->binding;
	block->size = binding->block.size;
	block->type = type;

	uint32_t total_members = spel_gfx_count_block_members(&binding->block, desc);

	block->members = spel_memory_malloc(total_members * sizeof(spel_gfx_shader_uniform),
										SPEL_MEM_TAG_GFX);
	block->member_count = 0;

	spel_gfx_flatten_block_members(&binding->block, "", block->members,
								   &block->member_count, shader);
}

sp_hidden uint32_t spel_gfx_count_block_members(SpvReflectBlockVariable* block,
												const spel_gfx_shader_desc* desc)
{
	if (block->member_count == 0)
	{
		return 1;
	}

	uint32_t count = 0;
	for (uint32_t i = 0; i < block->member_count; i++)
	{
		if (block->members[i].flags == SPV_REFLECT_VARIABLE_FLAGS_UNUSED)
		{
			sp_warn("shader block member %s%s%s is never accessed (%s)", block->name,
					strlen(block->name) != 0 ? "." : "", block->members[i].name,
					desc->debug_name);
		}
		count += spel_gfx_count_block_members(&block->members[i], desc);
	}

	return count;
}

sp_hidden void spel_gfx_flatten_block_members(SpvReflectBlockVariable* var,
											  const char* prefix,
											  spel_gfx_shader_uniform* uniforms,
											  uint32_t* index, spel_gfx_shader shader)
{
	char full_name[128];

	if (var->member_count > 0)
	{
		for (uint32_t i = 0; i < var->member_count; i++)
		{
			snprintf(full_name, sizeof(full_name), "%s%s%s", prefix,
					 strlen(prefix) > 0 ? "." : "", var->name);
			spel_gfx_flatten_block_members(&var->members[i], full_name, uniforms, index,
										   shader);
		}
	}
	else
	{
		snprintf(full_name, sizeof(full_name), "%s%s%s", prefix,
				 strlen(prefix) > 0 ? "." : "", var->name);

		spel_gfx_shader_uniform* uniform = &uniforms[(*index)++];
		uniform->name = spel_memory_strdup(full_name, SPEL_MEM_TAG_GFX);
		uniform->offset = var->offset;
		uniform->size = var->size;
		uniform->stage_mask = shader->type;
		uniform->array_count = var->array.dims_count > 0 ? var->array.dims[0] : 1;
	}
}