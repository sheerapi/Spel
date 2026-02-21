#include "core/entry.h"
#include "core/log.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include "gl.h"
#include "gl_types.h"
#include <stdint.h>
#include <string.h>

GLbitfield spel_gfx_gl_map_access(spel_gfx_access access);

spel_gfx_buffer spel_gfx_buffer_create_gl(spel_gfx_context ctx,
										  const spel_gfx_buffer_desc* desc)
{
	spel_gfx_buffer buf =
		(spel_gfx_buffer)spel_memory_malloc(sizeof(*buf), SPEL_MEM_TAG_GFX);
	if (!buf)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate buffer object");
		return NULL;
	}

	buf->ctx = ctx;
	buf->data = spel_memory_malloc(sizeof(spel_gfx_gl_buffer), SPEL_MEM_TAG_GFX);
	if (!buf->data)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate GL handle storage");
		spel_memory_free(buf);
		return NULL;
	}

	buf->persistent = false;
	buf->type = desc->type;
	buf->access = desc->access;
	buf->size = desc->size;

	spel_gfx_gl_buffer* glBuf = (spel_gfx_gl_buffer*)buf->data;
	glBuf->mirror = NULL;
	glBuf->dirty_min = UINT32_MAX;
	glBuf->dirty_max = 0;

	// TODO: Change into persistent mapping later (fences and all that)
	if (desc->type == SPEL_GFX_BUFFER_UNIFORM || desc->type == SPEL_GFX_BUFFER_STORAGE)
	{
		glBuf->mirror = spel_memory_malloc(desc->size, SPEL_MEM_TAG_GFX);

		if (desc->data != NULL)
		{
			memcpy(glBuf->mirror, desc->data, desc->size);
		}
	}

	glCreateBuffers(1, &glBuf->buffer);
	if (glBuf->buffer == 0)
	{
		sp_error(SPEL_ERR_CONTEXT_FAILED, "glCreateBuffers returned 0");
		spel_memory_free(buf->data);
		spel_memory_free(buf);
		return NULL;
	}

	GLbitfield storage_flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT |
							   GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
							   GL_MAP_COHERENT_BIT;

	glNamedBufferStorage(glBuf->buffer, desc->size, desc->data, storage_flags);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		sp_error(SPEL_ERR_INVALID_STATE, "glNamedBufferStorage error 0x%x", err);
		glDeleteBuffers(1, &((spel_gfx_gl_buffer*)buf->data)->buffer);
		spel_memory_free(buf->data);
		spel_memory_free(buf);
		return NULL;
	}

	sp_trace("created GL buffer %u size=%zu", glBuf->buffer, desc->size);

	return buf;
}

void spel_gfx_buffer_destroy_gl(spel_gfx_buffer buf)
{
	if (buf->type == SPEL_GFX_BUFFER_UNIFORM || buf->type == SPEL_GFX_BUFFER_STORAGE)
	{
		spel_memory_free(((spel_gfx_gl_buffer*)buf->data)->mirror);
	}

	GLuint handle = ((spel_gfx_gl_buffer*)buf->data)->buffer;
	glDeleteBuffers(1, &handle);
	sp_trace("destroyed GL buffer %u", handle);
	spel_memory_free(buf->data);
	spel_memory_free(buf);
}

void spel_gfx_buffer_update_gl(spel_gfx_buffer buf, const void* data, size_t size,
							   size_t offset)
{
	glNamedBufferSubData(((spel_gfx_gl_buffer*)buf->data)->buffer, (GLintptr)offset,
						 (GLsizeiptr)size, data);
}

void* spel_gfx_buffer_map_gl(spel_gfx_buffer buf, size_t offset, size_t size,
							 spel_gfx_access access)
{
	GLbitfield flags = spel_gfx_gl_map_access(access);
	buf->persistent = (access & sp_gfx_access_persistent) != 0;
	return glMapNamedBufferRange(((spel_gfx_gl_buffer*)buf->data)->buffer, offset, size,
								 flags);
}

void spel_gfx_buffer_unmap_gl(spel_gfx_buffer buf)
{
	if (buf->persistent)
	{
		return;
	}

	glUnmapNamedBuffer(((spel_gfx_gl_buffer*)buf->data)->buffer);
}

void spel_gfx_buffer_flush_gl(spel_gfx_buffer buf, size_t offset, size_t size)
{
	if (buf->type == SPEL_GFX_BUFFER_UNIFORM || buf->type == SPEL_GFX_BUFFER_STORAGE)
	{
		spel_gfx_gl_buffer* glBuf = (spel_gfx_gl_buffer*)buf->data;

		if (glBuf->dirty_min == UINT32_MAX || glBuf->dirty_min == glBuf->dirty_max)
		{
			return;
		}

		uint32_t range = glBuf->dirty_max - glBuf->dirty_min;
		glNamedBufferSubData(glBuf->buffer, glBuf->dirty_min, range,
							 (uint8_t*)glBuf->mirror + glBuf->dirty_min);
		glBuf->dirty_min = UINT32_MAX;
		glBuf->dirty_max = 0;

		return;
	}

	glFlushMappedNamedBufferRange(((spel_gfx_gl_buffer*)buf->data)->buffer, offset, size);
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

sp_hidden void spel_gfx_buffer_resize_gl(spel_gfx_buffer buf, size_t newSize,
										 bool preserveData)
{
	GLuint handle = ((spel_gfx_gl_buffer*)buf->data)->buffer;
	void* old_mirror = ((spel_gfx_gl_buffer*)buf->data)->mirror;
	size_t old_size = buf->size;

	glCreateBuffers(1, &((spel_gfx_gl_buffer*)buf->data)->buffer);
	if (((spel_gfx_gl_buffer*)buf->data)->buffer == 0)
	{
		sp_error(SPEL_ERR_CONTEXT_FAILED, "glCreateBuffers returned 0");
		buf->data = &handle;
		return;
	}

	GLbitfield storage_flags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT |
							   GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
							   GL_MAP_COHERENT_BIT;

	glNamedBufferStorage(((spel_gfx_gl_buffer*)buf->data)->buffer, newSize, NULL,
						 storage_flags);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		sp_error(SPEL_ERR_INVALID_STATE, "glNamedBufferStorage error 0x%x", err);
		glDeleteBuffers(1, &((spel_gfx_gl_buffer*)buf->data)->buffer);
		buf->data = &handle;
		return;
	}

	if (preserveData)
	{
		size_t copy_size = old_size < newSize ? old_size : newSize;
		glCopyNamedBufferSubData(handle, *(GLuint*)buf->data, 0, 0, copy_size);
	}

	sp_trace("buffer resized to %zu bytes (%.2fx more)", newSize,
			 old_size ? (double)newSize / (double)old_size : 0.0);

	buf->size = newSize;
	glDeleteBuffers(1, &handle);
	if (buf->type == SPEL_GFX_BUFFER_UNIFORM || buf->type == SPEL_GFX_BUFFER_STORAGE)
	{
		spel_memory_free(old_mirror);
	}

	// TODO: Change into persistent mapping later (fences and all that)
	if (buf->type == SPEL_GFX_BUFFER_UNIFORM || buf->type == SPEL_GFX_BUFFER_STORAGE)
	{
		((spel_gfx_gl_buffer*)buf->data)->mirror =
			spel_memory_malloc(buf->size, SPEL_MEM_TAG_GFX);

		if (preserveData && old_mirror)
		{
			memcpy(((spel_gfx_gl_buffer*)buf->data)->mirror, old_mirror,
				   old_size < buf->size ? old_size : buf->size);
		}
	}

	((spel_gfx_gl_buffer*)buf->data)->dirty_min = UINT32_MAX;
	((spel_gfx_gl_buffer*)buf->data)->dirty_max = 0;
}
