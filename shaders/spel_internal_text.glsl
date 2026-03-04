#pragma shader_stage(fragment)
#version 450

#define MODE_SDF       0
#define MODE_MSDF      1
#define MODE_MTSDF     2
#define MODE_BITMAP    3

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

layout(set = 0, binding = 0) uniform FrameUBO {
    mat4 u_projection;
};

layout(set = 0, binding = 1) uniform DrawUBO {
    int   u_mode;

    float u_sdf_smoothing;
    float u_sdf_threshold;

    float _pad[2];
};

layout(set = 1, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) out vec4 frag_color;

float sdf_alpha(float dist) {
    return smoothstep(u_sdf_threshold - u_sdf_smoothing,
                      u_sdf_threshold + u_sdf_smoothing,
                      dist);
}

float msdf_median(vec3 msd) {
    return max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));
}

void main() {
    switch (u_mode) {
        case MODE_SDF: {
            float dist  = texture(u_atlas, v_uv).r;
            float alpha = sdf_alpha(dist);
            frag_color  = vec4(v_color.rgb, v_color.a * alpha);
            break;
        }

        case MODE_MSDF: {
            vec3  msd   = texture(u_atlas, v_uv).rgb;
            float dist  = msdf_median(msd);
            float alpha = sdf_alpha(dist);
            frag_color  = vec4(v_color.rgb, v_color.a * alpha);
            break;
        }

        case MODE_MTSDF: {
            vec4  s        = texture(u_atlas, v_uv);
            float msdf_d   = msdf_median(s.rgb);
            float sdf_d    = s.a;
            float fw       = length(vec2(dFdx(v_uv.x), dFdy(v_uv.x)));
            float blend    = clamp(fw * 8.0, 0.0, 1.0);
            float dist     = mix(msdf_d, sdf_d, blend);

            float alpha    = sdf_alpha(dist);
            frag_color     = vec4(v_color.rgb, v_color.a * alpha);
            break;
        }
        case MODE_BITMAP: {
            vec4 tex = texture(u_atlas, v_uv);
            float mask = (tex.r == tex.g && tex.g == tex.b)
                         ? tex.r
                         : tex.a;

            frag_color = vec4(v_color.rgb, v_color.a * mask);
            break;
        }

        default: {
            frag_color = vec4(1.0, 0.0, 1.0, 1.0);
            break;
        }
    }
}
