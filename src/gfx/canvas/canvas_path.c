#include "gfx/canvas/canvas_path.h"
#include "core/log.h"
#include "core/memory.h"
#include "gfx/canvas/canvas_internal.h"
#include "gfx/gfx_internal.h"
#include <string.h>

void spel_canvas_path_begin()
{
	if (spel.gfx->canvas_ctx == NULL)
	{
		spel_canvas_ctx_create(spel.gfx);
	}

	spel.gfx->canvas_ctx->current_path.cmd_offset = 0;
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
	spel_canvas_fill_path(&spel.gfx->canvas_ctx->fill_paint);

	spel.gfx->canvas_ctx->current_path.closed = true;
}

void spel_canvas_path_stroke()
{
	spel_canvas_path_tessellate();
	spel_canvas_path_compute_directions();
	spel_canvas_path_compute_normals();

	spel_canvas_stroke_path(&spel.gfx->canvas_ctx->stroke_paint,
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

	spel_canvas_fill_path(&spel.gfx->canvas_ctx->fill_paint);
	spel_canvas_stroke_path(&spel.gfx->canvas_ctx->stroke_paint,
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

	for (size_t i = 0; i < last; i++)
	{
		spel_path_point* p0 = &path->points[i];
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
	}

	// for open paths, give the last point a direction so caps don't degenerate
	if (!path->closed && n >= 2)
	{
		spel_path_point* plast = &path->points[n - 1];
		spel_path_point* pprev = &path->points[n - 2];
		plast->direction = pprev->direction;
		plast->len = pprev->len;
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

	for (size_t i = 0; i < path->point_count; i++)
	{
		spel_path_point* p0 = spel_canvas_path_point_get((i + n - 1) % n);
		spel_path_point* p1 = &path->points[i];

		float dmx = (p0->direction.x + p1->direction.x) * 0.5F;
		float dmy = (p0->direction.y + p1->direction.y) * 0.5F;

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
		{
			p1->flags |= spel_point_left;
		}

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
void spel_canvas_fill_path(spel_canvas_paint* paint)
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

void spel_canvas_stroke_scratch_ensure(int needed)
{
	if (needed <= spel.gfx->canvas_ctx->stroke_scratch_capacity)
	{
		return;
	}
	int cap = spel.gfx->canvas_ctx->stroke_scratch_capacity
				  ? spel.gfx->canvas_ctx->stroke_scratch_capacity * 2
				  : 64;
	while (cap < needed)
		cap *= 2;
	spel.gfx->canvas_ctx->stroke_point_bases = spel_memory_realloc(
		spel.gfx->canvas_ctx->stroke_point_bases, cap * sizeof(int), SPEL_MEM_TAG_GFX);
	spel.gfx->canvas_ctx->stroke_is_double = spel_memory_realloc(
		spel.gfx->canvas_ctx->stroke_is_double, cap * sizeof(bool), SPEL_MEM_TAG_GFX);
	spel.gfx->canvas_ctx->stroke_scratch_capacity = cap;
}

void spel_canvas_stroke_path(spel_canvas_paint* paint, float width)
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;

	if (path->point_count < 2 || width <= 0)
		return;

	spel_color color = paint->color;
	float w = width * 0.5f;
	int n = path->point_count;

	spel_canvas_stroke_scratch_ensure(n);
	spel_canvas_check_batch(spel.gfx->canvas_ctx->white_texture, spel.gfx->canvas_ctx);

	spel_canvas_stroke_basic(path, w, color, spel.gfx->canvas_ctx->stroke_point_bases,
							 spel.gfx->canvas_ctx->stroke_is_double);

	// round cap start
	if (!path->closed && spel.gfx->canvas_ctx->cap_type == SPEL_CANVAS_CAP_ROUND)
	{
		spel_canvas_cap_round_connected(&path->points[0], w, color, true,
										spel.gfx->canvas_ctx->stroke_point_bases[0]);
		spel_canvas_cap_round_connected(&path->points[n - 1], w, color, false,
										spel.gfx->canvas_ctx->stroke_point_bases[n - 1]);
	}

	// joins
	for (int i = 1; i < (path->closed ? n : n - 1); i++)
	{
		if (!spel.gfx->canvas_ctx->stroke_is_double[i])
			continue;

		spel_path_point* p0 = &path->points[i - 1];
		spel_path_point* p1 = &path->points[i];
		int vbase = spel.gfx->canvas_ctx->stroke_point_bases[i];

		if (spel.gfx->canvas_ctx->join_type == SPEL_CANVAS_JOIN_ROUND)
			spel_canvas_join_round_connected(p0, p1, w, color, vbase);
		else
			spel_canvas_join_bevel_connected(p0, p1, w, color, vbase);
	}

	// wrap-around join for closed path
	if (path->closed && spel.gfx->canvas_ctx->stroke_is_double[0])
	{
		spel_path_point* p0 = &path->points[n - 1];
		spel_path_point* p1 = &path->points[0];
		int vbase = spel.gfx->canvas_ctx->stroke_point_bases[0];

		if (spel.gfx->canvas_ctx->join_type == SPEL_CANVAS_JOIN_ROUND)
			spel_canvas_join_round_connected(p0, p1, w, color, vbase);
		else
			spel_canvas_join_bevel_connected(p0, p1, w, color, vbase);
	}
}

void spel_canvas_fill_path_convex(spel_canvas_paint* paint)
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
	for (size_t i = 0; i < path->point_count; i++)
	{
		spel_path_point* p = &path->points[i];

		spel.gfx->canvas_ctx->verts[base + i] = (spel_canvas_vertex){
			spel_mat3_transform_point(t, p->position), {0, 0}, paint->color};
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

static void spel_canvas_ear_ensure(int needed)
{
	if (needed <= spel.gfx->canvas_ctx->ear_cap)
	{
		return;
	}

	int new_cap = spel.gfx->canvas_ctx->ear_cap ? spel.gfx->canvas_ctx->ear_cap * 2 : 64;
	while (new_cap < needed)
	{
		new_cap *= 2;
	}

	spel.gfx->canvas_ctx->ear_indices = spel_memory_realloc(
		spel.gfx->canvas_ctx->ear_indices, new_cap * sizeof(int), SPEL_MEM_TAG_GFX);
	spel.gfx->canvas_ctx->ear_active = spel_memory_realloc(
		spel.gfx->canvas_ctx->ear_active, new_cap * sizeof(bool), SPEL_MEM_TAG_GFX);
	spel.gfx->canvas_ctx->ear_cap = new_cap;

	memset(spel.gfx->canvas_ctx->ear_active, 1, spel.gfx->canvas_ctx->ear_cap);
}

static bool spel_canvas_point_in_triangle(spel_path_point* p, spel_path_point* a,
										  spel_path_point* b, spel_path_point* c)
{
	float d1 = ((p->position.x - b->position.x) * (a->position.y - b->position.y)) -
			   ((a->position.x - b->position.x) * (p->position.y - b->position.y));
	float d2 = ((p->position.x - c->position.x) * (b->position.y - c->position.y)) -
			   ((b->position.x - c->position.x) * (p->position.y - c->position.y));
	float d3 = ((p->position.x - a->position.x) * (c->position.y - a->position.y)) -
			   ((c->position.x - a->position.x) * (p->position.y - a->position.y));

	bool has_neg = (d1 < -spel_epsilon) || (d2 < -spel_epsilon) || (d3 < -spel_epsilon);
	bool has_pos = (d1 > spel_epsilon) || (d2 > spel_epsilon) || (d3 > spel_epsilon);

	return !(has_neg && has_pos);
}

void spel_canvas_fill_path_concave(spel_canvas_paint* paint)
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;

	uint32_t n = path->point_count;
	if (n < 3)
	{
		return;
	}

	spel_canvas_ear_ensure(path->point_count);

	float area = 0;
	for (size_t i = 0; i < path->point_count; i++)
	{
		spel_path_point* a = &path->points[i];
		spel_path_point* b = spel_canvas_path_point_get((i + 1) % n);

		area += spel_vec2_cross(a->position, b->position);
	}

	spel_canvas_check_batch(spel.gfx->canvas_ctx->white_texture, spel.gfx->canvas_ctx);
	spel_canvas_ensure_capacity(n, (n - 2) * 3);

	int base = spel.gfx->canvas_ctx->vert_count;
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];

	for (int i = 0; i < n; i++)
	{
		if (area < 0)
		{
			spel.gfx->canvas_ctx->ear_indices[i] = n - 1 - i;
		}
		else
		{
			spel.gfx->canvas_ctx->ear_indices[i] = i;
		}

		int idx = spel.gfx->canvas_ctx->ear_indices[i];

		spel_path_point* point = spel_canvas_path_point_get(idx);

		spel.gfx->canvas_ctx->verts[base + i] = (spel_canvas_vertex){
			spel_mat3_transform_point(t, point->position), {0, 0}, paint->color};
	}

	int remaining = n;
	int out = spel.gfx->canvas_ctx->index_count;

	while (remaining > 3)
	{
		bool found = false;

		for (int step = 0; step < remaining; step++)
		{
			int a = step % remaining;
			int b = (step + 1) % remaining;
			int c = (step + 2) % remaining;

			spel_path_point* pa =
				spel_canvas_path_point_get(spel.gfx->canvas_ctx->ear_indices[a]);
			spel_path_point* pb =
				spel_canvas_path_point_get(spel.gfx->canvas_ctx->ear_indices[b]);
			spel_path_point* pc =
				spel_canvas_path_point_get(spel.gfx->canvas_ctx->ear_indices[c]);

			float cross =
				((pb->position.x - pa->position.x) * (pc->position.y - pa->position.y)) -
				((pb->position.y - pa->position.y) * (pc->position.x - pa->position.x));
			if (cross <= 0)
			{
				continue;
			}

			bool ear = true;
			for (int j = 0; j < remaining; j++)
			{
				if (j == a || j == b || j == c)
				{
					continue;
				}
				spel_path_point* p =
					spel_canvas_path_point_get(spel.gfx->canvas_ctx->ear_indices[j]);
				if (spel_canvas_point_in_triangle(p, pa, pb, pc))
				{
					ear = false;
					break;
				}
			}

			if (!ear)
			{
				continue;
			}

			spel.gfx->canvas_ctx->indices[out++] =
				base + spel.gfx->canvas_ctx->ear_indices[a];
			spel.gfx->canvas_ctx->indices[out++] =
				base + spel.gfx->canvas_ctx->ear_indices[b];
			spel.gfx->canvas_ctx->indices[out++] =
				base + spel.gfx->canvas_ctx->ear_indices[c];

			for (int j = b; j < remaining - 1; j++)
			{
				spel.gfx->canvas_ctx->ear_indices[j] =
					spel.gfx->canvas_ctx->ear_indices[j + 1];
			}
			remaining--;
			found = true;
			break;
		}

		if (!found)
		{
			break;
		}
	}

	if (remaining == 3)
	{
		spel.gfx->canvas_ctx->indices[out++] =
			base + spel.gfx->canvas_ctx->ear_indices[0];
		spel.gfx->canvas_ctx->indices[out++] =
			base + spel.gfx->canvas_ctx->ear_indices[1];
		spel.gfx->canvas_ctx->indices[out++] =
			base + spel.gfx->canvas_ctx->ear_indices[2];
	}

	spel.gfx->canvas_ctx->vert_count += n;
	spel.gfx->canvas_ctx->index_count = out;
}

void spel_canvas_cap_round_connected(spel_path_point* p, float w, spel_color color,
									 bool start, int stripEndpointBase)
{
	const int SEGS = 8;
	spel_canvas_ensure_capacity(SEGS + 1, SEGS * 3);
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];

	// the two strip endpoint verts are already in the buffer
	// left vert = strip_endpoint_base + 0
	// right vert = strip_endpoint_base + 1
	// we just add the center and intermediate arc verts

	int left_vert = stripEndpointBase + 0;
	int right_vert = stripEndpointBase + 1;

	// direction for the cap arc
	float dir_x = start ? -p->direction.x : p->direction.x;
	float dir_y = start ? -p->direction.y : p->direction.y;

	float base_angle = atan2f(dir_y, dir_x);
	float a0 = base_angle - 3.14159f * 0.5f;
	float a1 = base_angle + 3.14159f * 0.5f;

	int center_idx = spel.gfx->canvas_ctx->vert_count;

	// center vert
	spel.gfx->canvas_ctx->verts[center_idx] =
		(spel_canvas_vertex){spel_mat3_transform_point(t, p->position), {0, 0}, color};
	spel.gfx->canvas_ctx->vert_count++;

	// intermediate arc verts (excluding the two endpoints which are strip verts)
	int arc_base = spel.gfx->canvas_ctx->vert_count;
	for (int i = 1; i < SEGS; i++)
	{
		float a = a0 + (a1 - a0) * (float)i / SEGS;
		spel.gfx->canvas_ctx->verts[spel.gfx->canvas_ctx->vert_count++] =
			(spel_canvas_vertex){
				spel_mat3_transform_point(t, spel_vec2(p->position.x + cosf(a) * w,
													   p->position.y + sinf(a) * w)),
				{0, 0},
				color};
	}

	// first triangle connects center to left strip vert and first arc vert
	uint32_t* idx = &spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count];
	idx[0] = center_idx;
	idx[1] = start ? left_vert : right_vert;
	idx[2] = arc_base;
	spel.gfx->canvas_ctx->index_count += 3;

	// middle triangles
	for (int i = 0; i < SEGS - 2; i++)
	{
		idx = &spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count];
		idx[0] = center_idx;
		idx[1] = arc_base + i;
		idx[2] = arc_base + i + 1;
		spel.gfx->canvas_ctx->index_count += 3;
	}

	// last triangle connects last arc vert to right strip vert
	idx = &spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count];
	idx[0] = center_idx;
	idx[1] = arc_base + SEGS - 2;
	idx[2] = start ? right_vert : left_vert;
	spel.gfx->canvas_ctx->index_count += 3;
}

void spel_canvas_join_round(spel_path_point* p0, spel_path_point* p1, float w,
							spel_color color)
{
	const int SEGS = 8;
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];
	spel_canvas_ensure_capacity(SEGS + 2, SEGS * 3);

	bool left_turn = (p1->flags & spel_point_left);

	float a0 = left_turn ? atan2f(p0->direction.x, -p0->direction.y)
						 : atan2f(-p0->direction.x, p0->direction.y);
	float a1 = left_turn ? atan2f(p1->direction.x, -p1->direction.y)
						 : atan2f(-p1->direction.x, p1->direction.y);

	// ensure correct sweep direction
	if (left_turn)
	{
		while (a1 > a0)
			a1 -= spel_tau;
		// clamp to at most Pi sweep
		if (a0 - a1 > spel_pi)
			a1 += spel_tau;
	}
	else
	{
		while (a1 < a0)
			a1 += spel_tau;
		// clamp to at most Pi sweep
		if (a1 - a0 > spel_pi)
			a1 -= spel_tau;
	}

	int base = spel.gfx->canvas_ctx->vert_count;

	// center at join point
	spel.gfx->canvas_ctx->verts[base] =
		(spel_canvas_vertex){spel_mat3_transform_point(t, p1->position), {0, 0}, color};
	spel.gfx->canvas_ctx->vert_count++;

	for (int i = 0; i <= SEGS; i++)
	{
		float a = a0 + ((a1 - a0) * (float)i / SEGS);
		spel.gfx->canvas_ctx->verts[spel.gfx->canvas_ctx->vert_count++] =
			(spel_canvas_vertex){
				spel_mat3_transform_point(t, spel_vec2(p1->position.x + cosf(a) * w,
													   p1->position.y + sinf(a) * w)),
				{0, 0},
				color};
	}

	for (int i = 0; i < SEGS; i++)
	{
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count++] = base;
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count++] = base + i + 1;
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count++] = base + i + 2;
	}
}

void spel_canvas_join_bevel_connected(spel_path_point* p0, spel_path_point* p1, float w,
									  spel_color color, int vbase)
{
	// incoming outer vert and outgoing outer vert are already in the buffer
	// just need one triangle connecting them through the center
	bool left_turn = (p1->flags & spel_point_left);

	// outer verts depend on which side the join is on
	int outer_in = left_turn ? vbase + 0 : vbase + 1;
	int outer_out = left_turn ? vbase + 2 : vbase + 3;

	// center vert
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];
	spel_canvas_ensure_capacity(1, 3);
	int center = spel.gfx->canvas_ctx->vert_count;
	spel.gfx->canvas_ctx->verts[center] =
		(spel_canvas_vertex){spel_mat3_transform_point(t, p1->position), {0, 0}, color};
	spel.gfx->canvas_ctx->vert_count++;

	uint32_t* idx = &spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count];
	idx[0] = center;
	idx[1] = outer_in;
	idx[2] = outer_out;
	spel.gfx->canvas_ctx->index_count += 3;
}

void spel_canvas_join_round_connected(spel_path_point* p0, spel_path_point* p1, float w,
									  spel_color color, int vbase)
{
	const int SEGS = 8;
	bool left_turn = (p1->flags & spel_point_left);

	int outer_in = left_turn ? vbase + 0 : vbase + 1;
	int outer_out = left_turn ? vbase + 2 : vbase + 3;

	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];
	spel_canvas_ensure_capacity(SEGS + 1, (SEGS + 2) * 3);

	int center = spel.gfx->canvas_ctx->vert_count++;
	spel.gfx->canvas_ctx->verts[center] =
		(spel_canvas_vertex){spel_mat3_transform_point(t, p1->position), {0, 0}, color};

	// arc angles
	float a0 = left_turn ? atan2f(p0->direction.x, -p0->direction.y)
						 : atan2f(-p0->direction.x, p0->direction.y);
	float a1 = left_turn ? atan2f(p1->direction.x, -p1->direction.y)
						 : atan2f(-p1->direction.x, p1->direction.y);

	if (left_turn)
	{
		while (a1 > a0)
			a1 -= spel_tau;
		if (a0 - a1 > spel_pi)
			a1 += spel_tau;
	}
	else
	{
		while (a1 < a0)
			a1 += spel_tau;
		if (a1 - a0 > spel_pi)
			a1 -= spel_tau;
	}

	// first triangle: center -> outer_in -> first intermediate vert
	// intermediate verts (excluding boundary verts which are already in buffer)
	int arc_start = spel.gfx->canvas_ctx->vert_count;

	for (int i = 1; i < SEGS; i++)
	{
		float a = a0 + (a1 - a0) * (float)i / SEGS;
		spel.gfx->canvas_ctx->verts[spel.gfx->canvas_ctx->vert_count++] =
			(spel_canvas_vertex){
				spel_mat3_transform_point(t, spel_vec2(p1->position.x + cosf(a) * w,
													   p1->position.y + sinf(a) * w)),
				{0, 0},
				color};
	}

	// stitch: center -> outer_in -> arc[0]
	uint32_t* idx = &spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count];
	idx[0] = center;
	idx[1] = outer_in;
	idx[2] = arc_start;
	spel.gfx->canvas_ctx->index_count += 3;

	// center -> arc[i] -> arc[i+1]
	for (int i = 0; i < SEGS - 2; i++)
	{
		idx = &spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count];
		idx[0] = center;
		idx[1] = arc_start + i;
		idx[2] = arc_start + i + 1;
		spel.gfx->canvas_ctx->index_count += 3;
	}

	// center -> arc[last] -> outer_out
	idx = &spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count];
	idx[0] = center;
	idx[1] = arc_start + SEGS - 2;
	idx[2] = outer_out;
	spel.gfx->canvas_ctx->index_count += 3;
}

static void canvas_push_vert(spel_mat3 t, float x, float y, spel_color color)
{
	spel.gfx->canvas_ctx->verts[spel.gfx->canvas_ctx->vert_count++] =
		(spel_canvas_vertex){
			spel_mat3_transform_point(t, spel_vec2(x, y)), {0, 0}, color};
}

// helper to push a quad between two vert pairs
static void canvas_push_quad(int v0, int v1, int v2, int v3)
{
	uint32_t* idx = &spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count];
	idx[0] = v0;
	idx[1] = v2;
	idx[2] = v1;
	idx[3] = v1;
	idx[4] = v2;
	idx[5] = v3;
	spel.gfx->canvas_ctx->index_count += 6;
}

void spel_canvas_stroke_basic(spel_canvas_path* path, float w, spel_color color,
							  int* outPointBases, bool* outIsDouble)
{
	int n = path->point_count;
	if (n < 2)
		return;

	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];

	// first pass - figure out vert counts and total capacity needed
	int total_verts = 0;
	int total_indices = 0;

	for (int i = 0; i < n; i++)
	{
		spel_path_point* p = &path->points[i];
		bool is_endpoint = !path->closed && (i == 0 || i == n - 1);

		// endpoints and miter corners get 1 pair, round/bevel get 2 pairs
		bool needs_double =
			!is_endpoint && (spel.gfx->canvas_ctx->join_type == SPEL_CANVAS_JOIN_ROUND ||
							 spel.gfx->canvas_ctx->join_type == SPEL_CANVAS_JOIN_BEVEL ||
							 (p->flags & spel_point_bevel));

		outIsDouble[i] = needs_double;
		outPointBases[i] = total_verts;
		total_verts += needs_double ? 4 : 2;
	}

	// each segment is one quad = 6 indices
	total_indices = (n - 1) * 6 + (path->closed ? 6 : 0);

	spel_canvas_ensure_capacity(total_verts, total_indices);
	int base = spel.gfx->canvas_ctx->vert_count;

	// second pass - push verts
	for (int i = 0; i < n; i++)
	{
		spel_path_point* p = &path->points[i];
		bool is_endpoint = !path->closed && (i == 0 || i == n - 1);

		if (is_endpoint)
		{
			// endpoint - use edge direction
			spel_path_point* d = (i == 0) ? &path->points[0] : &path->points[n - 2];
			float px = -d->direction.y * w;
			float py = d->direction.x * w;
			canvas_push_vert(t, p->position.x + px, p->position.y + py, color);
			canvas_push_vert(t, p->position.x - px, p->position.y - py, color);
		}
		else if (!outIsDouble[i])
		{
			// miter corner - single pair at miter intersection
			float px = -p->miter_direction.y * w;
			float py = p->miter_direction.x * w;
			canvas_push_vert(t, p->position.x + px, p->position.y + py, color);
			canvas_push_vert(t, p->position.x - px, p->position.y - py, color);
		}
		else
		{
			// round/bevel corner - two pairs
			spel_path_point* prev = &path->points[(i + n - 1) % n];

			// incoming pair - perpendicular to incoming direction
			float ipx = -prev->direction.y * w;
			float ipy = prev->direction.x * w;

			// outgoing pair - perpendicular to outgoing direction
			float opx = -p->direction.y * w;
			float opy = p->direction.x * w;

			canvas_push_vert(t, p->position.x + ipx, p->position.y + ipy, color);
			canvas_push_vert(t, p->position.x - ipx, p->position.y - ipy, color);
			canvas_push_vert(t, p->position.x + opx, p->position.y + opy, color);
			canvas_push_vert(t, p->position.x - opx, p->position.y - opy, color);
		}

		// adjust base-relative offsets
		outPointBases[i] += base;
	}

	// square cap - offset endpoint verts
	if (!path->closed && spel.gfx->canvas_ctx->cap_type == SPEL_CANVAS_CAP_SQUARE)
	{
		spel_path_point* p0 = &path->points[0];
		spel_path_point* pn = &path->points[n - 2];

		// start - push back
		spel.gfx->canvas_ctx->verts[base + 0].position.x -= p0->direction.x * w;
		spel.gfx->canvas_ctx->verts[base + 0].position.y -= p0->direction.y * w;
		spel.gfx->canvas_ctx->verts[base + 1].position.x -= p0->direction.x * w;
		spel.gfx->canvas_ctx->verts[base + 1].position.y -= p0->direction.y * w;

		// end - push forward
		int end_base = outPointBases[n - 1];
		spel.gfx->canvas_ctx->verts[end_base + 0].position.x += pn->direction.x * w;
		spel.gfx->canvas_ctx->verts[end_base + 0].position.y += pn->direction.y * w;
		spel.gfx->canvas_ctx->verts[end_base + 1].position.x += pn->direction.x * w;
		spel.gfx->canvas_ctx->verts[end_base + 1].position.y += pn->direction.y * w;
	}

	// third pass - stitch quads between consecutive points
	for (int i = 0; i < n - 1; i++)
	{
		// end verts of point i - if double, use the outgoing pair (offset +2)
		int seg_end = outPointBases[i] + (outIsDouble[i] ? 2 : 0);
		// start verts of point i+1 - always the first pair
		int seg_start = outPointBases[i + 1];

		canvas_push_quad(seg_end, seg_end + 1, seg_start, seg_start + 1);
	}

	if (path->closed)
	{
		int seg_end = outPointBases[n - 1] + (outIsDouble[n - 1] ? 2 : 0);
		int seg_start = outPointBases[0];
		canvas_push_quad(seg_end, seg_end + 1, seg_start, seg_start + 1);
	}
}

void spel_canvas_join_bevel(spel_path_point* p0, spel_path_point* p1, float w,
							spel_color color)
{
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];
	spel_canvas_ensure_capacity(3, 3);
	int base = spel.gfx->canvas_ctx->vert_count;

	// which side needs the bevel triangle depends on turn direction
	bool left_turn = (p1->flags & spel_point_left);

	spel_vec2 center = p1->position;

	// two outer points from each edge direction
	spel_vec2 e0;
	spel_vec2 e1;
	if (left_turn)
	{
		e0 = spel_vec2(center.x + p0->direction.y * w, center.y - p0->direction.x * w);
		e1 = spel_vec2(center.x + p1->direction.y * w, center.y - p1->direction.x * w);
	}
	else
	{
		e0 = spel_vec2(center.x - p0->direction.y * w, center.y + p0->direction.x * w);
		e1 = spel_vec2(center.x - p1->direction.y * w, center.y + p1->direction.x * w);
	}

	spel.gfx->canvas_ctx->verts[base + 0] =
		(spel_canvas_vertex){spel_mat3_transform_point(t, center), {0, 0}, color};
	spel.gfx->canvas_ctx->verts[base + 1] =
		(spel_canvas_vertex){spel_mat3_transform_point(t, e0), {0, 0}, color};
	spel.gfx->canvas_ctx->verts[base + 2] =
		(spel_canvas_vertex){spel_mat3_transform_point(t, e1), {0, 0}, color};

	spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count + 0] = base;
	spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count + 1] = base + 1;
	spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count + 2] = base + 2;

	spel.gfx->canvas_ctx->vert_count += 3;
	spel.gfx->canvas_ctx->index_count += 3;
}

void spel_canvas_join_miter(spel_path_point* p0, spel_path_point* p1, float w,
							float miterLimit, spel_color color)
{
	spel_canvas_context* ctx = spel.gfx->canvas_ctx;
	spel_mat3 t = ctx->transforms[ctx->transform_top];

	// edge directions
	float dx0 = p0->direction.x;
	float dy0 = p0->direction.y;
	float dx1 = p1->direction.x;
	float dy1 = p1->direction.y;

	// outward normals depending on turn side
	bool left_turn = (p1->flags & spel_point_left);
	spel_vec2 n0 =
		left_turn ? spel_vec2(-dy0, dx0) : spel_vec2(dy0, -dx0); // prev edge normal
	spel_vec2 n1 =
		left_turn ? spel_vec2(-dy1, dx1) : spel_vec2(dy1, -dx1); // next edge normal

	// offset points on each edge
	float ox0 = p1->position.x + n0.x * w;
	float oy0 = p1->position.y + n0.y * w;
	float ox1 = p1->position.x + n1.x * w;
	float oy1 = p1->position.y + n1.y * w;

	// line intersection of offset edges
	float denom = (dx0 * dy1) - (dy0 * dx1);
	bool bevel = false;
	float mx = 0.0F;
	float my = 0.0F;

	if (fabsf(denom) > spel_epsilonf)
	{
		float t0 = ((ox1 - ox0) * dy1 - (oy1 - oy0) * dx1) / denom;
		mx = ox0 + dx0 * t0;
		my = oy0 + dy0 * t0;
		float mdx = mx - p1->position.x;
		float mdy = my - p1->position.y;
		float mlen2 = (mdx * mdx) + (mdy * mdy);
		if (mlen2 > (miterLimit * miterLimit * w * w))
		{
			bevel = true;
		}
	}
	else
	{
		// nearly parallel - bevel
		bevel = true;
	}

	if (bevel)
	{
		spel_canvas_join_bevel(p0, p1, w, color);
		return;
	}

	// emit wedge triangle: prev offset -> miter tip -> next offset
	spel_canvas_ensure_capacity(3, 3);
	int base = ctx->vert_count;
	ctx->verts[base + 0] = (spel_canvas_vertex){
		spel_mat3_transform_point(t, spel_vec2(ox0, oy0)), {0, 0}, color};
	ctx->verts[base + 1] = (spel_canvas_vertex){
		spel_mat3_transform_point(t, spel_vec2(mx, my)), {0, 0}, color};
	ctx->verts[base + 2] = (spel_canvas_vertex){
		spel_mat3_transform_point(t, spel_vec2(ox1, oy1)), {0, 0}, color};

	ctx->indices[ctx->index_count + 0] = base + 0;
	ctx->indices[ctx->index_count + 1] = base + 1;
	ctx->indices[ctx->index_count + 2] = base + 2;

	ctx->vert_count += 3;
	ctx->index_count += 3;
}

void spel_canvas_stroke_push_cap_verts(spel_path_point* p, float w, float ox, float oy,
									   spel_color color)
{
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];
	int base = spel.gfx->canvas_ctx->vert_count;

	// left and right of stroke at this point
	// perpendicular to direction is (-dy, dx)
	spel.gfx->canvas_ctx->verts[base + 0] = (spel_canvas_vertex){
		spel_mat3_transform_point(
			t, spel_vec2(p->position.x - p->direction.y * w + p->direction.x * ox,
						 p->position.y + p->direction.x * w + p->direction.y * ox)),
		{0, 0},
		color};
	spel.gfx->canvas_ctx->verts[base + 1] = (spel_canvas_vertex){
		spel_mat3_transform_point(
			t, spel_vec2(p->position.x + p->direction.y * w + p->direction.x * ox,
						 p->position.y - p->direction.x * w + p->direction.y * ox)),
		{0, 0},
		color};

	spel.gfx->canvas_ctx->vert_count += 2;
}

void spel_canvas_cap_square(spel_path_point* p, float w, spel_color color, bool start)
{
	float dir = start ? -1.0F : 1.0F;
	spel_canvas_ensure_capacity(2, 0);
	spel_canvas_stroke_push_cap_verts(p, w, dir * w, 0, color);
}

void spel_canvas_cap_round(spel_path_point* p, float w, spel_color color, bool start)
{
	const int SEGS = 8;
	spel_canvas_ensure_capacity(SEGS + 2, SEGS * 3);
	spel_mat3 t = spel.gfx->canvas_ctx->transforms[spel.gfx->canvas_ctx->transform_top];

	float dir = start ? 1.0f : -1.0f;
	float base_angle = start ? atan2f(p->direction.y, p->direction.x) + spel_pi * 0.5F
							 : atan2f(-p->direction.y, -p->direction.x) + spel_pi * 0.5F;

	int base = spel.gfx->canvas_ctx->vert_count;

	// center
	spel.gfx->canvas_ctx->verts[base] =
		(spel_canvas_vertex){spel_mat3_transform_point(t, p->position), {0, 0}, color};
	spel.gfx->canvas_ctx->vert_count++;

	for (int i = 0; i <= SEGS; i++)
	{
		float a = base_angle + (dir * (float)i / SEGS * spel_pi);
		spel.gfx->canvas_ctx->verts[spel.gfx->canvas_ctx->vert_count++] =
			(spel_canvas_vertex){
				spel_mat3_transform_point(t, spel_vec2(p->position.x + cosf(a) * w,
													   p->position.y + sinf(a) * w)),
				{0, 0},
				color};
	}

	for (int i = 0; i < SEGS; i++)
	{
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count++] = base;
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count++] = base + i + 1;
		spel.gfx->canvas_ctx->indices[spel.gfx->canvas_ctx->index_count++] = base + i + 2;
	}
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
	for (size_t i = 0; i < path->point_count; i++)
	{
		spel_path_point* p0 = &path->points[i];

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
	}
	return true;
}

spel_path_point* spel_canvas_path_point_get(uint32_t index)
{
	spel_canvas_path* path = &spel.gfx->canvas_ctx->current_path;
	return &path->points[index];
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

	for (size_t i = 0; i < path->point_count; i++)
	{
		spel_path_point* p = &path->points[i];

		spel_debug("  [%d] (%.2f, %.2f) dx(%.2f, %.2f) len(%.2f) flags(%d)", i,
				   p->position.x, p->position.y, p->direction.x, p->direction.y, p->len,
				   p->flags);
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

	if (spel.gfx->canvas_ctx->current_path.point_count + 1 >
		spel.gfx->canvas_ctx->current_path.point_capacity)
	{
		spel.gfx->canvas_ctx->current_path.point_capacity *= 2;

		spel.gfx->canvas_ctx->current_path.points = spel_memory_realloc(
			spel.gfx->canvas_ctx->current_path.points,
			spel.gfx->canvas_ctx->current_path.point_capacity * sizeof(spel_path_point),
			SPEL_MEM_TAG_GFX);
	}

	spel.gfx->canvas_ctx->current_path.point_count++;

	spel_path_point* point =
		&spel.gfx->canvas_ctx->current_path
			 .points[spel.gfx->canvas_ctx->current_path.point_count - 1];

	point->position = position;
	point->flags = flags;
	point->direction = spel_vec2_zero;
	point->miter_direction = spel_vec2_zero;
	point->len = 0;
}
