#include "gfx/gfx_pipeline.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_shader.h"
#include "gfx/gfx_types.h"
#include "gfx_internal_shaders.h"
#include <string.h>

sp_hidden int32_t spel_gfx_find_block_by_binding(spel_gfx_shader_block* blocks,
												 uint32_t count, uint32_t binding);
sp_hidden int32_t spel_gfx_find_sampler_by_binding(spel_gfx_shader_uniform* samplers,
												   uint32_t count, uint32_t binding);
sp_hidden void spel_gfx_copy_block(spel_gfx_shader_block* dest,
								   const spel_gfx_shader_block* src);
sp_hidden void spel_gfx_copy_sampler(spel_gfx_shader_uniform* dest,
									 const spel_gfx_shader_uniform* src);
sp_hidden void spel_gfx_verify_block_compatibility(spel_gfx_shader_block* existing,
												   const spel_gfx_shader_block* newBlock);
sp_hidden void spel_gfx_verify_sampler_compatibility(
	spel_gfx_shader_uniform* existing, const spel_gfx_shader_uniform* newSampler);

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_default()
{
	spel_gfx_pipeline_desc desc;

	desc.vertex_shader = (spel_gfx_shader)0;
	desc.fragment_shader = (spel_gfx_shader)0;
	desc.geometry_shader = (spel_gfx_shader)0;

	desc.vertex_layout.attribs = NULL;
	desc.vertex_layout.attrib_count = 0;
	desc.vertex_layout.streams = NULL;
	desc.vertex_layout.stream_count = 0;

	desc.topology = SPEL_GFX_TOPOLOGY_TRIANGLES;
	desc.cull_mode = SPEL_GFX_CULL_BACK;
	desc.winding = SPEL_GFX_WINDING_COUNTER_CLOCKWISE;

	desc.blend_state.enabled = false;
	desc.blend_state.color_write_mask = 0xF;
	desc.blend_state.src_factor = SPEL_GFX_BLEND_ONE;
	desc.blend_state.dst_factor = SPEL_GFX_BLEND_ZERO;
	desc.blend_state.operation = SPEL_GFX_BLEND_OP_ADD;
	desc.blend_state.src_alpha_factor = SPEL_GFX_BLEND_ONE;
	desc.blend_state.dst_alpha_factor = SPEL_GFX_BLEND_ZERO;
	desc.blend_state.alpha_op = SPEL_GFX_BLEND_OP_ADD;

	desc.depth_state.depth_test = true;
	desc.depth_state.depth_write = true;
	desc.depth_state.depth_clamp = false;
	desc.depth_state.depth_compare = SPEL_GFX_COMPARE_LESS;

	desc.stencil.enabled = false;
	desc.stencil.compare = SPEL_GFX_COMPARE_ALWAYS;
	desc.stencil.read_mask = 0xFF;
	desc.stencil.write_mask = 0xFF;
	desc.stencil.reference = 0;
	desc.stencil.fail_op = SPEL_GFX_STENCIL_KEEP;
	desc.stencil.depth_fail_op = SPEL_GFX_STENCIL_KEEP;
	desc.stencil.pass_op = SPEL_GFX_STENCIL_KEEP;

	desc.scissor_test = true;

	return desc;
}

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_minimal(spel_gfx_shader vertex,
														spel_gfx_shader fragment)
{
	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();
	desc.vertex_shader = vertex;
	desc.fragment_shader = fragment;

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{.location = 0, .format = sp_gfx_vertex_format_float3, .offset = 0, .stream = 0}};

	static const spel_gfx_vertex_stream STREAMS[] = {
		{.stride = sizeof(float) * 3, .rate = SPEL_GFX_VERTEX_RATE_VERTEX}};

	desc.vertex_layout.attribs = ATTRIBS;
	desc.vertex_layout.attrib_count = 1;
	desc.vertex_layout.streams = STREAMS;
	desc.vertex_layout.stream_count = 1;

	return desc;
}

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_vertex_color(spel_gfx_shader vertex,
															 spel_gfx_shader fragment)
{
	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();
	desc.vertex_shader = vertex;
	desc.fragment_shader = fragment;

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{0, sp_gfx_vertex_format_float2, 0, 0},	  // position
		{1, sp_gfx_vertex_format_ubyte4n, 8, 0}}; // color

	static const spel_gfx_vertex_stream STREAMS[] = {
		{.stride = ((sizeof(float) * 2) + sizeof(char) * 4),
		 .rate = SPEL_GFX_VERTEX_RATE_VERTEX}};

	desc.vertex_layout.attribs = ATTRIBS;
	desc.vertex_layout.attrib_count = 2;
	desc.vertex_layout.streams = STREAMS;
	desc.vertex_layout.stream_count = 1;

	desc.depth_state.depth_test = false;
	desc.depth_state.depth_write = false;

	desc.blend_state.enabled = true;
	desc.blend_state.src_factor = SPEL_GFX_BLEND_SRC_ALPHA;
	desc.blend_state.dst_factor = SPEL_GFX_BLEND_ONE_MINUS_SRC_ALPHA;

	return desc;
}

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_textured(spel_gfx_shader vertex,
														 spel_gfx_shader fragment)
{
	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();

	desc.vertex_shader = vertex;
	desc.fragment_shader = fragment;

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{0, sp_gfx_vertex_format_float3, 0, 0}, // position
		{1, sp_gfx_vertex_format_float2, 12, 0} // uv
	};

	static const spel_gfx_vertex_stream STREAMS[] = {
		{.stride = ((sizeof(float) * 3) + sizeof(float) * 2),
		 .rate = SPEL_GFX_VERTEX_RATE_VERTEX}};

	desc.vertex_layout.attribs = ATTRIBS;
	desc.vertex_layout.attrib_count = 2;
	desc.vertex_layout.streams = STREAMS;
	desc.vertex_layout.stream_count = 1;

	return desc;
}

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_fullscreen(spel_gfx_context ctx,
														   spel_gfx_shader fragment)
{
	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();

	if (ctx->shaders[2] == NULL)
	{
		spel_gfx_shader_desc vertex_desc;
		vertex_desc.debug_name = "spel_internal_fullscreen_vertex";
		vertex_desc.source = spel_internal_fullscreen_vert_spv;
		vertex_desc.source_size = spel_internal_fullscreen_vert_spv_len;

		ctx->shaders[2] = spel_gfx_shader_create(ctx, &vertex_desc);
		ctx->shaders[2]->internal = true;
	}

	desc.vertex_shader = ctx->shaders[2];
	desc.fragment_shader = fragment;

	desc.vertex_layout.attribs = NULL;
	desc.vertex_layout.attrib_count = 0;
	desc.vertex_layout.streams = NULL;
	desc.vertex_layout.stream_count = 0;

	desc.depth_state.depth_test = false;
	desc.depth_state.depth_write = false;

	return desc;
}

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_default_2d(spel_gfx_context ctx)
{
	if (ctx->shaders[0] == NULL)
	{
		spel_gfx_shader_desc vertex_desc;
		vertex_desc.debug_name = "spel_internal_2d_vertex";
		vertex_desc.source = spel_internal_2d_vert_spv;
		vertex_desc.source_size = spel_internal_2d_vert_spv_len;

		ctx->shaders[0] = spel_gfx_shader_create(ctx, &vertex_desc);
		ctx->shaders[0]->internal = true;
	}

	if (ctx->shaders[1] == NULL)
	{
		spel_gfx_shader_desc vertex_desc;
		vertex_desc.debug_name = "spel_internal_2d_fragment";
		vertex_desc.source = spel_internal_2d_frag_spv;
		vertex_desc.source_size = spel_internal_2d_frag_spv_len;

		ctx->shaders[1] = spel_gfx_shader_create(ctx, &vertex_desc);
		ctx->shaders[1]->internal = true;
	}

	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();
	desc.vertex_shader = ctx->shaders[0];
	desc.fragment_shader = ctx->shaders[1];

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{0, sp_gfx_vertex_format_float2, 0, 0},	   // position
		{1, sp_gfx_vertex_format_float2, 8, 0},	   // uv
		{2, sp_gfx_vertex_format_ubyte4n, 16, 0}}; // color

	static const spel_gfx_vertex_stream STREAMS[] = {
		{.stride = ((sizeof(float) * 2) + (sizeof(float) * 2) + ((sizeof(char) * 4))),
		 .rate = SPEL_GFX_VERTEX_RATE_VERTEX}};

	desc.vertex_layout.attribs = ATTRIBS;
	desc.vertex_layout.attrib_count = 3;
	desc.vertex_layout.streams = STREAMS;
	desc.vertex_layout.stream_count = 1;

	desc.depth_state.depth_test = false;
	desc.depth_state.depth_write = false;

	desc.blend_state.enabled = true;
	desc.blend_state.src_factor = SPEL_GFX_BLEND_SRC_ALPHA;
	desc.blend_state.dst_factor = SPEL_GFX_BLEND_ONE_MINUS_SRC_ALPHA;
	desc.blend_state.operation = SPEL_GFX_BLEND_OP_ADD;

	return desc;
}

sp_hidden extern void spel_gfx_pipeline_merge_reflections(spel_gfx_pipeline pipeline,
														  spel_gfx_shader* shaders,
														  uint32_t shaderCount)
{
	uint32_t total_ubos = 0;
	uint32_t total_ssbos = 0;
	uint32_t total_samplers = 0;

	for (uint32_t i = 0; i < shaderCount; i++)
	{
		if (!shaders[i])
		{
			continue;
		}
		total_ubos += shaders[i]->reflection.uniform_count;
		total_ssbos += shaders[i]->reflection.storage_count;
		total_samplers += shaders[i]->reflection.sampler_count;
	}

	spel_gfx_shader_block* all_ubos = NULL;
	spel_gfx_shader_block* all_ssbos = NULL;
	spel_gfx_shader_uniform* all_samplers = NULL;

	if (total_ubos > 0)
	{
		all_ubos = spel_memory_malloc(total_ubos * sizeof(spel_gfx_shader_block),
									  SPEL_MEM_TAG_GFX);
	}
	if (total_ssbos > 0)
	{
		all_ssbos = spel_memory_malloc(total_ssbos * sizeof(spel_gfx_shader_block),
									   SPEL_MEM_TAG_GFX);
	}
	if (total_samplers > 0)
	{
		all_samplers = spel_memory_malloc(
			total_samplers * sizeof(spel_gfx_shader_uniform), SPEL_MEM_TAG_GFX);
	}

	uint32_t ubo_idx = 0;
	uint32_t ssbo_idx = 0;
	uint32_t sampler_idx = 0;

	for (uint32_t i = 0; i < shaderCount; i++)
	{
		if (!shaders[i])
		{
			continue;
		}
		
		spel_gfx_shader_reflection* refl = &shaders[i]->reflection;

		for (uint32_t j = 0; j < refl->uniform_count; j++)
		{
			spel_gfx_shader_block* block = &refl->uniforms[j];

			int32_t existing =
				spel_gfx_find_block_by_binding(all_ubos, ubo_idx, block->binding);

			if (existing >= 0)
			{
				spel_gfx_verify_block_compatibility(&all_ubos[existing], block);
			}
			else
			{
				spel_gfx_copy_block(&all_ubos[ubo_idx++], block);
			}
		}

		for (uint32_t j = 0; j < refl->storage_count; j++)
		{
			spel_gfx_shader_block* block = &refl->storage[j];
			int32_t existing =
				spel_gfx_find_block_by_binding(all_ssbos, ssbo_idx, block->binding);

			if (existing >= 0)
			{
				spel_gfx_verify_block_compatibility(&all_ssbos[existing], block);
			}
			else
			{
				spel_gfx_copy_block(&all_ssbos[ssbo_idx++], block);
			}
		}

		for (uint32_t j = 0; j < refl->sampler_count; j++)
		{
			spel_gfx_shader_uniform* sampler = &refl->samplers[j];
			int32_t existing = spel_gfx_find_sampler_by_binding(all_samplers, sampler_idx,
																sampler->binding);

			if (existing >= 0)
			{
				spel_gfx_verify_sampler_compatibility(&all_samplers[existing], sampler);
			}
			else
			{
				spel_gfx_copy_sampler(&all_samplers[sampler_idx++], sampler);
			}
		}
	}

	pipeline->reflection.uniforms = all_ubos;
	pipeline->reflection.uniform_count = ubo_idx;
	pipeline->reflection.storage = all_ssbos;
	pipeline->reflection.storage_count = ssbo_idx;
	pipeline->reflection.samplers = all_samplers;
	pipeline->reflection.sampler_count = sampler_idx;
}

sp_hidden int32_t spel_gfx_find_block_by_binding(spel_gfx_shader_block* blocks,
												 uint32_t count, uint32_t binding)
{
	for (uint32_t i = 0; i < count; i++)
	{
		if (blocks[i].binding == binding)
		{
			return (int32_t)i;
		}
	}
	return -1;
}

sp_hidden int32_t spel_gfx_find_sampler_by_binding(spel_gfx_shader_uniform* samplers,
												   uint32_t count, uint32_t binding)
{
	for (uint32_t i = 0; i < count; i++)
	{
		if (samplers[i].binding == binding)
		{
			return (int32_t)i;
		}
	}
	return -1;
}

sp_hidden void spel_gfx_copy_block(spel_gfx_shader_block* dest,
								   const spel_gfx_shader_block* src)
{
	memcpy(dest, src, sizeof(spel_gfx_shader_block));
	dest->name = spel_memory_strdup(src->name, SPEL_MEM_TAG_GFX);

	if (src->member_count > 0)
	{
		dest->members = spel_memory_malloc(
			src->member_count * sizeof(spel_gfx_shader_uniform), SPEL_MEM_TAG_GFX);
		memcpy(dest->members, src->members,
			   src->member_count * sizeof(spel_gfx_shader_uniform));

		for (uint32_t i = 0; i < src->member_count; ++i)
		{
			dest->members[i].name =
				spel_memory_strdup(src->members[i].name, SPEL_MEM_TAG_GFX);
		}
	}
}

sp_hidden void spel_gfx_copy_sampler(spel_gfx_shader_uniform* dest,
									 const spel_gfx_shader_uniform* src)
{
	memcpy(dest, src, sizeof(spel_gfx_shader_uniform));
	dest->name = spel_memory_strdup(src->name, SPEL_MEM_TAG_GFX);
}

sp_hidden void spel_gfx_verify_block_compatibility(spel_gfx_shader_block* existing,
												   const spel_gfx_shader_block* newBlock)
{
	if (existing->size != newBlock->size ||
		existing->member_count != newBlock->member_count)
	{
		sp_error(SPEL_ERR_SHADER_REFLECTION_MISMATCH,
				 "UBO binding %u has conflicting definitions across shader stages",
				 existing->binding);
		return;
	}

	for (uint32_t i = 0; i < existing->member_count; i++)
	{
		if (strcmp(existing->members[i].name, newBlock->members[i].name) != 0 ||
			existing->members[i].type != newBlock->members[i].type ||
			existing->members[i].offset != newBlock->members[i].offset)
		{
			sp_error(SPEL_ERR_SHADER_REFLECTION_MISMATCH,
					 "UBO binding %u member mismatch: %s vs %s", existing->binding,
					 existing->members[i].name, newBlock->members[i].name);
		}
	}
}

const char* spel_gfx_uniform_type_to_string(spel_gfx_uniform_type type)
{
	switch (type)
	{
	case SPEL_GFX_UNIFORM_UNKNOWN:
		return "unk";

	case SPEL_GFX_UNIFORM_SAMPLER1D:
		return "sampler1D";

	case SPEL_GFX_UNIFORM_SAMPLER2D:
		return "sampler2D";

	case SPEL_GFX_UNIFORM_SAMPLER3D:
		return "sampler3D";

	case SPEL_GFX_UNIFORM_SAMPLER_CUBE:
		return "samplerCube";
	}
}

sp_hidden void spel_gfx_verify_sampler_compatibility(
	spel_gfx_shader_uniform* existing, const spel_gfx_shader_uniform* newSampler)
{
	if (existing->type != newSampler->type)
	{
		sp_error(SPEL_ERR_SHADER_REFLECTION_MISMATCH,
				 "Sampler binding %u type mismatch: %s in one stage, %s in another",
				 existing->binding, spel_gfx_uniform_type_to_string(existing->type),
				 spel_gfx_uniform_type_to_string(newSampler->type));
	}

	existing->stage_mask |= newSampler->stage_mask;
}
