#pragma shader_stage(vertex)
#version 450

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;

layout(set = 0, binding = 0) uniform ProjData
{
	vec2 scale;
	vec2 translate;
};

void main()
{
	v_uv = a_uv;
	v_color = a_color;
	gl_Position = vec4(a_pos * scale + translate, 0.0, 1.0);
}

#pragma shader_stage(fragment)
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D u_texture;

void main()
{
	out_color = v_color * texture(u_texture, v_uv);
}