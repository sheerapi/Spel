#include "core/entry.h"
#include "core/log.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include "gl.h"

static GLenum spel_gfx_gl_buffer_usage(spel_gfx_buffer_access access,
									   spel_gfx_buffer_usage usage);

GLbitfield spel_gfx_gl_map_access(spel_gfx_access access);

spel_gfx_buffer spel_gfx_buffer_create_gl(spel_gfx_context ctx,
										  const spel_gfx_buffer_desc* desc)
{
	spel_gfx_buffer buf = (spel_gfx_buffer)sp_malloc(sizeof(*buf), SPEL_MEM_TAG_GFX);
	if (!buf)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate buffer object");
		return NULL;
	}

	buf->ctx = ctx;
	buf->data = sp_malloc(sizeof(GLuint), SPEL_MEM_TAG_GFX);
	if (!buf->data)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate GL handle storage");
		sp_free(buf);
		return NULL;
	}

	buf->persistent = false;
	buf->type = desc->type;

	glCreateBuffers(1, (GLuint*)buf->data);
	if (*(GLuint*)buf->data == 0)
	{
		sp_error(SPEL_ERR_CONTEXT_FAILED, "glCreateBuffers returned 0");
		sp_free(buf->data);
		sp_free(buf);
		return NULL;
	}

	glNamedBufferData(*(GLuint*)buf->data, desc->size, desc->data,
					  spel_gfx_gl_buffer_usage(desc->access, desc->usage));
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		sp_error(SPEL_ERR_INVALID_STATE, "glNamedBufferData error 0x%x", err);
		glDeleteBuffers(1, (GLuint*)buf->data);
		sp_free(buf->data);
		sp_free(buf);
		return NULL;
	}

	sp_debug("created GL buffer %u size=%zu", *(GLuint*)buf->data, desc->size);

	return buf;
}

void spel_gfx_buffer_destroy_gl(spel_gfx_buffer buf)
{
	GLuint handle = *(GLuint*)buf->data;
	glDeleteBuffers(1, &handle);
	sp_debug("destroyed GL buffer %u", handle);
	sp_free(buf->data);
	sp_free(buf);
}

void spel_gfx_buffer_update_gl(spel_gfx_buffer buf, const void* data, size_t size,
							   size_t offset)
{
	glNamedBufferSubData(*(GLuint*)buf->data, (GLintptr)offset, (GLsizeiptr)size, data);
}

void* spel_gfx_buffer_map_gl(spel_gfx_buffer buf, size_t offset, size_t size,
							 spel_gfx_access access)
{
	buf->persistent = true;
	return glMapNamedBufferRange(*(GLuint*)buf->data, offset, size,
								 spel_gfx_gl_map_access(access));
}

void spel_gfx_buffer_unmap_gl(spel_gfx_buffer buf)
{
	if (buf->persistent)
	{
		return;
	}

	glUnmapNamedBuffer(*(GLuint*)buf->data);
}

void spel_gfx_buffer_flush_gl(spel_gfx_buffer buf, size_t offset, size_t size)
{
	glFlushMappedNamedBufferRange(*(GLuint*)buf->data, offset, size);
}

GLenum spel_gfx_gl_buffer_usage(spel_gfx_buffer_access access,
								spel_gfx_buffer_usage usage)
{
	static const GLenum SPEL_GL_USAGE_TABLE[3][3] = {
		{GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY},
		{GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY},
		{GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, GL_DYNAMIC_COPY}};

	return SPEL_GL_USAGE_TABLE[usage][access];
}

GLbitfield spel_gfx_gl_map_access(spel_gfx_access access)
{
#ifdef DEBUG
	if ((access & sp_gfx_access_read) &&
		(access & (sp_gfx_access_invalidate_range | sp_gfx_access_invalidate_buffer)))
	{
		sp_error(SPEL_ERR_INVALID_ARGUMENT,
				 "Invalidate flags are meaningless with READ access");
	}

	if ((access & sp_gfx_access_persistent) && !(access & sp_gfx_access_write))
	{
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "Persistent mapping requires WRITE access");
	}
#endif

	GLbitfield flags = 0;

	if (access & sp_gfx_access_read)
	{
		flags |= GL_MAP_READ_BIT;
	}

	if (access & sp_gfx_access_write)
	{
		flags |= GL_MAP_WRITE_BIT;
	}

	if (access & sp_gfx_access_invalidate_range)
	{
		flags |= GL_MAP_INVALIDATE_RANGE_BIT;
	}

	if (access & sp_gfx_access_invalidate_buffer)
	{
		flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
	}

	if (access & sp_gfx_access_unsynchronized)
	{
		flags |= GL_MAP_UNSYNCHRONIZED_BIT;
	}

	if (access & sp_gfx_access_flush_explicit)
	{
		flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
	}

	if (access & sp_gfx_access_persistent)
	{
		flags |= GL_MAP_PERSISTENT_BIT;
	}

	if (access & sp_gfx_access_coherent)
	{
		flags |= GL_MAP_COHERENT_BIT;
	}

	return flags;
}
