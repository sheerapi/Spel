#include "core/entry.h"
#include "core/log.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_texture.h"
#include "gfx/gfx_types.h"
#include "gfx_vtable_gl.h"
#include <math.h>

static GLenum spel_gl_filter(spel_gfx_sampler_filter f)
{
	return (f == SPEL_GFX_SAMPLER_FILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;
}

static GLenum spel_gl_wrap(spel_gfx_sampler_wrap w)
{
	switch (w)
	{
	case SPEL_GFX_SAMPLER_WRAP_REPEAT:
		return GL_REPEAT;
	case SPEL_GFX_SAMPLER_WRAP_CLAMP:
		return GL_CLAMP_TO_EDGE;
	case SPEL_GFX_SAMPLER_WRAP_MIRROR:
		return GL_MIRRORED_REPEAT;
	}
	return GL_REPEAT;
}

static GLenum spel_gl_min_filter(spel_gfx_sampler_filter min,
								 spel_gfx_sampler_mip_filter mip)
{
	if (mip == SPEL_GFX_SAMPLER_MIP_NONE)
	{
		return (min == SPEL_GFX_SAMPLER_FILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;
	}

	if (mip == SPEL_GFX_SAMPLER_MIP_NEAREST)
	{
		return (min == SPEL_GFX_SAMPLER_FILTER_NEAREST) ? GL_NEAREST_MIPMAP_NEAREST
														: GL_LINEAR_MIPMAP_NEAREST;
	}

	return (min == SPEL_GFX_SAMPLER_FILTER_NEAREST) ? GL_NEAREST_MIPMAP_LINEAR
													: GL_LINEAR_MIPMAP_LINEAR;
}

static GLenum spel_gl_texture_target(spel_gfx_texture_type type)
{
	switch (type)
	{
	case SPEL_GFX_TEXTURE_2D:
		return GL_TEXTURE_2D;
	case SPEL_GFX_TEXTURE_2D_ARRAY:
		return GL_TEXTURE_2D_ARRAY;
	case SPEL_GFX_TEXTURE_3D:
		return GL_TEXTURE_3D;
	case SPEL_GFX_TEXTURE_CUBE:
		return GL_TEXTURE_CUBE_MAP;
	}

	return GL_TEXTURE_2D;
}

spel_gfx_texture spel_gfx_texture_create_gl(spel_gfx_context ctx,
											const spel_gfx_texture_desc* desc)
{
	if (!spel_gfx_texture_validate(desc))
	{
		sp_warn("invalid texture description (w:%d h:%d d:%d mips:%d type:%d)",
				desc->width, desc->height, desc->depth, desc->mip_count, desc->type);
		return ctx->checkerboard != NULL ? ctx->checkerboard : NULL;
	}

	const spel_gfx_gl_format_info* fmt = &GL_FORMATS[desc->format];

	if ((desc->usage & SPEL_GFX_TEXTURE_USAGE_RENDER) && !fmt->renderable)
	{
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "format not renderable");
		return NULL;
	}

	if ((desc->usage & SPEL_GFX_TEXTURE_USAGE_STORAGE) && !fmt->storage)
	{
		sp_error(SPEL_ERR_INVALID_ARGUMENT, "format not storage-capable");
		return NULL;
	}

	uint32_t depth = desc->depth == 0 ? 1 : desc->depth;
	uint32_t max_mips = 1 + (uint32_t)floor(log2(fmax(desc->width, desc->height)));

	uint32_t mip_count = desc->mip_count == 0 ? max_mips : desc->mip_count;
#ifdef DEBUG
	if (desc->data)
	{
		size_t expected = (size_t)desc->width * (size_t)desc->height * (size_t)depth *
						  (size_t)fmt->bytes_per_pixel;
		sp_assert(desc->data_size >= expected, "i expected more data");
	}
#endif

	spel_gfx_texture texture =
		(spel_gfx_texture)spel_memory_malloc(sizeof(*texture), SPEL_MEM_TAG_GFX);
	if (!texture)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate texture object");
		return NULL;
	}

	if (desc->usage & SPEL_GFX_TEXTURE_USAGE_RENDER)
	{
		mip_count = 1;
	}

	texture->ctx = ctx;
	texture->type = desc->type;
	texture->internal = false;
	texture->width = desc->width;
	texture->height = desc->height;
	texture->format = desc->format;
	texture->mip_count = mip_count;
	texture->depth = desc->depth == 0 ? 1 : desc->depth;

	texture->data = spel_memory_malloc(sizeof(GLuint), SPEL_MEM_TAG_GFX);
	if (!texture->data)
	{
		sp_error(SPEL_ERR_OOM, "failed to allocate GL handle storage");
		spel_memory_free(texture);
		return NULL;
	}
	GLuint* gl_handle = (GLuint*)texture->data;

	GLenum target = spel_gl_texture_target(desc->type);
	glCreateTextures(target, 1, gl_handle);
	if (*gl_handle == 0)
	{
		sp_error(SPEL_ERR_CONTEXT_FAILED, "glCreateTextures returned 0");
		spel_memory_free(texture->data);
		spel_memory_free(texture);
		return NULL;
	}

	if (desc->type == SPEL_GFX_TEXTURE_2D)
	{
		glTextureStorage2D(*gl_handle, (int)mip_count, fmt->internal_format,
						   (int)desc->width, (int)desc->height);
	}
	else
	{
		glTextureStorage3D(*gl_handle, (int)mip_count, fmt->internal_format,
						   (int)desc->width, (int)desc->height, (int)desc->depth);
	}

	if (desc->data)
	{
		switch (desc->type)
		{
		case SPEL_GFX_TEXTURE_2D:
			glTextureSubImage2D(*gl_handle, 0, 0, 0, (int)desc->width, (int)desc->height,
								fmt->external_format, fmt->type, desc->data);
			break;

		case SPEL_GFX_TEXTURE_2D_ARRAY:
		case SPEL_GFX_TEXTURE_3D:
		case SPEL_GFX_TEXTURE_CUBE:
			glTextureSubImage3D(*gl_handle, 0, 0, 0, 0, (int)desc->width,
								(int)desc->height, (int)desc->depth, fmt->external_format,
								fmt->type, desc->data);
			break;
		}
	}

	if ((desc->mip_count > 1 || desc->mip_count == 0) && desc->data)
	{
		glGenerateTextureMipmap(*gl_handle);
	}

	glTextureParameteri(*gl_handle, GL_TEXTURE_BASE_LEVEL, 0);
	int max_level = (int)mip_count - 1;
	if (max_level < 0)
	{
		max_level = 0;
	}
	glTextureParameteri(*gl_handle, GL_TEXTURE_MAX_LEVEL, max_level);

	sp_trace("created GL texture %u (%dx%dx%d, mips=%d, fmt=%d)", *gl_handle, desc->width,
			 desc->height, desc->depth, desc->mip_count, desc->format);

	return texture;
}

void spel_gfx_texture_destroy_gl(spel_gfx_texture texture)
{
	if (texture->internal)
	{
		sp_error(SPEL_ERR_INVALID_RESOURCE, "you can't destroy an internal texture!");
		return;
	}

	GLuint handle = *(GLuint*)texture->data;
	glDeleteTextures(1, &handle);
	sp_trace("destroyed GL texture %u", handle);
	spel_memory_free(texture->data);
	spel_memory_free(texture);
}

spel_gfx_sampler spel_gfx_sampler_create_gl(spel_gfx_context ctx,
											const spel_gfx_sampler_desc* desc)
{
	spel_gfx_sampler sampler =
		(spel_gfx_sampler)spel_memory_malloc(sizeof(*sampler), SPEL_MEM_TAG_GFX);

	sampler->ctx = ctx;
	sampler->data = spel_memory_malloc(sizeof(GLuint), SPEL_MEM_TAG_GFX);
	GLuint* gl_handle = (GLuint*)sampler->data;

	glCreateSamplers(1, gl_handle);

	glSamplerParameteri(*gl_handle, GL_TEXTURE_MIN_FILTER,
						(int)spel_gl_min_filter(desc->min, desc->mip));

	glSamplerParameteri(*gl_handle, GL_TEXTURE_MAG_FILTER,
						(int)spel_gl_filter(desc->mag));

	glSamplerParameteri(*gl_handle, GL_TEXTURE_WRAP_S, (int)spel_gl_wrap(desc->wrap_u));
	glSamplerParameteri(*gl_handle, GL_TEXTURE_WRAP_T, (int)spel_gl_wrap(desc->wrap_v));
	glSamplerParameteri(*gl_handle, GL_TEXTURE_WRAP_R, (int)spel_gl_wrap(desc->wrap_w));

	glSamplerParameterf(*gl_handle, GL_TEXTURE_LOD_BIAS, desc->lod_bias);

	if (desc->max_aniso > 1.0F)
	{
		float max_supported = 1.0F;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_supported);

		float aniso = desc->max_aniso;
		if (aniso > max_supported)
		{
			aniso = max_supported;
		}

		glSamplerParameterf(*gl_handle, GL_TEXTURE_MAX_ANISOTROPY, aniso);
	}

	return sampler;
}

void spel_gfx_sampler_destroy_gl(spel_gfx_sampler sampler)
{
	GLuint* gl_handle = (GLuint*)sampler->data;
	if (gl_handle)
	{
		glDeleteSamplers(1, gl_handle);
	}

	spel_memory_free(sampler->data);
	spel_memory_free(sampler);
}

sp_hidden void spel_gfx_texture_resize_gl(spel_gfx_texture tex, uint32_t width,
										  uint32_t height)
{
	width = width == 0 ? 1 : width;
	height = height == 0 ? 1 : height;

	if (tex->width == width && tex->height == height)
	{
		return;
	}

	GLuint* gl_handle = (GLuint*)tex->data;
	glDeleteTextures(1, gl_handle);
	GLenum target = spel_gl_texture_target(tex->type);
	glCreateTextures(target, 1, gl_handle);

	tex->width = width;
	tex->height = height;

	const spel_gfx_gl_format_info* fmt = &GL_FORMATS[tex->format];

	if (tex->type == SPEL_GFX_TEXTURE_2D)
	{
		glTextureStorage2D(*gl_handle, (int)tex->mip_count, fmt->internal_format,
						   (int)width, (int)height);
	}
	else
	{
		glTextureStorage3D(*gl_handle, (int)tex->mip_count, fmt->internal_format,
						   (int)width, (int)height, (int)tex->depth);
	}

	// match creation-time parameters so sampling stays valid
	glTextureParameteri(*gl_handle, GL_TEXTURE_BASE_LEVEL, 0);
	int max_level = (int)tex->mip_count - 1;
	if (max_level < 0)
	{
		max_level = 0;
	}
	glTextureParameteri(*gl_handle, GL_TEXTURE_MAX_LEVEL, max_level);

	// keep dimensions in sync for future resizes
	tex->width = width;
	tex->height = height;
}
