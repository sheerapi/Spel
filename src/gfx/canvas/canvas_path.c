#include "gfx/canvas/canvas_path.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/canvas/canvas_internal.h"
#include "gfx/gfx_internal.h"

void spel_canvas_path_begin()
{
	if (spel.gfx->canvas_ctx == NULL)
	{
		spel_canvas_ctx_create(spel.gfx);
	}

	spel.gfx->canvas_ctx->current_path.cmd_offset = 0;
	spel.gfx->canvas_ctx->current_path.point_offset = 0;
	spel.gfx->canvas_ctx->current_path.closed = false;
	spel.gfx->canvas_ctx->current_path.cursor = spel_vec2_zero;
	spel.gfx->canvas_ctx->current_path.start = spel_vec2_zero;

	spel.gfx->canvas_ctx->current_path.cmd_count = 0;
	spel.gfx->canvas_ctx->current_path.point_count = 0;
}

void spel_canvas_path_close()
{
	if (spel.gfx->canvas_ctx == NULL)
	{
		spel_canvas_ctx_create(spel.gfx);
	}

	spel_assert(!spel.gfx->canvas_ctx->current_path.closed, "path already closed");

	spel_path_cmd* cmd = spel_canvas_path_alloc();
	cmd->type = SPEL_PATH_CLOSE;

	spel.gfx->canvas_ctx->current_path.cursor = spel.gfx->canvas_ctx->current_path.start;
	spel.gfx->canvas_ctx->current_path.closed = true;
}

void spel_canvas_path_moveto(spel_vec2 position)
{
	spel_assert(!spel.gfx->canvas_ctx->current_path.closed,
				"path_moveto called outside path_begin/end");

	spel_path_cmd* cmd = spel_canvas_path_alloc();
	cmd->type = SPEL_PATH_MOVE_TO;
	cmd->move = position;

	spel.gfx->canvas_ctx->current_path.cursor = position;
	spel.gfx->canvas_ctx->current_path.start = position;
}

void spel_canvas_path_lineto(spel_vec2 position)
{
	spel_assert(!spel.gfx->canvas_ctx->current_path.closed,
				"path_lineto called outside path_begin/end");

	spel_path_cmd* cmd = spel_canvas_path_alloc();
	cmd->type = SPEL_PATH_LINE_TO;
	cmd->move = position;
	spel.gfx->canvas_ctx->current_path.cursor = position;
}

void spel_canvas_path_bezierto(spel_vec2 control1, spel_vec2 control2, spel_vec2 position)
{
	spel_assert(!spel.gfx->canvas_ctx->current_path.closed,
				"path_bezierto called outside path_begin/end");

	spel_path_cmd* cmd = spel_canvas_path_alloc();
	cmd->type = SPEL_PATH_BEZIER_TO;
	cmd->bezier.control1 = control1;
	cmd->bezier.control2 = control2;
	cmd->bezier.position = position;
	spel.gfx->canvas_ctx->current_path.cursor = position;
}

void spel_canvas_path_quadto(spel_vec2 control, spel_vec2 position)
{
	spel_assert(!spel.gfx->canvas_ctx->current_path.closed,
				"path_quadto called outside path_begin/end");

	float x0 = spel.gfx->canvas_ctx->current_path.cursor.x;
	float y0 = spel.gfx->canvas_ctx->current_path.cursor.y;

	float cx0 = x0 + (2.0F / 3.0F * (control.x - x0));
	float cy0 = y0 + (2.0F / 3.0F * (control.y - y0));
	float cx1 = position.x + (2.0F / 3.0F * (control.x - position.x));
	float cy1 = position.y + (2.0F / 3.0F * (control.y - position.y));

	spel_path_cmd* cmd = spel_canvas_path_alloc();
	cmd->type = SPEL_PATH_BEZIER_TO;
	cmd->bezier.control1 = spel_vec2(cx0, cy0);
	cmd->bezier.control2 = spel_vec2(cx1, cy1);
	cmd->bezier.position = position;

	spel.gfx->canvas_ctx->current_path.cursor = position;
}

// actual work
void spel_canvas_path_fill()
{
	spel_assert(spel.gfx->canvas_ctx->current_path.closed,
				"path_fill called outside path_begin");

	spel_canvas_path_tessellate();
	spel_canvas_path_compute_directions();
	spel_canvas_path_compute_normals();

	spel_canvas_fill_path(spel.gfx->canvas_ctx->fill_paint);

	spel.gfx->canvas_ctx->current_path.closed = true;
}

void spel_canvas_path_stroke()
{
	spel_assert(spel.gfx->canvas_ctx->current_path.closed,
				"path_fill called outside path_begin");

	spel_canvas_path_tessellate();
	spel_canvas_path_compute_directions();
	spel_canvas_path_compute_normals();

	spel_canvas_stroke_path(spel.gfx->canvas_ctx->stroke_paint,
							spel.gfx->canvas_ctx->line_width);

	spel.gfx->canvas_ctx->current_path.closed = true;
}
void spel_canvas_path_fill_stroke()
{
	spel_assert(spel.gfx->canvas_ctx->current_path.closed,
				"path_fill called outside path_begin");

	spel_canvas_path_tessellate();
	spel_canvas_path_compute_directions();
	spel_canvas_path_compute_normals();

	spel_canvas_fill_path(spel.gfx->canvas_ctx->fill_paint);
	spel_canvas_stroke_path(spel.gfx->canvas_ctx->stroke_paint,
							spel.gfx->canvas_ctx->line_width);

	spel.gfx->canvas_ctx->current_path.closed = true;
}

void spel_canvas_path_tessellate()
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;

	path->point_count = 0;

	float cx = 0;
	float cy = 0;

	uint8_t* ptr = path->cmds;
	while (ptr < path->cmds + path->cmd_offset)
	{
		spel_path_cmd* cmd = (spel_path_cmd*)ptr;

		switch (cmd->type)
		{
		case SPEL_PATH_MOVE_TO:
			cx = cmd->move.x;
			cy = cmd->move.y;
			spel_canvas_path_point_add(spel_vec2(cx, cy), spel_point_corner);
			break;

		case SPEL_PATH_LINE_TO:
		{
			float dx = cmd->move.x - cx;
			float dy = cmd->move.y - cy;
			if ((dx * dx) + (dy * dy) < spel_canvas_dist_tol * spel_canvas_dist_tol)
			{
				break;
			}
			cx = cmd->move.x;
			cy = cmd->move.y;
			spel_canvas_path_point_add(spel_vec2(cx, cy), spel_point_corner);
			break;
		}

		case SPEL_PATH_BEZIER_TO:
			spel_canvas_path_tessellate_bezier(spel_vec2(cx, cy), cmd->bezier.control1,
											   cmd->bezier.control2, cmd->bezier.position,
											   0);
			cx = cmd->bezier.position.x;
			cy = cmd->bezier.position.y;
			break;

		case SPEL_PATH_CLOSE:
			path->closed = true;
			break;
		}

		ptr += cmd->size;
	}
}

void spel_canvas_path_compute_directions()
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;

	uint32_t n = path->point_count;
	if (n < 2)
	{
		return;
	}

	uint32_t last = path->closed ? n : n - 1;

	uint8_t* ptr = path->points;
	uint32_t i = 0;
	while (ptr < path->points + path->point_offset && i < last)
	{
		spel_path_point* p0 = (spel_path_point*)ptr;
		spel_path_point* p1 = spel_canvas_path_point_get((i + 1) % n);

		float dx = p1->position.x - p0->position.x;
		float dy = p1->position.y - p0->position.y;
		float len = sqrtf((dx * dx) + (dy * dy));

		if (len > spel_canvas_dist_tol)
		{
			p0->direction.x = dx / len;
			p0->direction.y = dy / len;
			p0->len = len;
		}
		else
		{
			p0->direction.x = 0;
			p0->direction.y = 0;
			p0->len = 0;
		}

		ptr += p0->size;
		i++;
	}
}

void spel_canvas_path_compute_normals()
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;
	uint32_t n = path->point_count;
	if (n < 2)
	{
		return;
	}

	uint8_t* ptr = path->points;
	uint32_t i = 0;
	while (ptr < path->points + path->point_offset)
	{
		spel_path_point* p0 = spel_canvas_path_point_get((i + n - 1) % n);
		spel_path_point* p1 = (spel_path_point*)ptr;

		float dmx = p0->direction.x + p1->direction.x;
		float dmy = p0->direction.y + p1->direction.y;

		float dmr2 = (dmx * dmx) + (dmy * dmy);
		if (dmr2 > spel_epsilonf)
		{
			float scale = 1.0F / dmr2;
			if (scale > 600.0F)
			{
				scale = 600.0F;
			}
			dmx *= scale;
			dmy *= scale;
		}

		p1->miter_direction.x = dmx;
		p1->miter_direction.y = dmy;

		float cross =
			(p1->direction.x * p0->direction.y) - (p0->direction.x * p1->direction.y);
		if (cross > 0.0F)
			p1->flags |= spel_point_left;

		float limit = spel_math_maxf(1.01F, spel_math_minf(p0->len, p1->len) * 0.5F);
		if ((dmx * dmx + dmy * dmy) * limit * limit < 1.0F)
		{
			p1->flags |= spel_point_inner_bevel;
		}

		if (p1->flags & spel_point_corner)
		{
			if ((dmx * dmx + dmy * dmy) * spel.gfx->canvas_ctx->miter_limit *
					spel.gfx->canvas_ctx->miter_limit <
				1.0F)
			{
				p1->flags |= spel_point_bevel;
			}
		}

		ptr += p1->size;
		i++;
	}
}

void spel_canvas_path_tessellate_bezier(spel_vec2 start, spel_vec2 control1,
										spel_vec2 control2, spel_vec2 end, int depth)
{
	if (depth > 10)
	{
		spel_canvas_path_point_add(end, 0);
		return;
	}

	float x01 = (start.x + control1.x) * 0.5F;
	float y01 = (start.y + control1.y) * 0.5F;
	float x12 = (control1.x + control2.x) * 0.5F;
	float y12 = (control1.y + control2.y) * 0.5F;
	float x23 = (control2.x + end.x) * 0.5F;
	float y23 = (control2.y + end.y) * 0.5F;
	float x012 = (x01 + x12) * 0.5F;
	float y012 = (y01 + y12) * 0.5F;
	float x123 = (x12 + x23) * 0.5F;
	float y123 = (y12 + y23) * 0.5F;
	float x0123 = (x012 + x123) * 0.5F;
	float y0123 = (y012 + y123) * 0.5F;

	float dx = end.x - start.x;
	float dy = end.y - start.y;
	float d1 = fabsf(((control1.x - end.x) * dy) - ((control1.y - end.y) * dx));
	float d2 = fabsf(((control2.x - end.x) * dy) - ((control2.y - end.y) * dx));

	if ((d1 + d2) * (d1 + d2) < spel_canvas_tess_tol * (dx * dx + dy * dy))
	{
		spel_canvas_path_point_add(end, 0);
		return;
	}

	spel_canvas_path_tessellate_bezier(start, spel_vec2(x01, y01), spel_vec2(x012, y012),
									   spel_vec2(x0123, y0123), depth + 1);
	spel_canvas_path_tessellate_bezier(spel_vec2(x0123, y0123), spel_vec2(x123, y123),
									   spel_vec2(x23, y23), end, depth + 1);
}

// filling and stroking
void spel_canvas_fill_path(spel_canvas_paint paint)
{
	if (spel.gfx->canvas_ctx->current_path.point_count < 3)
	{
		return;
	}

	if (spel_canvas_path_convex())
	{
		spel_canvas_fill_path_convex(paint);
	}
	else
	{
		spel_canvas_fill_path_concave(paint);
	}
}

void spel_canvas_stroke_path(spel_canvas_paint paint, float width)
{
}

void spel_canvas_fill_path_convex(spel_canvas_paint paint)
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;

	uint32_t n = path->point_count;
	if (n < 3)
	{
		return;
	}

	spel_canvas_check_batch(spel.gfx->canvas_ctx->white_texture, spel.gfx->canvas_ctx);
	spel_canvas_ensure_capacity(n, (n - 2) * 3);

	int base = spel.gfx->canvas_ctx->vert_count;
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];

	// push all points as vertices
	uint8_t* ptr = path->points;
	uint32_t i = 0;
	while (ptr < path->points + path->point_offset)
	{
		spel_path_point* p = (spel_path_point*)ptr;

		spel.gfx->canvas_ctx->verts[base + i] = (spel_canvas_vertex){
			spel_mat3_transform_point(t, p->position), {0, 0}, paint.color};
		
		ptr += p->size;
		i++;
	}
	// fan from first vertex
	for (int i = 0; i < n - 2; i++)
	{
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count + (i * 3) + 0] =
			base;
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count + (i * 3) + 1] =
			base + i + 1;
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count + (i * 3) + 2] =
			base + i + 2;
	}

	spel.gfx->canvas_ctx->vert_count += n;
	spel.gfx->canvas_ctx->index_count += (n - 2) * 3;
}

void spel_canvas_fill_path_concave(spel_canvas_paint paint)
{
}

// utilities
bool spel_canvas_path_convex()
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;

	uint32_t n = path->point_count;
	if (n < 3)
	{
		return false;
	}

	int sign = 0;
	uint8_t* ptr = path->points;
	uint32_t i = 0;
	while (ptr < path->points + path->point_offset)
	{
		spel_path_point* p0 = (spel_path_point*)ptr;

		spel_path_point* p1 = spel_canvas_path_point_get((i + 1) % n);
		spel_path_point* p2 = spel_canvas_path_point_get((i + 2) % n);

		float cross =
			((p1->position.x - p0->position.x) * (p2->position.y - p1->position.y)) -
			((p1->position.y - p0->position.y) * (p2->position.x - p1->position.y));

		if (cross > 0)
		{
			if (sign == -1)
			{
				return false;
			}
			sign = 1;
		}
		else if (cross < 0)
		{
			if (sign == 1)
			{
				return false;
			}
			sign = -1;
		}

		ptr += p0->size;
		i++;
	}
	return true;
}

spel_path_point* spel_canvas_path_point_get(uint32_t index)
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;
	uint32_t i = 0;

	uint8_t* ptr = path->points;
	while (ptr < path->points + path->point_offset)
	{
		if (i == index)
		{
			return (spel_path_point*)ptr;
		}

		ptr += ((spel_path_point*)ptr)->size;
		i++;
	}
	return NULL;
}
void spel_canvas_path_dump()
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;

	spel_debug("canvas_path_dump: %d commands, cursor (%.2f, %.2f)", path->cmd_count,
			   path->cursor.x, path->cursor.y);

	uint8_t* ptr = path->cmds;
	uint32_t i = 0;
	while (ptr < path->cmds + path->cmd_offset)
	{
		spel_path_cmd* cmd = (spel_path_cmd*)ptr;

		switch (cmd->type)
		{
		case SPEL_PATH_MOVE_TO:
			spel_debug("  [%d] move to    (%.2f, %.2f)", i, cmd->move.x, cmd->move.y);
			break;
		case SPEL_PATH_LINE_TO:
			spel_debug("  [%d] line to    (%.2f, %.2f)", i, cmd->move.x, cmd->move.y);
			break;
		case SPEL_PATH_BEZIER_TO:
			spel_debug(
				"  [%d] bezier to  cp0(%.2f, %.2f) cp1(%.2f, %.2f) end(%.2f, %.2f)", i,
				cmd->bezier.control1.x, cmd->bezier.control1.y, cmd->bezier.control2.x,
				cmd->bezier.control2.y, cmd->bezier.position.x, cmd->bezier.position.y);
			break;
		case SPEL_PATH_CLOSE:
			spel_debug("  [%d] close", i);
			break;
		}

		ptr += cmd->size;
		i++;
	}
}

void spel_canvas_path_points_dump()
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;

	spel_debug("canvas_path_points_dump: %d points", path->point_count);

	uint8_t* ptr = path->points;
	uint32_t i = 0;
	while (ptr < path->points + path->point_offset)
	{
		spel_path_point* p = (spel_path_point*)ptr;

		spel_debug("  [%d] (%.2f, %.2f) dx(%.2f, %.2f) len(%.2f) flags(%d)", i,
				   p->position.x, p->position.y, p->direction.x, p->direction.y, p->len,
				   p->flags);

		ptr += p->size;
		i++;
	}
}

spel_path_cmd* spel_canvas_path_alloc()
{
	if (spel.gfx->canvas_ctx == NULL)
	{
		spel_canvas_ctx_create(spel.gfx);
	}

	const size_t MAX_ALIGN = _Alignof(max_align_t);
	uint64_t start_offset = spel.gfx->canvas_ctx->current_path.cmd_offset;

	uint8_t align = _Alignof(spel_path_cmd);

	uint64_t aligned =
		(spel.gfx->canvas_ctx->current_path.cmd_offset + (align - 1)) & ~(align - 1);
	uint64_t end = aligned + sizeof(spel_path_cmd);
	uint64_t padded_end = (end + (MAX_ALIGN - 1)) & ~(MAX_ALIGN - 1);

	while (padded_end > spel.gfx->canvas_ctx->current_path.cmd_capacity)
	{
		spel.gfx->canvas_ctx->current_path.cmd_capacity *= 2;
		void* new_buffer = spel_memory_realloc(
			spel.gfx->canvas_ctx->current_path.cmds,
			spel.gfx->canvas_ctx->current_path.cmd_capacity, SPEL_MEM_TAG_GFX);
		if (new_buffer == NULL)
		{
			spel_error(SPEL_ERR_OOM, "out of memory?");
			return NULL;
		}
		spel.gfx->canvas_ctx->current_path.cmds = new_buffer;
	}

	void* ptr = spel.gfx->canvas_ctx->current_path.cmds + aligned;
	spel.gfx->canvas_ctx->current_path.cmd_offset = padded_end;
	spel.gfx->canvas_ctx->current_path.cmd_count++;
	((spel_path_cmd*)ptr)->size =
		(uint8_t)(spel.gfx->canvas_ctx->current_path.cmd_offset - start_offset);
	return (spel_path_cmd*)ptr;
}

void spel_canvas_path_point_add(spel_vec2 position, uint16_t flags)
{
	if (spel.gfx->canvas_ctx == NULL)
	{
		spel_canvas_ctx_create(spel.gfx);
	}

	const size_t MAX_ALIGN = _Alignof(max_align_t);
	uint64_t start_offset = spel.gfx->canvas_ctx->current_path.point_offset;

	uint8_t align = _Alignof(spel_path_point);

	uint64_t aligned =
		(spel.gfx->canvas_ctx->current_path.point_offset + (align - 1)) & ~(align - 1);
	uint64_t end = aligned + sizeof(spel_path_point);
	uint64_t padded_end = (end + (MAX_ALIGN - 1)) & ~(MAX_ALIGN - 1);

	while (padded_end > spel.gfx->canvas_ctx->current_path.point_capacity)
	{
		spel.gfx->canvas_ctx->current_path.point_capacity *= 2;
		void* new_buffer = spel_memory_realloc(
			spel.gfx->canvas_ctx->current_path.points,
			spel.gfx->canvas_ctx->current_path.point_capacity, SPEL_MEM_TAG_GFX);
		if (new_buffer == NULL)
		{
			spel_error(SPEL_ERR_OOM, "out of memory?");
			return;
		}
		spel.gfx->canvas_ctx->current_path.points = new_buffer;
	}

	void* ptr = spel.gfx->canvas_ctx->current_path.points + aligned;
	spel.gfx->canvas_ctx->current_path.point_offset = padded_end;
	spel.gfx->canvas_ctx->current_path.point_count++;

	spel_path_point* point = ((spel_path_point*)ptr);

	point->size =
		(uint8_t)(spel.gfx->canvas_ctx->current_path.point_offset - start_offset);
	point->position = position;
	point->flags = flags;
	point->direction = spel_vec2_zero;
	point->miter_direction = spel_vec2_zero;
	point->len = 0;
}