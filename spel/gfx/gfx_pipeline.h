#ifndef SPEL_GFX_PIPELINE
#define SPEL_GFX_PIPELINE
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

typedef struct spel_gfx_graphic_pipeline_desc
{
	spel_gfx_shader vertex_shader;
	spel_gfx_shader fragment_shader;
	spel_gfx_shader geometry_shader;

	spel_gfx_vertex_layout vertex_layout;

	spel_gfx_primitive_topology topology;
	spel_gfx_cull_mode cull_mode;
	spel_gfx_winding_mode winding;

	bool depth_test;
	bool depth_write;
	spel_gfx_compare_func depth_compare;

} spel_gfx_graphic_pipeline_desc;

#endif