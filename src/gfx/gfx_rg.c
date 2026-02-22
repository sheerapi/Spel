#include "gfx/gfx_rg.h"
#include "core/memory.h"
#include "gfx/gfx_internal.h"

spel_api spel_gfx_rg spel_gfx_rg_create(spel_gfx_context gfx)
{
	spel_gfx_rg rg = spel_memory_malloc(sizeof(*rg), SPEL_MEM_TAG_GFX);

	rg->ctx = gfx;

	rg->resource_cap = 4;
	rg->sorted_pass_cap = 8;
	rg->pass_cap = 8;

	rg->resources =
		spel_memory_malloc(sizeof(*rg->resources) * rg->resource_cap, SPEL_MEM_TAG_GFX);

	rg->sorted_passes = spel_memory_malloc(
		sizeof(*rg->sorted_passes) * rg->sorted_pass_cap, SPEL_MEM_TAG_GFX);

	rg->passes = spel_memory_malloc(sizeof(*rg->passes) * rg->pass_cap, SPEL_MEM_TAG_GFX);

	return rg;
}

spel_api void spel_gfx_rg_destroy(spel_gfx_rg rg)
{
	spel_memory_free(rg->resources);
	spel_memory_free(rg->sorted_passes);
	spel_memory_free(rg->passes);
	spel_memory_free(rg);
}

spel_api void spel_gfx_rg_reset(spel_gfx_rg rg)
{
}

spel_api void spel_gfx_rg_compile(spel_gfx_rg rg)
{
}

spel_api void spel_gfx_rg_execute(spel_gfx_rg rg, spel_gfx_cmdlist cl)
{
}