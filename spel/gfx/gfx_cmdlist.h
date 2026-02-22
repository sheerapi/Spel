#ifndef SPEL_GFX_CMDLIST
#define SPEL_GFX_CMDLIST
#include "core/macros.h"
#include "gfx/gfx_types.h"
#include "gfx/gfx_uniform.h"
#include "utils/math.h"

spel_api spel_gfx_cmdlist spel_gfx_cmdlist_create(spel_gfx_context ctx);
spel_api spel_gfx_cmdlist spel_gfx_cmdlist_default(spel_gfx_context ctx);

spel_api void spel_gfx_cmdlist_destroy(spel_gfx_cmdlist cmdlist);

spel_api void spel_gfx_cmdlist_submit(spel_gfx_cmdlist cmdlist);

spel_api void spel_gfx_cmd_bind_vertex(spel_gfx_cmdlist cl, uint32_t stream,
									 spel_gfx_buffer buf, size_t offset);

spel_api void spel_gfx_cmd_bind_index(spel_gfx_cmdlist cl, spel_gfx_buffer buf,
									spel_gfx_index_type type, size_t offset);

spel_api void spel_gfx_cmd_clear(spel_gfx_cmdlist cl, spel_color color);
spel_api void spel_gfx_cmd_viewport(spel_gfx_cmdlist cl, int x, int y, int width,
								  int height);

spel_api void spel_gfx_cmd_scissor(spel_gfx_cmdlist cl, int x, int y, int width,
								 int height);

spel_api void spel_gfx_cmd_bind_pipeline(spel_gfx_cmdlist cl, spel_gfx_pipeline pipeline);

spel_api void spel_gfx_cmd_draw(spel_gfx_cmdlist cl, uint32_t vertexCount,
							  uint32_t firstVertex);

spel_api void spel_gfx_cmd_draw_indexed(spel_gfx_cmdlist cl, uint32_t indexCount,
									  uint32_t firstIndex, int32_t vertexOffset);

spel_api void spel_gfx_cmd_bind_texture(spel_gfx_cmdlist cl, uint32_t slot,
									  spel_gfx_texture texture);

spel_api void spel_gfx_cmd_bind_sampler(spel_gfx_cmdlist cl, uint32_t slot,
									  spel_gfx_sampler sampler);

spel_api void spel_gfx_cmd_bind_image(spel_gfx_cmdlist cl, uint32_t slot,
									spel_gfx_texture texture, spel_gfx_sampler sampler);

spel_api void spel_gfx_cmd_uniform_update(spel_gfx_cmdlist cl, spel_gfx_uniform_buffer buf,
										spel_gfx_uniform handle, const void* data,
										size_t size);

spel_api void spel_gfx_cmd_uniform_block_update(spel_gfx_cmdlist cl,
											  spel_gfx_uniform_buffer buf,
											  const void* data, size_t size);

spel_api void spel_gfx_cmd_bind_shader_buffer(spel_gfx_cmdlist cl,
											spel_gfx_uniform_buffer buf);

spel_api void spel_gfx_cmd_begin_pass(spel_gfx_cmdlist cl, spel_gfx_render_pass pass);
spel_api void spel_gfx_cmd_end_pass(spel_gfx_cmdlist cl);

#endif