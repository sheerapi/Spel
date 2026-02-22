#ifndef SPEL_IMGUI
#define SPEL_IMGUI
#include "gfx/gfx_types.h"
#include "core/macros.h"

typedef struct spel_imgui_context_t* spel_imgui_context;

spel_api spel_imgui_context spel_imgui_context_create(spel_gfx_context gfx);
spel_api void spel_imgui_context_destroy(spel_imgui_context ctx);

spel_api void spel_imgui_frame_begin(spel_imgui_context ctx);
spel_api void spel_imgui_render(spel_imgui_context ctx, spel_gfx_cmdlist cl);

#endif