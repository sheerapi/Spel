#include "extras/spel_imgui.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_mouse.h"
#include "core/log.h"
#include "core/memory.h"
#include "core/window.h"
#include "dcimgui.h"
#include "dcimgui_impl_sdl3.h"
#include "gfx/gfx.h"
#include "gfx/gfx_cmdlist.h"
#include "gfx/gfx_pipeline.h"
#include "gfx_internal_shaders.h"
#include <stddef.h>

typedef struct spel_imgui_context_t
{
	spel_gfx_context gfx;

	ImGuiContext* context;
	ImGuiIO* io;
	ImGuiPlatformIO* platform_io;

	spel_gfx_pipeline pipeline;
	spel_gfx_shader vtx_shader;
	spel_gfx_shader frag_shader;

	spel_gfx_buffer vbuffer;
	spel_gfx_buffer ibuffer;

	spel_gfx_uniform matrix_handle;
	spel_gfx_uniform_buffer ubuffer;

	spel_gfx_texture atlas;
} spel_imgui_context_t;

sp_hidden void spel_imgui_resources_create(spel_imgui_context ctx);
sp_hidden void spel_imgui_texture_update(spel_imgui_context ctx, ImTextureData* texture);
sp_hidden void spel_imgui_buffers_check(spel_imgui_context ctx, ImDrawData* drawData);
sp_hidden void spel_imgui_state_update(spel_imgui_context ctx, spel_gfx_cmdlist cl,
									   ImDrawData* drawData);

sp_hidden bool spel_imgui_event_callback(void* event, void* ctx);

sp_api spel_imgui_context spel_imgui_context_create(spel_gfx_context gfx)
{
	spel_imgui_context ctx = spel_memory_malloc(sizeof(*ctx), SPEL_MEM_TAG_MISC);

	ctx->gfx = gfx;

	ctx->context = ImGui_CreateContext(NULL);
	ImGui_SetCurrentContext(ctx->context);
	ctx->io = ImGui_GetIO();
	ctx->platform_io = ImGui_GetPlatformIO();

	ImFontAtlas_AddFontDefaultVector(ctx->io->Fonts, NULL);

	ctx->io->BackendRendererName = "spel_gfx";

	ctx->io->DisplaySize = (ImVec2){(float)spel.window.width, (float)spel.window.height};

	ctx->io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	ctx->io->BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	ctx->io->BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	ctx->io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ctx->io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ctx->pipeline = NULL;
	ctx->vbuffer = NULL;
	ctx->ibuffer = NULL;

	cImGui_ImplSDL3_InitForOther(spel.window.handle);

	spel_event_register(SPEL_EVENT_INTERNAL_INPUT_SDL_EVENT, spel_imgui_event_callback,
						ctx);

	sp_debug("initialized imgui");

	return ctx;
}

sp_api void spel_imgui_frame_begin(spel_imgui_context ctx)
{
	ImGui_SetCurrentContext(ctx->context);

	spel_vec2 fb_size = spel_window_framebuffer_size();
	spel_vec2 dpi = spel_window_dpi();

	ctx->io->DisplaySize = (ImVec2){fb_size.x, fb_size.y};
	ctx->io->DisplayFramebufferScale = (ImVec2){dpi.x, dpi.y};

	if (ctx->pipeline == NULL)
	{
		spel_imgui_resources_create(ctx);
	}

	cImGui_ImplSDL3_NewFrame();

	ImGui_NewFrame();
}

sp_api void spel_imgui_render(spel_imgui_context ctx, spel_gfx_cmdlist cl)
{
	ImGui_Render();
	ImDrawData* draw_data = ImGui_GetDrawData();

	if (draw_data == NULL || draw_data->DisplaySize.x <= 0.0F || draw_data->DisplaySize.y <= 0.0F)
	{
		return;
	}

	spel_imgui_buffers_check(ctx, draw_data);

	if (ctx->vbuffer == NULL || ctx->ibuffer == NULL)
	{
		return;
	}

	uint32_t vtx_offset_elements = 0;
	uint32_t idx_offset_elements = 0;

	if (draw_data->Textures != NULL)
	{
		for (size_t i = 0; i < draw_data->Textures->Size; i++)
		{
			ImTextureData* texture = draw_data->Textures->Data[i];

			if (texture->Status != ImTextureStatus_OK)
			{
				spel_imgui_texture_update(ctx, texture);
			}
		}
	}

	ImVec2 clip_off = draw_data->DisplayPos; // (0,0) unless using multi-viewports
	ImVec2 clip_scale =
		draw_data
			->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	spel_vec2 fb_size = spel_window_framebuffer_size();

	spel_gfx_cmd_bind_pipeline(cl, ctx->pipeline);
	spel_imgui_state_update(ctx, cl, draw_data);

	for (int i = 0; i < draw_data->CmdListsCount; i++)
	{
		const ImDrawList* list = draw_data->CmdLists.Data[i];

		spel_gfx_buffer_update(ctx->vbuffer, list->VtxBuffer.Data,
							   list->VtxBuffer.Size * sizeof(ImDrawVert), vtx_offset_elements * sizeof(ImDrawVert));

		spel_gfx_buffer_update(ctx->ibuffer, list->IdxBuffer.Data,
							   list->IdxBuffer.Size * sizeof(ImDrawIdx),
							   idx_offset_elements * sizeof(ImDrawIdx));

		spel_gfx_cmd_bind_shader_buffer(cl, ctx->ubuffer);

		for (int cmd_i = 0; cmd_i < list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &list->CmdBuffer.Data[cmd_i];

			if (pcmd->UserCallback != NULL)
			{
				if (pcmd->UserCallback != ImDrawCallback_ResetRenderState)
				{
					pcmd->UserCallback(list, pcmd);
				}
			}
			else
			{
				spel_vec2 clip_min =
					(spel_vec2){.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
								.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y};

				spel_vec2 clip_max =
					(spel_vec2){.x = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
								.y = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y};

				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
				{
					continue;
				}

				spel_gfx_cmd_scissor(cl, clip_min.x, clip_min.y,
									 clip_max.x - clip_min.x, clip_max.y - clip_min.y);

				spel_gfx_cmd_bind_texture(
					cl, 0, (spel_gfx_texture)((uintptr_t)ImDrawCmd_GetTexID(pcmd)));

				spel_gfx_cmd_bind_index(
					cl, ctx->ibuffer,
					sizeof(ImDrawIdx) == 2 ? SPEL_GFX_INDEX_U16 : SPEL_GFX_INDEX_U32, 0);

				spel_gfx_cmd_bind_vertex(cl, 0, ctx->vbuffer, 0);

				spel_gfx_cmd_draw_indexed(cl, pcmd->ElemCount,
										  idx_offset_elements + pcmd->IdxOffset,
										  vtx_offset_elements + pcmd->VtxOffset);
			}
		}

		vtx_offset_elements += list->VtxBuffer.Size;
		idx_offset_elements += list->IdxBuffer.Size;
	}

	spel_gfx_cmd_scissor(cl, 0, 0, fb_size.x, fb_size.y);
}

sp_hidden void spel_imgui_resources_create(spel_imgui_context ctx)
{
	spel_gfx_shader_desc vtx_shader_desc;
	vtx_shader_desc.shader_source = SPEL_GFX_SHADER_STATIC;
	vtx_shader_desc.source = spel_internal_imgui_vert_spv;
	vtx_shader_desc.source_size = spel_internal_imgui_vert_spv_len;
	vtx_shader_desc.debug_name = "spel_internal_imgui_vert";

	spel_gfx_shader_desc frag_shader_desc;
	frag_shader_desc.shader_source = SPEL_GFX_SHADER_STATIC;
	frag_shader_desc.source = spel_internal_imgui_frag_spv;
	frag_shader_desc.source_size = spel_internal_imgui_frag_spv_len;
	frag_shader_desc.debug_name = "spel_internal_imgui_frag";

	ctx->vtx_shader = spel_gfx_shader_create(ctx->gfx, &vtx_shader_desc);
	ctx->frag_shader = spel_gfx_shader_create(ctx->gfx, &frag_shader_desc);

	spel_gfx_pipeline_desc pipeline_desc = spel_gfx_pipeline_default_2d(ctx->gfx);
	pipeline_desc.vertex_shader = ctx->vtx_shader;
	pipeline_desc.fragment_shader = ctx->frag_shader;
	pipeline_desc.cull_mode = SPEL_GFX_CULL_NONE;
	pipeline_desc.depth_state.depth_test = false;
	pipeline_desc.stencil.enabled = false;
	pipeline_desc.scissor_test = true;

	ctx->pipeline = spel_gfx_pipeline_create(ctx->gfx, &pipeline_desc);

	ctx->ubuffer = spel_gfx_uniform_buffer_create(ctx->pipeline, "ProjData");
	ctx->matrix_handle = spel_gfx_uniform_get(ctx->pipeline, "ProjMtx");

	unsigned char* pixels;
	int bpp;
	int width;
	int height;
	ImFontAtlas_GetTexDataAsRGBA32(ctx->io->Fonts, &pixels, &width, &height, &bpp);

	spel_gfx_texture_desc font_desc = {.type = SPEL_GFX_TEXTURE_2D,
									   .width = width,
									   .height = height,
									   .depth = 1,
									   .mip_count = 1,
									   .format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
									   .usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
									   .data = pixels,
									   .data_size = (size_t)(width * height * bpp)};
	ctx->atlas = spel_gfx_texture_create(ctx->gfx, &font_desc);

	ImFontAtlas_SetTexID(ctx->io->Fonts, (ImTextureID)(uintptr_t)ctx->atlas);

	for (size_t i = 0; i < ctx->io->Fonts->TexList.Size; i++)
	{
		ctx->io->Fonts->TexList.Data[i]->Status = ImTextureStatus_OK;
	}
}

sp_api void spel_imgui_context_destroy(spel_imgui_context ctx)
{
	ImGui_SetCurrentContext(ctx->context);

	cImGui_ImplSDL3_Shutdown();
	ImGui_DestroyContext(ctx->context);

	if (ctx->vbuffer)
	{
		spel_gfx_buffer_destroy(ctx->vbuffer);
	}

	if (ctx->ibuffer)
	{
		spel_gfx_buffer_destroy(ctx->ibuffer);
	}

	spel_gfx_uniform_buffer_destroy(ctx->ubuffer);
	spel_gfx_pipeline_destroy(ctx->pipeline);
	spel_gfx_texture_destroy(ctx->atlas);

	spel_memory_free(ctx);
}

sp_hidden void spel_imgui_buffers_check(spel_imgui_context ctx, ImDrawData* drawData)
{
	uint32_t vtx_size = drawData->TotalVtxCount * sizeof(ImDrawVert);
	uint32_t idx_size = drawData->TotalIdxCount * sizeof(ImDrawIdx);

	if (vtx_size != 0)
	{
		if (!ctx->vbuffer)
		{
			spel_gfx_buffer_desc vb_desc = {.type = SPEL_GFX_BUFFER_VERTEX,
											.usage = SPEL_GFX_USAGE_DYNAMIC,
											.access = SPEL_GFX_BUFFER_DRAW,
											.size = (size_t)vtx_size * 2,
											.data = NULL};
			ctx->vbuffer = spel_gfx_buffer_create(ctx->gfx, &vb_desc);
		}
		else if (spel_gfx_buffer_size(ctx->vbuffer) < vtx_size)
		{
			spel_gfx_buffer_resize(ctx->vbuffer, (size_t)vtx_size * 2, false);
		}
	}

	if (idx_size != 0)
	{
		if (!ctx->ibuffer)
		{
			spel_gfx_buffer_desc ib_desc = {.type = SPEL_GFX_BUFFER_INDEX,
											.usage = SPEL_GFX_USAGE_DYNAMIC,
											.access = SPEL_GFX_BUFFER_DRAW,
											.size = (size_t)idx_size * 2,
											.data = NULL};
			ctx->ibuffer = spel_gfx_buffer_create(ctx->gfx, &ib_desc);
		}
		else if (spel_gfx_buffer_size(ctx->ibuffer) < idx_size)
		{
			spel_gfx_buffer_resize(ctx->ibuffer, (size_t)idx_size * 2, false);
		}
	}
}

sp_hidden void spel_imgui_texture_update(spel_imgui_context ctx, ImTextureData* texture)
{
	sp_assert(texture->TexID != 0,
			  "tried to update empty texture %p", texture->TexID);
	sp_assert(texture->Format == ImTextureFormat_RGBA32, "invalid texture format %d",
			  texture->Format);

	if (texture->Status == ImTextureStatus_WantCreate)
	{
		size_t size = (long)texture->Width * texture->Height * texture->BytesPerPixel;

		spel_gfx_texture_desc texture_desc = {.type = SPEL_GFX_TEXTURE_2D,
											  .width = texture->Width,
											  .height = texture->Height,
											  .depth = 1,
											  .mip_count = 1,
											  .format = SPEL_GFX_TEXTURE_FMT_RGBA8_UNORM,
											  .usage = SPEL_GFX_TEXTURE_USAGE_SAMPLED,
											  .data = texture->Pixels,
											  .data_size = size};

		spel_gfx_texture result = spel_gfx_texture_create(ctx->gfx, &texture_desc);
		ImTextureData_SetTexID(texture, (ImTextureID)(uintptr_t)result);
		ImTextureData_SetStatus(texture, ImTextureStatus_OK);
	}
	else if (texture->Status == ImTextureStatus_WantUpdates)
	{
		for (size_t i = 0; i < texture->Updates.Size; i++)
		{
			ImTextureRect update = texture->Updates.Data[i];
			int bpp = texture->BytesPerPixel;

			spel_gfx_texture_update(
				(spel_gfx_texture)((uintptr_t)texture->TexID), 0,
				(spel_rect){
					.x = update.x, .y = update.y, .width = update.w, .height = update.h},
				ImTextureData_GetPixelsAt(texture, update.x, update.y),
				update.w * update.h * bpp);
		}

		ImTextureData_SetStatus(texture, ImTextureStatus_OK);
	}
	else if (texture->Status == ImTextureStatus_WantDestroy && texture->UnusedFrames > 0)
	{
		spel_gfx_texture_destroy((spel_gfx_texture)((uintptr_t)texture->TexID));

		ImTextureData_SetTexID(texture, ImTextureID_Invalid);
		ImTextureData_SetStatus(texture, ImTextureStatus_Destroyed);
	}
}

spel_vec4* ortho_proj(float L, float R, float T, float B)
{
	static spel_vec4 ORTHO_PROJECTION[4];

	ORTHO_PROJECTION[0] = (spel_vec4){2.0F / (R - L), 0.0F, 0.0F, 0.0F};
	ORTHO_PROJECTION[1] = (spel_vec4){0.0F, 2.0F / (T - B), 0.0F, 0.0F};
	ORTHO_PROJECTION[2] = (spel_vec4){0.0F, 0.0F, -1.0F, 0.0F};
	ORTHO_PROJECTION[3] = (spel_vec4){(R + L) / (L - R), (T + B) / (B - T), 0.0F, 1.0F};

	return ORTHO_PROJECTION;
}

sp_hidden void spel_imgui_state_update(spel_imgui_context ctx, spel_gfx_cmdlist cl,
									   ImDrawData* drawData)
{
	float L = drawData->DisplayPos.x;
	float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
	float T = drawData->DisplayPos.y;
	float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

	spel_vec4* mat = ortho_proj(L, R, T, B);

	spel_gfx_cmd_uniform_update(cl, ctx->ubuffer, ctx->matrix_handle, mat, 64);
}

sp_hidden bool spel_imgui_event_callback(void* event, void* ctx)
{
	cImGui_ImplSDL3_ProcessEvent(event);
	spel_imgui_context context = (spel_imgui_context)ctx;
	return !(context->io->WantCaptureMouse || context->io->WantCaptureKeyboard);
}
