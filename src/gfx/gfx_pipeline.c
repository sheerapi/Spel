#include "gfx/gfx_pipeline.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_internal_shaders.h"
#include "gfx/gfx_shader.h"
#include "gfx/gfx_types.h"

spel_gfx_pipeline_desc spel_gfx_pipeline_default()
{
	spel_gfx_pipeline_desc desc;

	desc.vertex_shader = (spel_gfx_shader)0;
	desc.fragment_shader = (spel_gfx_shader)0;
	desc.geometry_shader = (spel_gfx_shader)0;

	desc.vertex_layout.attribs = nullptr;
	desc.vertex_layout.attrib_count = 0;
	desc.vertex_layout.streams = nullptr;
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

	return desc;
}

spel_gfx_pipeline_desc spel_gfx_pipeline_minimal(spel_gfx_shader vertex,
												 spel_gfx_shader fragment)
{
	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();
	desc.vertex_shader = vertex;
	desc.fragment_shader = fragment;

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{.location = 0,
		 .format = spel_gfx_vertex_format_float3,
		 .offset = 0,
		 .stream = 0}};

	static const spel_gfx_vertex_stream STREAMS[] = {
		{.stride = sizeof(float) * 3, .rate = SPEL_GFX_VERTEX_RATE_VERTEX}};

	desc.vertex_layout.attribs = ATTRIBS;
	desc.vertex_layout.attrib_count = 1;
	desc.vertex_layout.streams = STREAMS;
	desc.vertex_layout.stream_count = 1;

	return desc;
}

spel_gfx_pipeline_desc spel_gfx_pipeline_vertex_color(spel_gfx_shader vertex,
													  spel_gfx_shader fragment)
{
	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();
	desc.vertex_shader = vertex;
	desc.fragment_shader = fragment;

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{0, spel_gfx_vertex_format_float2, 0, 0},	// position
		{1, spel_gfx_vertex_format_ubyte4n, 8, 0}}; // color

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

spel_gfx_pipeline_desc spel_gfx_pipeline_textured(spel_gfx_shader vertex,
												  spel_gfx_shader fragment)
{
	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();

	desc.vertex_shader = vertex;
	desc.fragment_shader = fragment;

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{0, spel_gfx_vertex_format_float3, 0, 0}, // position
		{1, spel_gfx_vertex_format_float2, 12, 0} // uv
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

spel_gfx_pipeline_desc spel_gfx_pipeline_fullscreen(spel_gfx_context ctx,
													spel_gfx_shader fragment)
{
	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();

	if (ctx->shaders[2] == nullptr)
	{
		spel_gfx_shader_desc vertex_desc;
		vertex_desc.debug_name = "spel_internal_fullscreen_vertex";
		vertex_desc.entry = "main";
		vertex_desc.stage = SPEL_GFX_SHADER_VERTEX;
		vertex_desc.source = spel_internal_fullscreen_vert_spv;
		vertex_desc.source_size = spel_internal_fullscreen_vert_spv_len;

		ctx->shaders[2] = spel_gfx_shader_create(ctx, &vertex_desc);
		ctx->shaders[2]->internal = true;
	}

	desc.vertex_shader = ctx->shaders[2];
	desc.fragment_shader = fragment;

	desc.vertex_layout.attribs = nullptr;
	desc.vertex_layout.attrib_count = 0;
	desc.vertex_layout.streams = nullptr;
	desc.vertex_layout.stream_count = 0;

	desc.depth_state.depth_test = false;
	desc.depth_state.depth_write = false;

	desc.blend_state.enabled = true;
	desc.blend_state.src_factor = SPEL_GFX_BLEND_SRC_ALPHA;
	desc.blend_state.dst_factor = SPEL_GFX_BLEND_ONE_MINUS_SRC_ALPHA;

	return desc;
}

spel_gfx_pipeline_desc spel_gfx_pipeline_default_2d(spel_gfx_context ctx)
{
	if (ctx->shaders[0] == nullptr)
	{
		spel_gfx_shader_desc vertex_desc;
		vertex_desc.debug_name = "spel_internal_2d_vertex";
		vertex_desc.entry = "main";
		vertex_desc.stage = SPEL_GFX_SHADER_VERTEX;
		vertex_desc.source = spel_internal_2d_vert_spv;
		vertex_desc.source_size = spel_internal_2d_vert_spv_len;

		ctx->shaders[0] = spel_gfx_shader_create(ctx, &vertex_desc);
		ctx->shaders[0]->internal = true;
	}

	if (ctx->shaders[1] == nullptr)
	{
		spel_gfx_shader_desc vertex_desc;
		vertex_desc.debug_name = "spel_internal_2d_fragment";
		vertex_desc.entry = "main";
		vertex_desc.stage = SPEL_GFX_SHADER_FRAGMENT;
		vertex_desc.source = spel_internal_2d_frag_spv;
		vertex_desc.source_size = spel_internal_2d_frag_spv_len;

		ctx->shaders[1] = spel_gfx_shader_create(ctx, &vertex_desc);
		ctx->shaders[1]->internal = true;
	}

	spel_gfx_pipeline_desc desc = spel_gfx_pipeline_default();
	desc.vertex_shader = ctx->shaders[0];
	desc.fragment_shader = ctx->shaders[1];

	static const spel_gfx_vertex_attrib ATTRIBS[] = {
		{0, spel_gfx_vertex_format_float2, 0, 0},	 // position
		{1, spel_gfx_vertex_format_float2, 8, 0},	 // uv
		{2, spel_gfx_vertex_format_ubyte4n, 16, 0}}; // color

	static const spel_gfx_vertex_stream STREAMS[] = {
		{.stride = ((sizeof(float) * 2) + (sizeof(float) * 2) + ((sizeof(char) * 4))),
		 .rate = SPEL_GFX_VERTEX_RATE_VERTEX}};

	desc.vertex_layout.attribs = ATTRIBS;
	desc.vertex_layout.attrib_count = 3;
	desc.vertex_layout.streams = STREAMS;
	desc.vertex_layout.stream_count = 1;

	return desc;
}
