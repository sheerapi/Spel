#pragma shader_stage(vertex)
#version 450

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color; // normalized u8 → vec4

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;
layout(location = 2) out vec2 v_pos;

layout(set = 0, binding = 0) uniform FrameData
{
	mat4 proj;
};

void main()
{
	v_uv = in_uv;
	v_color = in_color;
	v_pos = in_pos;
	gl_Position = proj * vec4(in_pos, 0.0, 1.0);
}

#pragma shader_stage(fragment)
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler2D u_texture;

void main()
{
	vec4 tex = texture(u_texture, v_uv);
	out_color = tex * v_color;
}
