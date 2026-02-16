#pragma shader_stage(fragment)
#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 out_color;
layout(set = 0, binding = 0) uniform sampler2D u_screen;

void main() {
    out_color = texture(u_screen, v_uv);
}