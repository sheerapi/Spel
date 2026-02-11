#ifndef SPEL_GFX_TYPES
#define SPEL_GFX_TYPES
#include <stddef.h>
#include <stdint.h>

typedef struct spel_gfx_context_t* spel_gfx_context;
typedef struct spel_gfx_buffer_t* spel_gfx_buffer;
typedef struct spel_gfx_cmdlist_t* spel_gfx_cmdlist;
typedef struct spel_gfx_pipeline_t* spel_gfx_pipeline;
typedef struct spel_gfx_shader_t* spel_gfx_shader;
typedef struct spel_gfx_texture_t* spel_gfx_texture;
typedef struct spel_gfx_sampler_t* spel_gfx_sampler;
typedef struct spel_gfx_uniform_t* spel_gfx_uniform;

typedef enum
{
	SPEL_GFX_BACKEND_OPENGL
} spel_gfx_backend;

typedef enum
{
	SPEL_GFX_BUFFER_VERTEX,
	SPEL_GFX_BUFFER_INDEX,
	SPEL_GFX_BUFFER_UNIFORM,
	SPEL_GFX_BUFFER_STORAGE
} spel_gfx_buffer_type;

typedef enum
{
	SPEL_GFX_BUFFER_DRAW,
	SPEL_GFX_BUFFER_READ,
	SPEL_GFX_BUFFER_COPY,
} spel_gfx_buffer_access;

typedef uint32_t spel_gfx_access;

#define sp_gfx_access_read (1u << 0)
#define sp_gfx_access_write (1u << 1)

#define sp_gfx_access_invalidate_range (1u << 2)
#define sp_gfx_access_invalidate_buffer (1u << 3)
#define sp_gfx_access_unsynchronized (1u << 4)
#define sp_gfx_access_flush_explicit (1u << 5)
#define sp_gfx_access_persistent (1u << 6)
#define sp_gfx_access_coherent (1u << 7)

typedef enum
{
	SPEL_GFX_USAGE_STREAM,
	SPEL_GFX_USAGE_STATIC,
	SPEL_GFX_USAGE_DYNAMIC,
} spel_gfx_buffer_usage;

typedef enum
{
	SPEL_GFX_INDEX_U16,
	SPEL_GFX_INDEX_U32
} spel_gfx_index_type;

typedef enum
{
	SPEL_GFX_VERTEX_FLOAT,
	SPEL_GFX_VERTEX_HALF,
	SPEL_GFX_VERTEX_INT,
	SPEL_GFX_VERTEX_UINT
} spel_gfx_vertex_base_format;

typedef enum
{
	SPEL_GFX_VERTEX_NORMALIZED = 1 << 0,
	SPEL_GFX_VERTEX_INTEGER = 1 << 1,
} spel_gfx_vertex_flags;

typedef enum
{
	SPEL_GFX_VERTEX_RATE_VERTEX,
	SPEL_GFX_VERTEX_RATE_INSTANCE
} spel_gfx_vertex_rate;

typedef enum
{
	SPEL_GFX_TOPOLOGY_POINTS,
	SPEL_GFX_TOPOLOGY_LINES,
	SPEL_GFX_TOPOLOGY_LINE_STRIP,
	SPEL_GFX_TOPOLOGY_TRIANGLES,
	SPEL_GFX_TOPOLOGY_TRIANGLE_STRIP
} spel_gfx_primitive_topology;

typedef enum
{
	SPEL_GFX_CULL_NONE,
	SPEL_GFX_CULL_FRONT,
	SPEL_GFX_CULL_BACK
} spel_gfx_cull_mode;

typedef enum
{
	SPEL_GFX_WINDING_CLOCKWISE,
	SPEL_GFX_WINDING_COUNTER_CLOCKWISE
} spel_gfx_winding_mode;

typedef enum
{
	SPEL_GFX_COMPARE_NEVER,
	SPEL_GFX_COMPARE_LESS,
	SPEL_GFX_COMPARE_EQUAL,
	SPEL_GFX_COMPARE_LEQUAL,
	SPEL_GFX_COMPARE_GREATER,
	SPEL_GFX_COMPARE_NOTEQUAL,
	SPEL_GFX_COMPARE_GEQUAL,
	SPEL_GFX_COMPARE_ALWAYS
} spel_gfx_compare_func;

typedef enum
{
	SPEL_GFX_BLEND_ZERO,
	SPEL_GFX_BLEND_ONE,
	SPEL_GFX_BLEND_SRC_COLOR,
	SPEL_GFX_BLEND_ONE_MINUS_SRC_COLOR,
	SPEL_GFX_BLEND_DST_COLOR,
	SPEL_GFX_BLEND_ONE_MINUS_DST_COLOR,
	SPEL_GFX_BLEND_SRC_ALPHA,
	SPEL_GFX_BLEND_ONE_MINUS_SRC_ALPHA,
	SPEL_GFX_BLEND_DST_ALPHA,
	SPEL_GFX_BLEND_ONE_MINUS_DST_ALPHA
} spel_gfx_blend_factor;

typedef enum
{
	SPEL_GFX_BLEND_OP_ADD,
	SPEL_GFX_BLEND_OP_SUBTRACT,
	SPEL_GFX_BLEND_OP_REV_SUBTRACT,
	SPEL_GFX_BLEND_OP_MIN,
	SPEL_GFX_BLEND_OP_MAX
} spel_gfx_blend_op;

typedef enum
{
	SPEL_GFX_STENCIL_KEEP,
	SPEL_GFX_STENCIL_ZERO,
	SPEL_GFX_STENCIL_REPLACE,
	SPEL_GFX_STENCIL_INCREASE,
	SPEL_GFX_STENCIL_INCR_WRAP,
	SPEL_GFX_STENCIL_DECREASE,
	SPEL_GFX_STENCIL_DECR_WRAP,
	SPEL_GFX_STENCIL_INVERT
} spel_gfx_stencil_op;

typedef enum
{
	SPEL_GFX_SHADER_VERTEX,
	SPEL_GFX_SHADER_FRAGMENT,
	SPEL_GFX_SHADER_GEOMETRY,
	SPEL_GFX_SHADER_COMPUTE
} spel_gfx_shader_stage;

typedef uint32_t spel_gfx_vertex_format;

#define sp_vtx_fmt(base, comps, bits, flags)                                             \
	((spel_gfx_vertex_format)(((base) & 0x3) | (((comps) - 1) << 2) |                    \
							  (((bits) & 0xFF) << 4) | ((flags) << 12)))

#define sp_vtx_base(fmt) ((spel_gfx_vertex_base_format)(((fmt) >> 0) & 0x3))

#define sp_vtx_comps(fmt) ((((fmt) >> 2) & 0x3) + 1)

#define sp_vtx_bits(fmt) (((fmt) >> 4) & 0xFF)

#define sp_vtx_flags(fmt) ((fmt) >> 12)

#define sp_gfx_vertex_format_float2 sp_vtx_fmt(SPEL_GFX_VERTEX_FLOAT, 2, 32, 0)
#define sp_gfx_vertex_format_float3 sp_vtx_fmt(SPEL_GFX_VERTEX_FLOAT, 3, 32, 0)
#define sp_gfx_vertex_format_float4 sp_vtx_fmt(SPEL_GFX_VERTEX_FLOAT, 4, 32, 0)

#define sp_gfx_vertex_format_ubyte4n                                                     \
	sp_vtx_fmt(SPEL_GFX_VERTEX_UINT, 4, 8, SPEL_GFX_VERTEX_NORMALIZED)

typedef enum
{
	SPEL_GFX_TEXTURE_2D,
	SPEL_GFX_TEXTURE_2D_ARRAY,
	SPEL_GFX_TEXTURE_3D,
	SPEL_GFX_TEXTURE_CUBE
} spel_gfx_texture_type;

typedef enum
{
	SPEL_GFX_TEXTURE_USAGE_SAMPLED = 1 << 0,
	SPEL_GFX_TEXTURE_USAGE_STORAGE = 1 << 1,
	SPEL_GFX_TEXTURE_USAGE_RENDER = 1 << 2,
} spel_gfx_texture_usage;

typedef enum
{
	SPEL_GFX_TEXTURE_FMT_UNKNOWN,

	SPEL_GFX_TEXTURE_FMT_R8_UNORM,
	SPEL_GFX_TEXTURE_FMT_RG8_UNORM,
	SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
	SPEL_GFX_TEXTURE_FMT_RGBA8_SRGB,

	SPEL_GFX_TEXTURE_FMT_R16F,
	SPEL_GFX_TEXTURE_FMT_RG16F,
	SPEL_GFX_TEXTURE_FMT_RGBA16F,

	SPEL_GFX_TEXTURE_FMT_R32F,
	SPEL_GFX_TEXTURE_FMT_RG32F,
	SPEL_GFX_TEXTURE_FMT_RGBA32F,

	SPEL_GFX_TEXTURE_FMT_D16,
	SPEL_GFX_TEXTURE_FMT_D24S8,
	SPEL_GFX_TEXTURE_FMT_D32F,

	SPEL_GFX_TEXTURE_FORMAT_COUNT
} spel_gfx_texture_format;

typedef enum
{
	SPEL_GFX_SAMPLER_FILTER_NEAREST,
	SPEL_GFX_SAMPLER_FILTER_LINEAR,
} spel_gfx_sampler_filter;

typedef enum
{
	SPEL_GFX_SAMPLER_MIP_NONE,
	SPEL_GFX_SAMPLER_MIP_NEAREST,
	SPEL_GFX_SAMPLER_MIP_LINEAR,
} spel_gfx_sampler_mip_filter;

typedef enum
{
	SPEL_GFX_SAMPLER_WRAP_REPEAT,
	SPEL_GFX_SAMPLER_WRAP_CLAMP,
	SPEL_GFX_SAMPLER_WRAP_MIRROR,
} spel_gfx_sampler_wrap;

typedef enum
{
	SPEL_GFX_UNIFORM_UNKNOWN,

	SPEL_GFX_UNIFORM_SAMPLER1D,
	SPEL_GFX_UNIFORM_SAMPLER2D,
	SPEL_GFX_UNIFORM_SAMPLER3D,
	SPEL_GFX_UNIFORM_SAMPLER_CUBE,
} spel_gfx_uniform_type;

typedef struct
{
	const char* name;
	size_t name_size;

	const char* log;
	size_t log_size;
} spel_gfx_shader_log;

typedef struct
{
	const char* severity;
	uint8_t severity_size;
	const char* type;
	uint8_t type_size;
	const char* source;
	uint8_t source_size;
	const char* msg;
	uint16_t msg_size;
	unsigned int id;
} spel_gfx_backend_msg;

#endif
