#ifndef SPEL_GFX_RG
#define SPEL_GFX_RG
#include "gfx_types.h"

typedef void (*spel_gfx_rg_setup_fn)(spel_gfx_rg_pass, void*);
typedef void (*spel_gfx_rg_execute_fn)(spel_gfx_cmdlist, spel_gfx_rg_pass, void*);

spel_gfx_rg spel_gfx_rg_create(spel_gfx_context ctx);
void spel_gfx_rg_destroy(spel_gfx_rg rg);

// resource declaration
spel_gfx_rg_resource spel_gfx_rg_create_texture(spel_gfx_rg rg, const char* name,
												uint32_t width, uint32_t height,
												spel_gfx_texture_format format);

spel_gfx_rg_resource spel_gfx_rg_import_texture(spel_gfx_rg rg, const char* name,
												spel_gfx_texture texture);

// pass declaration
spel_gfx_rg_pass spel_gfx_rg_add_pass(spel_gfx_rg rg, const char* name,
									  spel_gfx_rg_setup_fn setup,
									  spel_gfx_rg_execute_fn execute, void* userData);

// resource access (called in setup callback)
void spel_gfx_rg_read(spel_gfx_rg_pass pass, spel_gfx_rg_resource resource);
void spel_gfx_rg_write(spel_gfx_rg_pass pass, spel_gfx_rg_resource resource,
					   spel_gfx_load_op loadOp);

// compilation and execution
void spel_gfx_rg_compile(spel_gfx_rg rg);
void spel_gfx_rg_execute(spel_gfx_rg rg, spel_gfx_cmdlist cl);

// resource access during execution (called in execute callback)
spel_gfx_texture spel_gfx_rg_get_texture(spel_gfx_rg_pass pass,
										 spel_gfx_rg_resource resource);

#endif