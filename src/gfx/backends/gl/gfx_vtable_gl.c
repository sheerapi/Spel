#include "gfx_vtable_gl.h"

spel_gfx_vtable_t GL_VTABLE = {.ctx_destroy = spel_gfx_context_destroy_gl,

							   .frame_begin = spel_gfx_frame_begin_gl,
							   .frame_end = spel_gfx_frame_end_gl,

							   .cmdlist_create = spel_gfx_cmdlist_create_gl,
							   .cmdlist_destroy = spel_gfx_cmdlist_destroy_gl,
							   .cmdlist_submit = spel_gfx_cmdlist_submit_gl,
							   .cmdlist_alloc = spel_gfx_cmdlist_alloc_gl,

							   .buffer_create = spel_gfx_buffer_create_gl,
							   .buffer_destroy = spel_gfx_buffer_destroy_gl,
							   .buffer_update = spel_gfx_buffer_update_gl,
							   .buffer_map = spel_gfx_buffer_map_gl,
							   .buffer_unmap = spel_gfx_buffer_unmap_gl,
							   .buffer_flush = spel_gfx_buffer_flush_gl,

							   .shader_create = spel_gfx_shader_create_gl,
							   .shader_destroy = spel_gfx_shader_destroy_gl,

							   .pipeline_create = spel_gfx_pipeline_create_gl,
							   .pipeline_destroy = spel_gfx_pipeline_destroy_gl,

							   .texture_create = spel_gfx_texture_create_gl,
							   .texture_destroy = spel_gfx_texture_destroy_gl,
							   .sampler_create = spel_gfx_sampler_create_gl,
							   .sampler_destroy = spel_gfx_sampler_destroy_gl,

							   .framebuffer_create = spel_gfx_framebuffer_create_gl,
							   .framebuffer_destroy = spel_gfx_framebuffer_destroy_gl,
							   .render_pass_create = spel_gfx_render_pass_create_gl,
							   .render_pass_destroy = spel_gfx_render_pass_destroy_gl};