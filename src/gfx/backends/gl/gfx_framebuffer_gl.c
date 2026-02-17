#include "core/log.h"
#include "gfx/gfx_types.h"
#include "gfx_vtable_gl.h"
#include <string.h>

const char* fbo_status_string(GLenum status);

sp_hidden spel_gfx_framebuffer spel_gfx_framebuffer_create_gl(
	spel_gfx_context ctx, const spel_gfx_framebuffer_desc* desc)
{
	spel_gfx_framebuffer fb =
		(spel_gfx_framebuffer)spel_memory_malloc(sizeof(*fb), SPEL_MEM_TAG_GFX);

	fb->ctx = ctx;

	fb->data = spel_memory_malloc(sizeof(GLuint), SPEL_MEM_TAG_GFX);
	fb->desc = *desc;

	if (!fb->data)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate GL handle storage");
		spel_memory_free(fb);
		return NULL;
	}

	GLuint* gl_handle = (GLuint*)fb->data;
	glCreateFramebuffers(1, gl_handle);

	for (uint32_t i = 0; i < desc->color_count; i++)
	{
		glNamedFramebufferTexture(*gl_handle, GL_COLOR_ATTACHMENT0 + i,
								  *(GLuint*)desc->color[i].texture->data,
								  (int)desc->color[i].mip);
	}

	if (desc->depth.texture)
	{
		GLenum attachment = desc->depth.type == SPEL_GFX_ATTACHMENT_DEPTH_STENCIL
								? GL_DEPTH_STENCIL_ATTACHMENT
								: GL_DEPTH_ATTACHMENT;
		glNamedFramebufferTexture(*gl_handle, attachment,
								  *(GLuint*)desc->depth.texture->data,
								  (int)desc->depth.mip);
	}

	GLenum status = glCheckNamedFramebufferStatus(*gl_handle, GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		sp_error(SPEL_ERR_INVALID_RESOURCE, "framebuffer incomplete: %s",
				 fbo_status_string(status));
	}

	sp_trace("created GL framebuffer %d (%dx%d, attachments=%d, depth=%d)", *gl_handle,
			 desc->width, desc->height, desc->color_count, desc->depth.texture != NULL);

	return fb;
}

sp_hidden void spel_gfx_framebuffer_destroy_gl(spel_gfx_framebuffer fb)
{
	sp_trace("destroyed GL framebuffer %d", *(GLuint*)fb->data);
	glDeleteFramebuffers(1, fb->data);
	spel_memory_free(fb->data);
	spel_memory_free(fb);
}

sp_hidden spel_gfx_render_pass spel_gfx_render_pass_create_gl(
	spel_gfx_context ctx, const spel_gfx_render_pass_desc* desc)
{
	spel_gfx_render_pass pass =
		(spel_gfx_render_pass)spel_memory_malloc(sizeof(*pass), SPEL_MEM_TAG_GFX);

	pass->ctx = ctx;
	pass->desc = *desc;
	pass->data = spel_memory_malloc(sizeof(spel_gfx_gl_framebuffer), SPEL_MEM_TAG_GFX);

	spel_gfx_gl_framebuffer* data = (spel_gfx_gl_framebuffer*)pass->data;

	if (desc->framebuffer)
	{
		GLenum bufs[SPEL_GFX_MAX_COLOR_ATTACHMENTS];
		for (uint32_t i = 0; i < desc->framebuffer->desc.color_count; i++)
		{
			bufs[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		memcpy(data->draw_buffers, bufs,
			   desc->framebuffer->desc.color_count * sizeof(GLenum));
		data->draw_buffer_count = desc->framebuffer->desc.color_count;
	}
	else
	{
		data->draw_buffers[0] = GL_BACK_LEFT;
		data->draw_buffer_count = 1;
	}

	return pass;
}

sp_hidden void spel_gfx_render_pass_destroy_gl(spel_gfx_render_pass pass)
{
	spel_memory_free(pass->data);
	spel_memory_free(pass);
}

sp_hidden void spel_gfx_framebuffer_blit_gl(spel_gfx_framebuffer src, spel_rect srcRegion,
											spel_gfx_framebuffer dst, spel_rect dstRegion,
											uint8_t attachment,
											spel_gfx_sampler_filter filter)
{
	GLuint src_fb = src ? *(GLuint*)src->data : 0;
	GLuint dst_fb = dst ? *(GLuint*)dst->data : 0;

	GLbitfield mask = 0;
	GLenum gl_filter = filter == SPEL_GFX_SAMPLER_FILTER_NEAREST ? GL_NEAREST : GL_LINEAR;

	if (attachment & SPEL_GFX_ATTACHMENT_COLOR)
	{
		mask |= GL_COLOR_BUFFER_BIT;
	}

	if (attachment & SPEL_GFX_ATTACHMENT_DEPTH)
	{
		if (gl_filter != GL_NEAREST)
		{
			sp_warn("gl: blitting the depth/stencil buffers requires nearest sampling");
			gl_filter = GL_NEAREST;
		}
		mask |= GL_DEPTH_BUFFER_BIT;
	}

	if (attachment & SPEL_GFX_ATTACHMENT_DEPTH_STENCIL)
	{
		if (gl_filter != GL_NEAREST)
		{
			sp_warn("gl: blitting the depth/stencil buffers requires nearest sampling");
			gl_filter = GL_NEAREST;
		}
		mask |= GL_DEPTH_BUFFER_BIT;
		mask |= GL_STENCIL_BUFFER_BIT;
	}

	glBlitNamedFramebuffer(src_fb, dst_fb, srcRegion.x, srcRegion.y, srcRegion.width,
						   srcRegion.height, dstRegion.x, dstRegion.y, dstRegion.width,
						   dstRegion.height, mask, gl_filter);
}

sp_hidden void spel_gfx_framebuffer_resize_gl(spel_gfx_framebuffer fb, uint32_t width,
											  uint32_t height)
{
	// Avoid zero-sized attachments which make FBO incomplete during live resize.
	width = width == 0 ? 1 : width;
	height = height == 0 ? 1 : height;

	if (fb->desc.width == width && fb->desc.height == height)
	{
		return;
	}

	GLuint* gl_handle = (GLuint*)fb->data;
	glDeleteFramebuffers(1, gl_handle);

	for (uint32_t i = 0; i < fb->desc.color_count; i++)
	{
		spel_gfx_texture_resize(fb->desc.color[i].texture, width, height);
	}

	if (fb->desc.depth.texture)
	{
		spel_gfx_texture_resize(fb->desc.depth.texture, width, height);
	}

	glCreateFramebuffers(1, gl_handle);
	for (uint32_t i = 0; i < fb->desc.color_count; i++)
	{
		glNamedFramebufferTexture(*gl_handle, GL_COLOR_ATTACHMENT0 + i,
								  *(GLuint*)fb->desc.color[i].texture->data,
								  fb->desc.color[i].mip);
	}

	if (fb->desc.depth.texture)
	{
		GLenum att = fb->desc.depth.type == SPEL_GFX_ATTACHMENT_DEPTH_STENCIL
						 ? GL_DEPTH_STENCIL_ATTACHMENT
						 : GL_DEPTH_ATTACHMENT;
		glNamedFramebufferTexture(*gl_handle, att, *(GLuint*)fb->desc.depth.texture->data,
								  fb->desc.depth.mip);
	}

	fb->desc.width = width;
	fb->desc.height = height;
}

const char* fbo_status_string(GLenum status)
{
	switch (status)
	{
	case GL_FRAMEBUFFER_UNDEFINED:
		return "undefined";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "incomplete attachment";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "missing attachment";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		return "incomplete draw buffer";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		return "incomplete read buffer";
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return "unsupported";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		return "incomplete multisample";
	default:
		return "unknown";
	}
}
