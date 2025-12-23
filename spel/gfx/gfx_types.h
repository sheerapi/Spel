#ifndef SPEL_GFX_TYPES
#define SPEL_GFX_TYPES
#include <stdint.h>

typedef struct spel_gfx_context_t* spel_gfx_context;
typedef struct spel_gfx_buffer_t* spel_gfx_buffer;
typedef struct spel_gfx_cmdlist_t* spel_gfx_cmdlist;
typedef struct spel_gfx_graphic_pipeline_t* spel_gfx_graphic_pipeline;
typedef struct spel_gfx_shader_t* spel_gfx_shader;

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
	SPEL_GFX_VERTEX_PER_VERTEX,
	SPEL_GFX_VERTEX_PER_INSTANCE
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

#endif