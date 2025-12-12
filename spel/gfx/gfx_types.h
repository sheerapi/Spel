#ifndef SPEL_GFX_TYPES
#define SPEL_GFX_TYPES

typedef struct spel_gfx_context_t* spel_gfx_context;
typedef struct spel_gfx_buffer_t* spel_gfx_buffer;
typedef struct spel_gfx_cmdlist_t* spel_gfx_cmdlist;

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
	SPEL_GFX_USAGE_STATIC,
	SPEL_GFX_USAGE_DYNAMIC,
	SPEL_GFX_USAGE_STREAM
} spel_gfx_buffer_usage;

#endif