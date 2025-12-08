#ifndef SPEL_GFX_INTERNAL
#define SPEL_GFX_INTERNAL
#include "gfx.h"

typedef struct spel_gfx_vtable spel_gfx_vtable;

typedef struct spel_gfx_context
{
	gfx_backend_type backend;
	bool debug;
	const spel_gfx_vtable* vt;
	void* data;
} spel_gfx_context;

typedef struct spel_gfx_vtable
{
	void (*ctx_destroy)(spel_gfx_context*);
	
	void (*frame_begin)(spel_gfx_context*);
	void (*frame_end)(spel_gfx_context*);
} spel_gfx_vtable;

#endif