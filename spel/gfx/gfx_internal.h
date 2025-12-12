#ifndef SPEL_GFX_INTERNAL
#define SPEL_GFX_INTERNAL
#include "gfx/gfx_types.h"

typedef struct spel_gfx_vtable_t* spel_gfx_vtable;

typedef struct spel_gfx_context_t
{
	spel_gfx_backend backend;
	spel_gfx_vtable vt;
	bool debug;
	int vsync;
	
	void* data;
} spel_gfx_context_t;

typedef struct spel_gfx_vtable_t
{
	void (*ctx_destroy)(spel_gfx_context);
} spel_gfx_vtable_t;

extern void spel_gfx_context_create_gl(spel_gfx_context ctx);

#endif