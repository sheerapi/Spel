#include "core/entry.h"
#include "core/log.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include "gfx_vtable_gl.h"
#include "gl.h"
#include "gl_types.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

void exec_cmd_clear(spel_gfx_cmdlist cl, spel_gfx_clear_cmd* cmd);
void exec_cmd_bind_vertex(spel_gfx_cmdlist cl, spel_gfx_bind_vertex_cmd* cmd);
void exec_cmd_bind_index(spel_gfx_cmdlist cl, spel_gfx_bind_index_cmd* cmd);
void exec_cmd_bind_pipeline(spel_gfx_cmdlist cl, spel_gfx_bind_pipeline_cmd* cmd);
void exec_cmd_draw(spel_gfx_cmdlist cl, spel_gfx_draw_cmd* cmd);
void exec_cmd_draw_indexed(spel_gfx_cmdlist cl, spel_gfx_draw_indexed_cmd* cmd);

void exec_cmd_bind_texture(spel_gfx_cmdlist cl, spel_gfx_bind_texture_cmd* cmd);
void exec_cmd_bind_sampler(spel_gfx_cmdlist cl, spel_gfx_bind_sampler_cmd* cmd);
void exec_cmd_bind_image(spel_gfx_cmdlist cl, spel_gfx_bind_image_cmd* cmd);

void exec_cmd_viewport(spel_gfx_cmdlist cl, spel_gfx_viewport_cmd* cmd);
void exec_cmd_scissor(spel_gfx_cmdlist cl, spel_gfx_scissor_cmd* cmd);

void exec_cmd_bind_shader_buffer(spel_gfx_cmdlist cl,
								 spel_gfx_bind_shader_buffer_cmd* cmd);

void exec_cmd_uniform_update(spel_gfx_cmdlist cl, spel_gfx_uniform_update_cmd* cmd);

typedef struct
{
	spel_gfx_pipeline pipeline;

	spel_gfx_buffer index_buffer;
	size_t index_offset;
	GLenum index_type;

	spel_gfx_sampler sampler;
} spel_gfx_cmdlist_gl;

spel_gfx_cmdlist spel_gfx_cmdlist_create_gl(spel_gfx_context ctx)
{
	spel_gfx_cmdlist cl =
		(spel_gfx_cmdlist)spel_memory_malloc(sizeof(*cl), SPEL_MEM_TAG_GFX);
	cl->capacity = (int)sp_cmdlist_default_size;
	cl->offset = 0;
	cl->ctx = ctx;
	cl->buffer = spel_memory_malloc(cl->capacity, SPEL_MEM_TAG_GFX);

	spel_gfx_cmdlist_gl* data = (spel_gfx_cmdlist_gl*)spel_memory_malloc(
		sizeof(spel_gfx_cmdlist_gl), SPEL_MEM_TAG_GFX);
	cl->data = data;

	cl->dirty_buffer_cap = 8;
	cl->dirty_buffer_count = 0;

	cl->dirty_buffers = (spel_gfx_buffer*)spel_memory_malloc(
		cl->dirty_buffer_cap * sizeof(*cl->dirty_buffers), SPEL_MEM_TAG_GFX);

	return cl;
}

void spel_gfx_cmdlist_destroy_gl(spel_gfx_cmdlist cl)
{
	spel_memory_free(cl->dirty_buffers);
	spel_memory_free(cl->data);
	spel_memory_free(cl->buffer);
	spel_memory_free(cl);
}

void* spel_gfx_cmdlist_alloc_gl(spel_gfx_cmdlist cl, size_t size, size_t align)
{
	const size_t MAX_ALIGN = _Alignof(max_align_t);

	uint64_t aligned = (cl->offset + (align - 1)) & ~(align - 1);
	uint64_t end = aligned + size;
	uint64_t padded_end = (end + (MAX_ALIGN - 1)) & ~(MAX_ALIGN - 1);

	while (padded_end > cl->capacity)
	{
		cl->capacity *= 2;
		void* new_buffer =
			spel_memory_realloc(cl->buffer, cl->capacity, SPEL_MEM_TAG_GFX);
		if (new_buffer == NULL)
		{
			sp_error(SPEL_ERR_OOM, "out of memory?");
			return NULL;
		}
		cl->buffer = new_buffer;
	}

	void* ptr = cl->buffer + aligned;
	cl->offset = padded_end;
	return ptr;
}

void spel_gfx_cmdlist_submit_gl(spel_gfx_cmdlist cl)
{
	if (spel.window.occluded)
	{
		cl->offset = 0;
		return;
	}

	for (size_t i = 0; i < cl->dirty_buffer_count; i++)
	{
		spel_gfx_buffer_flush_gl(cl->dirty_buffers[i], 0, 0);
	}

	uint8_t* ptr = cl->buffer;
	while (ptr < cl->buffer + cl->offset)
	{
		spel_gfx_cmd_header* hdr = (spel_gfx_cmd_header*)ptr;

		switch (hdr->type)
		{
		case SPEL_GFX_CMD_CLEAR:
			exec_cmd_clear(cl, (spel_gfx_clear_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_BIND_VERTEX:
			exec_cmd_bind_vertex(cl, (spel_gfx_bind_vertex_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_BIND_INDEX:
			exec_cmd_bind_index(cl, (spel_gfx_bind_index_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_BIND_PIPELINE:
			exec_cmd_bind_pipeline(cl, (spel_gfx_bind_pipeline_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_DRAW:
			exec_cmd_draw(cl, (spel_gfx_draw_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_DRAW_INDEXED:
			exec_cmd_draw_indexed(cl, (spel_gfx_draw_indexed_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_BIND_TEXTURE:
			exec_cmd_bind_texture(cl, (spel_gfx_bind_texture_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_BIND_SAMPLER:
			exec_cmd_bind_sampler(cl, (spel_gfx_bind_sampler_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_BIND_IMAGE:
			exec_cmd_bind_image(cl, (spel_gfx_bind_image_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_VIEWPORT:
			exec_cmd_viewport(cl, (spel_gfx_viewport_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_SCISSOR:
			exec_cmd_scissor(cl, (spel_gfx_scissor_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_BIND_SHADER_BUFFER:
			exec_cmd_bind_shader_buffer(cl, (spel_gfx_bind_shader_buffer_cmd*)ptr);
			break;
		case SPEL_GFX_CMD_UNIFORM_UPDATE:
			exec_cmd_uniform_update(cl, (spel_gfx_uniform_update_cmd*)ptr);
			break;
		}

		ptr += hdr->size;
	}

	cl->offset = 0;
	cl->dirty_buffer_count = 0;

	memset(cl->dirty_buffers, 0, cl->dirty_buffer_count * sizeof(*cl->dirty_buffers));
}

void exec_cmd_clear(spel_gfx_cmdlist cl, spel_gfx_clear_cmd* cmd)
{
	glClearColor((float)cmd->color.r / 255, (float)cmd->color.g / 255,
				 (float)cmd->color.b / 255, (float)cmd->color.a / 255);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void exec_cmd_bind_vertex(spel_gfx_cmdlist cl, spel_gfx_bind_vertex_cmd* cmd)
{
	if (cmd->buf->type != SPEL_GFX_BUFFER_VERTEX)
	{
		sp_warn("buffer type does not correspond to binding cmd (vertex). this is "
				"allowed, but discouraged");
	}

	if (((spel_gfx_cmdlist_gl*)cl->data)->pipeline == NULL)
	{
		sp_error(SPEL_ERR_INVALID_STATE,
				 "you need to bind a pipeline before binding a vertex buffer");
		return;
	}

	spel_gfx_pipeline_gl* pipeline =
		((spel_gfx_pipeline_gl*)((spel_gfx_cmdlist_gl*)cl->data)->pipeline->data);

	glVertexArrayVertexBuffer(pipeline->vao, cmd->stream, *(GLuint*)cmd->buf->data,
							  (long)cmd->offset, pipeline->strides[cmd->stream]);
}

void exec_cmd_bind_pipeline(spel_gfx_cmdlist cl, spel_gfx_bind_pipeline_cmd* cmd)
{
	if (((spel_gfx_cmdlist_gl*)cl->data)->pipeline == cmd->pipeline)
	{
		return;
	}

	((spel_gfx_cmdlist_gl*)cl->data)->pipeline = cmd->pipeline;
	spel_gfx_pipeline_gl* p = (spel_gfx_pipeline_gl*)cmd->pipeline->data;

	glUseProgram(p->program);
	glBindVertexArray(p->vao);

	if (p->depth_state.test)
	{
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}

	glDepthMask((int)p->depth_state.write ? GL_TRUE : GL_FALSE);
	glDepthFunc(p->depth_state.func);

	if (p->depth_state.clamp)
	{
		glEnable(GL_DEPTH_CLAMP);
	}
	else
	{
		glDisable(GL_DEPTH_CLAMP);
	}

	if (p->stencil_state.test)
	{
		glEnable(GL_STENCIL_TEST);

		glStencilFunc(p->stencil_state.func, (int)p->stencil_state.reference,
					  p->stencil_state.read_mask);

		glStencilOp(p->stencil_state.fail_op, p->stencil_state.depth_op,
					p->stencil_state.pass_op);

		glStencilMask(p->stencil_state.write_mask);
	}
	else
	{
		glDisable(GL_STENCIL_TEST);
	}

	if (p->blend_state.enabled)
	{
		glEnable(GL_BLEND);

		glBlendFuncSeparate(p->blend_state.src_rgb, p->blend_state.dst_rgb,
							p->blend_state.src_a, p->blend_state.dst_a);

		glBlendEquationSeparate(p->blend_state.op_rgb, p->blend_state.op_a);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	glColorMask(
		(p->blend_state.write_mask & 0x1) != 0, (p->blend_state.write_mask & 0x2) != 0,
		(p->blend_state.write_mask & 0x4) != 0, (p->blend_state.write_mask & 0x8) != 0);

	if (p->topology.cull_mode != 0)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(p->topology.cull_mode);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}

	if (p->scissor_test)
	{
		glEnable(GL_SCISSOR_TEST);
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
	}

	glFrontFace(p->topology.winding);
}

void exec_cmd_bind_index(spel_gfx_cmdlist cl, spel_gfx_bind_index_cmd* cmd)
{
	if (cmd->buf->type != SPEL_GFX_BUFFER_INDEX)
	{
		sp_warn("buffer type does not correspond to binding cmd (index). this is "
				"allowed, but discouraged");
	}

	((spel_gfx_cmdlist_gl*)cl->data)->index_buffer = cmd->buf;
	((spel_gfx_cmdlist_gl*)cl->data)->index_offset = cmd->offset;
	((spel_gfx_cmdlist_gl*)cl->data)->index_type =
		cmd->type == SPEL_GFX_INDEX_U16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

	glVertexArrayElementBuffer(
		((spel_gfx_pipeline_gl*)((spel_gfx_cmdlist_gl*)cl->data)->pipeline->data)->vao,
		*(GLuint*)cmd->buf->data);
}

void exec_cmd_draw(spel_gfx_cmdlist cl, spel_gfx_draw_cmd* cmd)
{
	glDrawArrays(((spel_gfx_pipeline_gl*)((spel_gfx_cmdlist_gl*)cl->data)->pipeline->data)
					 ->topology.primitives,
				 (int)cmd->first_vertex, (int)cmd->vertex_count);
}

static inline size_t spel_gl_index_size(GLenum type)
{
	switch (type)
	{
	case GL_UNSIGNED_SHORT:
		return 2;
	case GL_UNSIGNED_INT:
		return 4;
	default:
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "unsupported index type");
		return 0;
	}
}

void exec_cmd_draw_indexed(spel_gfx_cmdlist cl, spel_gfx_draw_indexed_cmd* cmd)
{
	spel_gfx_cmdlist_gl* exec = (spel_gfx_cmdlist_gl*)cl->data;
	spel_gfx_pipeline_gl* p = (spel_gfx_pipeline_gl*)exec->pipeline->data;

	size_t index_size = spel_gl_index_size(exec->index_type);

	size_t byte_offset = exec->index_offset + ((size_t)cmd->first_index * index_size);

	glDrawElementsBaseVertex(p->topology.primitives, (int)cmd->index_count,
							 exec->index_type, (void*)byte_offset, cmd->vertex_offset);
}

void exec_cmd_bind_texture(spel_gfx_cmdlist cl, spel_gfx_bind_texture_cmd* cmd)
{
	glBindTextureUnit(cmd->slot, *(GLuint*)cmd->texture->data);
}

void exec_cmd_bind_sampler(spel_gfx_cmdlist cl, spel_gfx_bind_sampler_cmd* cmd)
{
	glBindSampler(cmd->slot, *(GLuint*)cmd->sampler->data);
}

void exec_cmd_bind_image(spel_gfx_cmdlist cl, spel_gfx_bind_image_cmd* cmd)
{
	GLuint handle = *(GLuint*)cmd->texture->data;

	GLint internal_format = 0;
	glGetTextureLevelParameteriv(handle, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
	if (internal_format == 0)
	{
		// Fallback to a sane default to avoid GL errors; this should not happen.
		internal_format = GL_RGBA8;
	}

	GLboolean layered = GL_FALSE;
	switch (cmd->texture->type)
	{
	case SPEL_GFX_TEXTURE_2D_ARRAY:
	case SPEL_GFX_TEXTURE_3D:
	case SPEL_GFX_TEXTURE_CUBE:
		layered = GL_TRUE;
		break;
	default:
		break;
	}

	glBindImageTexture(cmd->slot, handle, 0, layered, 0, GL_READ_WRITE,
					   (GLenum)internal_format);

	if (cmd->sampler)
	{
		glBindSampler(cmd->slot, *(GLuint*)cmd->sampler->data);
	}
}

void exec_cmd_viewport(spel_gfx_cmdlist cl, spel_gfx_viewport_cmd* cmd)
{
	glViewport(cmd->x, cl->ctx->fb_height - (cmd->y + cmd->height), cmd->width,
			   cmd->height);
}

void exec_cmd_scissor(spel_gfx_cmdlist cl, spel_gfx_scissor_cmd* cmd)
{
	glScissor(cmd->x, cl->ctx->fb_height - (cmd->y + cmd->height), cmd->width,
			  cmd->height);
}

void exec_cmd_bind_shader_buffer(spel_gfx_cmdlist cl,
								 spel_gfx_bind_shader_buffer_cmd* cmd)
{
	uint32_t binding = -1;

	spel_gfx_pipeline pipeline = ((spel_gfx_cmdlist_gl*)cl->data)->pipeline;

	for (size_t i = 0; i < pipeline->reflection.uniform_count; i++)
	{
		if (cmd->location == pipeline->reflection.uniforms[i].location)
		{
			binding = pipeline->reflection.uniforms[i].internal;
		}
	}

	for (size_t i = 0; i < pipeline->reflection.storage_count; i++)
	{
		if (cmd->location == pipeline->reflection.storage[i].location)
		{
			binding = pipeline->reflection.storage[i].internal;
		}
	}

	if (binding == -1)
	{
		sp_warn("location %d not found within reflection data", cmd->location);
	}

	glBindBufferBase(cmd->buf->type == SPEL_GFX_BUFFER_UNIFORM ? GL_UNIFORM_BUFFER
															   : GL_SHADER_STORAGE_BUFFER,
					 binding, ((spel_gfx_gl_buffer*)cmd->buf->data)->buffer);
}

void exec_cmd_uniform_update(spel_gfx_cmdlist cl, spel_gfx_uniform_update_cmd* cmd)
{
	sp_assert(cmd->size <= (size_t)(cmd->handle.size * cmd->handle.count),
			  "data exceeds uniform handle size, expected %d got %d",
			  (size_t)(cmd->handle.size * cmd->handle.count), cmd->size);

	spel_gfx_gl_buffer* glBuf = (spel_gfx_gl_buffer*)cmd->buffer.buffer->data;
	memcpy((uint8_t*)glBuf->mirror + cmd->handle.offset, cmd->data, cmd->size);
	glBuf->dirty_min = (uint32_t)fmin(glBuf->dirty_min, cmd->handle.offset);
	glBuf->dirty_max =
		(uint32_t)fmax(glBuf->dirty_max, (double)(cmd->handle.offset + cmd->size));
}