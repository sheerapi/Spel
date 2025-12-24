#ifndef SPEL_GFX_PIPELINE
#define SPEL_GFX_PIPELINE
#include "core/macros.h"
#include "gfx/gfx_types.h"

typedef struct
{
	uint32_t location;
	spel_gfx_vertex_format format;
	uint32_t offset;
	uint32_t stream;
} spel_gfx_vertex_attrib;

typedef struct
{
	uint32_t stride;
	spel_gfx_vertex_rate rate;
} spel_gfx_vertex_stream;

typedef struct
{
	const spel_gfx_vertex_attrib* attribs;
	uint32_t attrib_count;

	const spel_gfx_vertex_stream* streams;
	uint32_t stream_count;
} spel_gfx_vertex_layout;

typedef struct
{
	bool enabled;
	uint8_t color_write_mask;

	spel_gfx_blend_factor src_factor;
	spel_gfx_blend_factor dst_factor;
	spel_gfx_blend_op operation;

	spel_gfx_blend_factor src_alpha_factor;
	spel_gfx_blend_factor dst_alpha_factor;
	spel_gfx_blend_op alpha_op;
} spel_gfx_blend_state;

typedef struct
{
	bool enabled;

	spel_gfx_compare_func compare;
	uint8_t read_mask;
	uint8_t write_mask;
	uint8_t reference;

	spel_gfx_stencil_op fail_op;
	spel_gfx_stencil_op depth_fail_op;
	spel_gfx_stencil_op pass_op;
} spel_gfx_stencil_state;

typedef struct
{
	bool depth_test;
	bool depth_write;
	bool depth_clamp;
	spel_gfx_compare_func depth_compare;
} spel_gfx_depth_state;

typedef struct spel_gfx_pipeline_desc
{
	spel_gfx_shader vertex_shader;
	spel_gfx_shader fragment_shader;
	spel_gfx_shader geometry_shader;

	spel_gfx_vertex_layout vertex_layout;

	spel_gfx_primitive_topology topology;
	spel_gfx_cull_mode cull_mode;
	spel_gfx_winding_mode winding;

	bool scissor_test;
	spel_gfx_blend_state blend_state;
	spel_gfx_depth_state depth_state;
	spel_gfx_stencil_state stencil;

} spel_gfx_pipeline_desc;

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_default();

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_minimal(spel_gfx_shader vertex,
														spel_gfx_shader fragment);

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_vertex_color(spel_gfx_shader vertex,
															 spel_gfx_shader fragment);

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_textured(spel_gfx_shader vertex,
														 spel_gfx_shader fragment);

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_fullscreen(spel_gfx_context ctx,
														   spel_gfx_shader fragment);

sp_api spel_gfx_pipeline_desc spel_gfx_pipeline_default_2d(spel_gfx_context ctx);

sp_api spel_gfx_pipeline spel_gfx_pipeline_create(spel_gfx_context ctx,
												  const spel_gfx_pipeline_desc* desc);
sp_api void spel_gfx_pipeline_destroy(spel_gfx_pipeline pipeline);

#endif