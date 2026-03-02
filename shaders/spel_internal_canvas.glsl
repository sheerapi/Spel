#pragma shader_stage(vertex)
#version 450

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;
layout(location = 2) out vec2 v_pos; // world position for gradient math

layout(set = 0, binding = 0) uniform FrameData
{
	mat4 proj;
};

void main()
{
	v_uv = in_uv;
	v_color = in_color;
	v_pos = in_pos; // pass through before projection
	gl_Position = proj * vec4(in_pos, 0.0, 1.0);
}

#pragma shader_stage(fragment)
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 2) in vec2 v_pos;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler2D u_texture;

layout(set = 1, binding = 0) uniform PaintData
{
	int paint_type;
	float _pad0, _pad1, _pad2; // std140 alignment
	vec4 inner_color;
	vec4 outer_color;
	vec2 paint_start;
	vec2 paint_end;
	float radius_inner;
	float radius_outer;
	float _pad3, _pad4;
};

vec4 eval_paint()
{
	if (paint_type == 0) // solid
		return v_color;

	if (paint_type == 1) // linear gradient
	{
		vec2 line = paint_end - paint_start;
		float len = dot(line, line);
		float t = len > 0.0 ? clamp(dot(v_pos - paint_start, line) / len, 0.0, 1.0) : 0.0;
		return mix(inner_color, outer_color, t);
	}

	if (paint_type == 2) // radial gradient
	{
		float d = length(v_pos - paint_start);
		float t = clamp((d - radius_inner) / (radius_outer - radius_inner), 0.0, 1.0);
		return mix(inner_color, outer_color, t);
	}

	if (paint_type == 3) // image
	{
		return texture(u_texture, v_uv) * v_color;
	}

	return v_color;
}

void main()
{
	out_color = eval_paint();
}