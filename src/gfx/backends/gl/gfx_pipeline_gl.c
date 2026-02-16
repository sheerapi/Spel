#include "core/entry.h"
#include "core/log.h"
#include "core/macros.h"
#include "core/memory.h"
#include "gfx/gfx_internal.h"
#include "gfx/gfx_types.h"
#include "gfx_vtable_gl.h"
#include "utils/internal/xxhash.h"
#include <stdio.h>
#include <string.h>

static GLenum spel_gl_vertex_type(spel_gfx_vertex_base_format base, uint32_t bits);
static GLenum spel_gl_primitive(spel_gfx_primitive_topology t);
static GLenum spel_gl_cull(spel_gfx_cull_mode c);
static GLenum spel_gl_winding(spel_gfx_winding_mode w);
static GLenum spel_gl_compare(spel_gfx_compare_func f);
static GLenum spel_gl_stencil_op(spel_gfx_stencil_op op);
static GLenum spel_gl_blend_factor(spel_gfx_blend_factor f);
static GLenum spel_gl_blend_op(spel_gfx_blend_op op);

static void spel_gl_configure_vertex_layout(GLuint vao,
											const spel_gfx_vertex_layout* layout)
{
	for (uint32_t i = 0; i < layout->stream_count; i++)
	{
		const spel_gfx_vertex_stream* stream = &layout->streams[i];
		glVertexArrayVertexBuffer(vao, i, 0, 0, (int)stream->stride);
		glVertexArrayBindingDivisor(
			vao, i, (stream->rate == SPEL_GFX_VERTEX_RATE_INSTANCE) ? 1 : 0);
	}

	for (uint32_t i = 0; i < layout->attrib_count; i++)
	{
		const spel_gfx_vertex_attrib* attrib = &layout->attribs[i];

		const uint32_t FLAGS = sp_vtx_flags(attrib->format);
		const bool INTEGER = (FLAGS & SPEL_GFX_VERTEX_INTEGER) != 0;
		const bool NORMALIZED = (FLAGS & SPEL_GFX_VERTEX_NORMALIZED) != 0;

		const GLenum TYPE =
			spel_gl_vertex_type(sp_vtx_base(attrib->format), sp_vtx_bits(attrib->format));
		const GLint SIZE = (GLint)sp_vtx_comps(attrib->format);

		glEnableVertexArrayAttrib(vao, attrib->location);

		if (INTEGER)
		{
			glVertexArrayAttribIFormat(vao, attrib->location, SIZE, TYPE, attrib->offset);
		}
		else
		{
			glVertexArrayAttribFormat(vao, attrib->location, SIZE, TYPE,
									  (int)NORMALIZED ? GL_TRUE : GL_FALSE,
									  attrib->offset);
		}

		glVertexArrayAttribBinding(vao, attrib->location, attrib->stream);
	}
}

static void spel_gl_cache_pipeline_state(spel_gfx_pipeline_gl* glPipeline,
										 const spel_gfx_pipeline_desc* desc)
{
	glPipeline->depth_state.test = desc->depth_state.depth_test;
	glPipeline->depth_state.write = desc->depth_state.depth_write;
	glPipeline->depth_state.clamp = desc->depth_state.depth_clamp;
	glPipeline->depth_state.func = spel_gl_compare(desc->depth_state.depth_compare);

	glPipeline->stencil_state.test = desc->stencil.enabled;
	glPipeline->stencil_state.func = spel_gl_compare(desc->stencil.compare);
	glPipeline->stencil_state.read_mask = desc->stencil.read_mask;
	glPipeline->stencil_state.write_mask = desc->stencil.write_mask;
	glPipeline->stencil_state.reference = desc->stencil.reference;
	glPipeline->stencil_state.fail_op = spel_gl_stencil_op(desc->stencil.fail_op);
	glPipeline->stencil_state.depth_op = spel_gl_stencil_op(desc->stencil.depth_fail_op);
	glPipeline->stencil_state.pass_op = spel_gl_stencil_op(desc->stencil.pass_op);

	glPipeline->blend_state.enabled = desc->blend_state.enabled;
	glPipeline->blend_state.src_rgb = spel_gl_blend_factor(desc->blend_state.src_factor);
	glPipeline->blend_state.dst_rgb = spel_gl_blend_factor(desc->blend_state.dst_factor);
	glPipeline->blend_state.op_rgb = spel_gl_blend_op(desc->blend_state.operation);
	glPipeline->blend_state.src_a =
		spel_gl_blend_factor(desc->blend_state.src_alpha_factor);
	glPipeline->blend_state.dst_a =
		spel_gl_blend_factor(desc->blend_state.dst_alpha_factor);
	glPipeline->blend_state.op_a = spel_gl_blend_op(desc->blend_state.alpha_op);
	glPipeline->blend_state.write_mask = desc->blend_state.color_write_mask;

	glPipeline->topology.primitives = spel_gl_primitive(desc->topology);
	glPipeline->topology.cull_mode = spel_gl_cull(desc->cull_mode);
	glPipeline->topology.winding = spel_gl_winding(desc->winding);
}

static void spel_hash_vertex_layout(XXH3_state_t* state,
									const spel_gfx_vertex_layout* layout);

spel_gfx_pipeline spel_gfx_pipeline_create_gl(spel_gfx_context ctx,
											  const spel_gfx_pipeline_desc* desc)
{
	spel_gfx_pipeline pipeline =
		(spel_gfx_pipeline)sp_malloc(sizeof(*pipeline), SPEL_MEM_TAG_GFX);

	XXH3_state_t* state = XXH3_createState();
	XXH3_64bits_reset(state);

	uint8_t shaderCount = 0;
	spel_gfx_shader shaders[3];

	if (desc->vertex_shader != NULL)
	{
		XXH3_64bits_update(state, &desc->vertex_shader->hash,
						   sizeof(desc->vertex_shader->hash));
		shaderCount++;
		shaders[0] = desc->vertex_shader;
	}

	if (desc->fragment_shader != NULL)
	{
		XXH3_64bits_update(state, &desc->fragment_shader->hash,
						   sizeof(desc->fragment_shader->hash));
		shaderCount++;
		shaders[1] = desc->fragment_shader;
	}

	if (desc->geometry_shader != NULL)
	{
		XXH3_64bits_update(state, &desc->geometry_shader->hash,
						   sizeof(desc->geometry_shader->hash));
		shaderCount++;
		shaders[2] = desc->geometry_shader;
	}

	spel_gfx_pipeline_merge_reflections(pipeline, shaders, shaderCount);

	spel_hash_vertex_layout(state, &desc->vertex_layout);

	XXH3_64bits_update(state, &desc->topology, sizeof(desc->topology));
	XXH3_64bits_update(state, &desc->cull_mode, sizeof(desc->cull_mode));
	XXH3_64bits_update(state, &desc->winding, sizeof(desc->winding));

	XXH3_64bits_update(state, &desc->blend_state, sizeof(desc->blend_state));
	XXH3_64bits_update(state, &desc->depth_state, sizeof(desc->depth_state));
	XXH3_64bits_update(state, &desc->stencil, sizeof(desc->stencil));

	XXH3_64bits_update(state, &desc->scissor_test, sizeof(desc->scissor_test));

	pipeline->hash = XXH3_64bits_digest(state);
	XXH3_freeState(state);

	pipeline->ctx = ctx;
	pipeline->type = SPEL_GFX_PIPELINE_GRAPHIC;

	pipeline->data =
		(spel_gfx_pipeline_gl*)sp_malloc(sizeof(spel_gfx_pipeline_gl), SPEL_MEM_TAG_GFX);

	spel_gfx_pipeline_gl* gl_pipeline = (spel_gfx_pipeline_gl*)pipeline->data;
	gl_pipeline->scissor_test = desc->scissor_test;

	glCreateVertexArrays(1, &gl_pipeline->vao);
	spel_gl_configure_vertex_layout(gl_pipeline->vao, &desc->vertex_layout);

	spel_gl_cache_pipeline_state(gl_pipeline, desc);

	gl_pipeline->strides =
		sp_malloc(sizeof(GLsizei) * desc->vertex_layout.stream_count, SPEL_MEM_TAG_GFX);

	for (size_t i = 0; i < desc->vertex_layout.stream_count; i++)
	{
		gl_pipeline->strides[i] = (int)desc->vertex_layout.streams[i].stride;
	}

	spel_gfx_pipeline cached = spel_gfx_pipeline_cache_get_or_create(
		&ctx->pipeline_cache, pipeline->hash, pipeline);

	if (cached != pipeline)
	{
		spel_gfx_pipeline_destroy(pipeline);
		return cached;
	}

	gl_pipeline->program = glCreateProgram();

	if (desc->vertex_shader != NULL)
	{
		glAttachShader(gl_pipeline->program,
					   ((spel_gfx_shader_gl*)desc->vertex_shader->data)->shader);
	}

	if (desc->fragment_shader != NULL)
	{
		glAttachShader(gl_pipeline->program,
					   ((spel_gfx_shader_gl*)desc->fragment_shader->data)->shader);
	}

	if (desc->geometry_shader != NULL)
	{
		glAttachShader(gl_pipeline->program,
					   ((spel_gfx_shader_gl*)desc->geometry_shader->data)->shader);
	}

	glLinkProgram(gl_pipeline->program);

	int status;
	glGetProgramiv(gl_pipeline->program, GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		char info_log[512];
		GLsizei info_log_size = 0;
		glGetProgramInfoLog(gl_pipeline->program, sizeof(info_log), &info_log_size,
							(GLchar*)info_log);

		char str[24];
		snprintf(str, sizeof(str), "%lx", pipeline->hash);

		spel_gfx_shader_log log = {.name = str,
								   .name_size = strlen(str),
								   .log = info_log,
								   .log_size = info_log_size};

		sp_log(SPEL_SEV_ERROR, SPEL_ERR_SHADER_FAILED, &log, SPEL_DATA_SHADER_LOG,
			   sizeof(log), "failed to compile pipeline %s: %s", str, info_log);

		glDeleteProgram(gl_pipeline->program);
		spel_gfx_pipeline_destroy(pipeline);
		return NULL;
	}

	return pipeline;
}

void spel_gfx_pipeline_destroy_gl(spel_gfx_pipeline pipeline)
{
	spel_gfx_pipeline_gl* glp = (spel_gfx_pipeline_gl*)pipeline->data;

	if (glp->strides)
	{
		sp_free(glp->strides);
	}

	if (glp->vao)
	{
		glDeleteVertexArrays(1, &glp->vao);
	}

	if (glp->program)
	{
		glDeleteProgram(glp->program);
	}

	sp_free(pipeline->data);
	sp_free(pipeline);
}

static GLenum spel_gl_vertex_type(spel_gfx_vertex_base_format base, uint32_t bits)
{
	switch (base)
	{
	case SPEL_GFX_VERTEX_FLOAT:
		return GL_FLOAT; // bits must be 32

	case SPEL_GFX_VERTEX_HALF:
		return GL_HALF_FLOAT; // bits must be 16

	case SPEL_GFX_VERTEX_INT:
		switch (bits)
		{
		case 8:
			return GL_BYTE;
		case 16:
			return GL_SHORT;
		case 32:
		default:
			return GL_INT;
		}
		break;

	case SPEL_GFX_VERTEX_UINT:
		switch (bits)
		{
		case 8:
			return GL_UNSIGNED_BYTE;
		case 16:
			return GL_UNSIGNED_SHORT;
		case 32:
		default:
			return GL_UNSIGNED_INT;
		}
		break;
	}

	sp_error(SPEL_ERR_INVALID_ARGUMENT, "invalid vertex format");
	return GL_FLOAT;
}

static GLenum spel_gl_primitive(spel_gfx_primitive_topology t)
{
	switch (t)
	{
	case SPEL_GFX_TOPOLOGY_TRIANGLES:
		return GL_TRIANGLES;
	case SPEL_GFX_TOPOLOGY_TRIANGLE_STRIP:
		return GL_TRIANGLE_STRIP;
	case SPEL_GFX_TOPOLOGY_LINES:
		return GL_LINES;
	case SPEL_GFX_TOPOLOGY_LINE_STRIP:
		return GL_LINE_STRIP;
	case SPEL_GFX_TOPOLOGY_POINTS:
		return GL_POINTS;
	default:
		return GL_TRIANGLES;
	}
}

static GLenum spel_gl_cull(spel_gfx_cull_mode c)
{
	switch (c)
	{
	case SPEL_GFX_CULL_NONE:
		return 0;
	case SPEL_GFX_CULL_BACK:
		return GL_BACK;
	case SPEL_GFX_CULL_FRONT:
		return GL_FRONT;
	default:
		return GL_BACK;
	}
}

static GLenum spel_gl_winding(spel_gfx_winding_mode w)
{
	switch (w)
	{
	case SPEL_GFX_WINDING_COUNTER_CLOCKWISE:
		return GL_CCW;
	case SPEL_GFX_WINDING_CLOCKWISE:
		return GL_CW;
	default:
		return GL_CCW;
	}
}

static GLenum spel_gl_compare(spel_gfx_compare_func f)
{
	switch (f)
	{
	case SPEL_GFX_COMPARE_NEVER:
		return GL_NEVER;
	case SPEL_GFX_COMPARE_LESS:
		return GL_LESS;
	case SPEL_GFX_COMPARE_LEQUAL:
		return GL_LEQUAL;
	case SPEL_GFX_COMPARE_EQUAL:
		return GL_EQUAL;
	case SPEL_GFX_COMPARE_GREATER:
		return GL_GREATER;
	case SPEL_GFX_COMPARE_GEQUAL:
		return GL_GEQUAL;
	case SPEL_GFX_COMPARE_NOTEQUAL:
		return GL_NOTEQUAL;
	case SPEL_GFX_COMPARE_ALWAYS:
		return GL_ALWAYS;
	default:
		return GL_LESS;
	}
}

static GLenum spel_gl_stencil_op(spel_gfx_stencil_op op)
{
	switch (op)
	{
	case SPEL_GFX_STENCIL_KEEP:
		return GL_KEEP;
	case SPEL_GFX_STENCIL_ZERO:
		return GL_ZERO;
	case SPEL_GFX_STENCIL_REPLACE:
		return GL_REPLACE;
	case SPEL_GFX_STENCIL_INCREASE:
		return GL_INCR;
	case SPEL_GFX_STENCIL_INCR_WRAP:
		return GL_INCR_WRAP;
	case SPEL_GFX_STENCIL_DECREASE:
		return GL_DECR;
	case SPEL_GFX_STENCIL_DECR_WRAP:
		return GL_DECR_WRAP;
	case SPEL_GFX_STENCIL_INVERT:
		return GL_INVERT;
	default:
		return GL_KEEP;
	}
}

static GLenum spel_gl_blend_factor(spel_gfx_blend_factor f)
{
	switch (f)
	{
	case SPEL_GFX_BLEND_ZERO:
		return GL_ZERO;
	case SPEL_GFX_BLEND_ONE:
		return GL_ONE;
	case SPEL_GFX_BLEND_SRC_ALPHA:
		return GL_SRC_ALPHA;
	case SPEL_GFX_BLEND_ONE_MINUS_SRC_ALPHA:
		return GL_ONE_MINUS_SRC_ALPHA;
	case SPEL_GFX_BLEND_DST_ALPHA:
		return GL_DST_ALPHA;
	case SPEL_GFX_BLEND_ONE_MINUS_DST_ALPHA:
		return GL_ONE_MINUS_DST_ALPHA;
	case SPEL_GFX_BLEND_SRC_COLOR:
		return GL_SRC_COLOR;
	case SPEL_GFX_BLEND_ONE_MINUS_SRC_COLOR:
		return GL_ONE_MINUS_SRC_COLOR;
	case SPEL_GFX_BLEND_DST_COLOR:
		return GL_DST_COLOR;
	case SPEL_GFX_BLEND_ONE_MINUS_DST_COLOR:
		return GL_ONE_MINUS_DST_COLOR;
	}

	sp_error(SPEL_ERR_INVALID_ARGUMENT, "invalid blend factor");
	return GL_ONE;
}

static GLenum spel_gl_blend_op(spel_gfx_blend_op op)
{
	switch (op)
	{
	case SPEL_GFX_BLEND_OP_ADD:
		return GL_FUNC_ADD;
	case SPEL_GFX_BLEND_OP_SUBTRACT:
		return GL_FUNC_SUBTRACT;
	case SPEL_GFX_BLEND_OP_REV_SUBTRACT:
		return GL_FUNC_REVERSE_SUBTRACT;
	case SPEL_GFX_BLEND_OP_MIN:
		return GL_MIN;
	case SPEL_GFX_BLEND_OP_MAX:
		return GL_MAX;
	}

	sp_error(SPEL_ERR_INVALID_ARGUMENT, "invalid blend op");
	return GL_FUNC_ADD;
}

static void spel_hash_vertex_layout(XXH3_state_t* state,
									const spel_gfx_vertex_layout* layout)
{
	XXH3_64bits_update(state, &layout->attrib_count, sizeof(layout->attrib_count));

	for (uint32_t i = 0; i < layout->attrib_count; ++i)
	{
		const spel_gfx_vertex_attrib* a = &layout->attribs[i];

		XXH3_64bits_update(state, &a->location, sizeof(a->location));
		XXH3_64bits_update(state, &a->format, sizeof(a->format));
		XXH3_64bits_update(state, &a->offset, sizeof(a->offset));
		XXH3_64bits_update(state, &a->stream, sizeof(a->stream));
	}

	XXH3_64bits_update(state, &layout->stream_count, sizeof(layout->stream_count));

	for (uint32_t i = 0; i < layout->stream_count; ++i)
	{
		const spel_gfx_vertex_stream* s = &layout->streams[i];

		XXH3_64bits_update(state, &s->stride, sizeof(s->stride));
		XXH3_64bits_update(state, &s->rate, sizeof(s->rate));
	}
}
