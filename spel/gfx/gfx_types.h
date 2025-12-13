#ifndef SPEL_GFX_TYPES
#define SPEL_GFX_TYPES
#include <stdint.h>

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
	SPEL_GFX_USAGE_STATIC,
	SPEL_GFX_USAGE_DYNAMIC,
	SPEL_GFX_USAGE_STREAM
} spel_gfx_buffer_usage;

#endif