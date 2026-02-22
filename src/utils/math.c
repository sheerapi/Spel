#include "utils/math.h"
#include <math.h>
#include <string.h>

// ============================================================
//  COLOR PRESETS
// ============================================================

spel_color spel_color_black = {0, 0, 0, 255};
spel_color spel_color_white = {255, 255, 255, 255};
spel_color spel_color_red = {255, 0, 0, 255};
spel_color spel_color_green = {0, 255, 0, 255};
spel_color spel_color_blue = {0, 0, 255, 255};
spel_color spel_color_cyan = {0, 255, 255, 255};
spel_color spel_color_magenta = {255, 0, 255, 255};
spel_color spel_color_yellow = {255, 255, 0, 255};
spel_color spel_color_orange = {255, 165, 0, 255};
spel_color spel_color_purple = {128, 0, 128, 255};
spel_color spel_color_pink = {255, 192, 203, 255};
spel_color spel_color_brown = {139, 69, 19, 255};
spel_color spel_color_gray = {128, 128, 128, 255};
spel_color spel_color_dark_gray = {64, 64, 64, 255};
spel_color spel_color_light_gray = {192, 192, 192, 255};
spel_color spel_color_transparent = {0, 0, 0, 0};

// ============================================================
//  SCALAR MATH
// ============================================================

int spel_math_isnan(double x)
{
	return x != x;
}
int spel_math_isinf(double x)
{
	return x == (1.0 / 0.0) || x == -(1.0 / 0.0);
}
int spel_math_isfinite(double x)
{
	return !spel_math_isnan(x) && !spel_math_isinf(x);
}

float spel_math_sqrt(float x)
{
	return sqrtf(x);
}
float spel_math_rsqrt(float x)
{
	return 1.0F / sqrtf(x);
}
float spel_math_pow(float base, float e)
{
	return powf(base, e);
}
float spel_math_log(float x)
{
	return logf(x);
}
float spel_math_log2(float x)
{
	return log2f(x);
}
float spel_math_exp(float x)
{
	return expf(x);
}

float spel_math_sin(float x)
{
	return sinf(x);
}
float spel_math_cos(float x)
{
	return cosf(x);
}
float spel_math_tan(float x)
{
	return tanf(x);
}
float spel_math_asin(float x)
{
	return asinf(x);
}
float spel_math_acos(float x)
{
	return acosf(x);
}
float spel_math_atan(float x)
{
	return atanf(x);
}
float spel_math_atan2(float y, float x)
{
	return atan2f(y, x);
}

float spel_math_floor(float x)
{
	return floorf(x);
}
float spel_math_ceil(float x)
{
	return ceilf(x);
}
float spel_math_round(float x)
{
	return roundf(x);
}
float spel_math_fmod(float x, float y)
{
	return fmodf(x, y);
}

float spel_math_wrap(float x, float min, float max)
{
	float range = max - min;
	if (range <= 0.0F)
	{
		return min;
	}
	return x - (range * floorf((x - min) / range));
}

float spel_math_snap(float x, float step)
{
	if (step <= 0.0F)
	{
		return x;
	}
	return roundf(x / step) * step;
}

float spel_math_lerp_clamped(float a, float b, float t)
{
	t = t < 0.0F ? 0.0F : t > 1.0F ? 1.0F : t;
	return a + ((b - a) * t);
}
float spel_math_inverse_lerp(float a, float b, float v)
{
	return (b - a) < spel_epsilon ? 0.0F : (v - a) / (b - a);
}
float spel_math_remap(float v, float a0, float b0, float a1, float b1)
{
	float t = spel_math_inverse_lerp(a0, b0, v);
	return spel_math_lerp(a1, b1, t);
}
float spel_math_smootherstep(float e0, float e1, float x)
{
	float t = (x - e0) / (e1 - e0);
	t = t < 0.0F ? 0.0F : t > 1.0F ? 1.0F : t;
	return t * t * t * (t * (t * 6.0F - 15.0F) + 10.0F);
}

float spel_math_angle_wrap(float r)
{
	return spel_math_wrap(r, -spel_pi, spel_pi);
}
float spel_math_angle_diff(float a, float b)
{
	return spel_math_angle_wrap(b - a);
}
float spel_math_angle_lerp(float a, float b, float t)
{
	return a + (spel_math_angle_diff(a, b) * t);
}

int spel_math_next_pow2(int x)
{
	if (x <= 0)
	{
		return 1;
	}
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}
int spel_math_is_pow2(int x)
{
	return x > 0 && (x & (x - 1)) == 0;
}
int spel_math_gcd(int a, int b)
{
	while (b)
	{
		int t = b;
		b = a % b;
		a = t;
	}
	return a;
}
int spel_math_lcm(int a, int b)
{
	return a / spel_math_gcd(a, b) * b;
}

float spel_math_approach(float current, float target, float step)
{
	float d = target - current;
	if (d > step)
	{
		return current + step;
	}
	if (d < -step)
	{
		return current - step;
	}
	return target;
}

// ============================================================
//  VEC2
// ============================================================

spel_vec2 spel_vec2_mul(spel_vec2 a, spel_vec2 b)
{
	return spel_vec2(a.x * b.x, a.y * b.y);
}
spel_vec2 spel_vec2_div(spel_vec2 a, spel_vec2 b)
{
	return spel_vec2(a.x / b.x, a.y / b.y);
}
spel_vec2 spel_vec2_neg(spel_vec2 v)
{
	return spel_vec2(-v.x, -v.y);
}

float spel_vec2_cross(spel_vec2 a, spel_vec2 b)
{
	return (a.x * b.y) - (a.y * b.x);
}
float spel_vec2_dist_sq(spel_vec2 a, spel_vec2 b)
{
	return spel_vec2_len_sq(spel_vec2_sub(a, b));
}
float spel_vec2_dist(spel_vec2 a, spel_vec2 b)
{
	return sqrtf(spel_vec2_dist_sq(a, b));
}
float spel_vec2_angle(spel_vec2 v)
{
	return atan2f(v.y, v.x);
}
float spel_vec2_angle_between(spel_vec2 a, spel_vec2 b)
{
	float d = spel_vec2_dot(a, b);
	float la = spel_vec2_len(a);
	float lb = spel_vec2_len(b);
	if (la < spel_epsilon || lb < spel_epsilon)
	{
		return 0.0F;
	}
	float cos_a = d / (la * lb);
	cos_a = cos_a < -1.0F ? -1.0F : cos_a > 1.0F ? 1.0F : cos_a;
	return acosf(cos_a);
}

spel_vec2 spel_vec2_normalize_safe(spel_vec2 v, spel_vec2 fallback)
{
	float l = spel_vec2_len(v);
	return l > spel_epsilon ? spel_vec2_scale(v, 1.0F / l) : fallback;
}
spel_vec2 spel_vec2_lerp(spel_vec2 a, spel_vec2 b, float t)
{
	return spel_vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}
spel_vec2 spel_vec2_clamp_len(spel_vec2 v, float maxLen)
{
	float l = spel_vec2_len(v);
	return (l > maxLen && l > spel_epsilon) ? spel_vec2_scale(v, maxLen / l) : v;
}
spel_vec2 spel_vec2_clamp(spel_vec2 v, spel_vec2 lo, spel_vec2 hi)
{
	return spel_vec2(v.x < lo.x	  ? lo.x
					 : v.x > hi.x ? hi.x
								  : v.x,
					 v.y < lo.y	  ? lo.y
					 : v.y > hi.y ? hi.y
								  : v.y);
}
spel_vec2 spel_vec2_min(spel_vec2 a, spel_vec2 b)
{
	return spel_vec2(fminf(a.x, b.x), fminf(a.y, b.y));
}
spel_vec2 spel_vec2_max(spel_vec2 a, spel_vec2 b)
{
	return spel_vec2(fmaxf(a.x, b.x), fmaxf(a.y, b.y));
}
spel_vec2 spel_vec2_abs(spel_vec2 v)
{
	return spel_vec2(fabsf(v.x), fabsf(v.y));
}
spel_vec2 spel_vec2_floor(spel_vec2 v)
{
	return spel_vec2(floorf(v.x), floorf(v.y));
}
spel_vec2 spel_vec2_ceil(spel_vec2 v)
{
	return spel_vec2(ceilf(v.x), ceilf(v.y));
}
spel_vec2 spel_vec2_round(spel_vec2 v)
{
	return spel_vec2(roundf(v.x), roundf(v.y));
}
spel_vec2 spel_vec2_perpendicular(spel_vec2 v)
{
	return spel_vec2(-v.y, v.x);
}
spel_vec2 spel_vec2_reflect(spel_vec2 v, spel_vec2 n)
{
	float d = 2.0F * spel_vec2_dot(v, n);
	return spel_vec2(v.x - d * n.x, v.y - d * n.y);
}
spel_vec2 spel_vec2_project(spel_vec2 v, spel_vec2 onto)
{
	float d = spel_vec2_dot(onto, onto);
	if (d < spel_epsilon)
	{
		return spel_vec2_zero;
	}
	return spel_vec2_scale(onto, spel_vec2_dot(v, onto) / d);
}
spel_vec2 spel_vec2_rotate(spel_vec2 v, float r)
{
	float c = cosf(r);
	float s = sinf(r);
	return spel_vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}
spel_vec2 spel_vec2_from_angle(float r)
{
	return spel_vec2(cosf(r), sinf(r));
}
spel_vec2 spel_vec2_approach(spel_vec2 current, spel_vec2 target, float step)
{
	spel_vec2 d = spel_vec2_sub(target, current);
	float dist = spel_vec2_len(d);
	if (dist <= step)
	{
		return target;
	}
	return spel_vec2_add(current, spel_vec2_scale(d, step / dist));
}

int spel_vec2_eq(spel_vec2 a, spel_vec2 b)
{
	return a.x == b.x && a.y == b.y;
}
int spel_vec2_nearly_eq(spel_vec2 a, spel_vec2 b, float eps)
{
	return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps;
}

// ============================================================
//  VEC3
// ============================================================

spel_vec3 spel_vec3_mul(spel_vec3 a, spel_vec3 b)
{
	return spel_vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}
spel_vec3 spel_vec3_div(spel_vec3 a, spel_vec3 b)
{
	return spel_vec3(a.x / b.x, a.y / b.y, a.z / b.z);
}
spel_vec3 spel_vec3_neg(spel_vec3 v)
{
	return spel_vec3(-v.x, -v.y, -v.z);
}

float spel_vec3_dist_sq(spel_vec3 a, spel_vec3 b)
{
	return spel_vec3_len_sq(spel_vec3_sub(a, b));
}
float spel_vec3_dist(spel_vec3 a, spel_vec3 b)
{
	return sqrtf(spel_vec3_dist_sq(a, b));
}
float spel_vec3_angle_between(spel_vec3 a, spel_vec3 b)
{
	float la = spel_vec3_len(a);
	float lb = spel_vec3_len(b);
	if (la < spel_epsilon || lb < spel_epsilon)
	{
		return 0.0F;
	}
	float c = spel_vec3_dot(a, b) / (la * lb);
	c = c < -1.0F ? -1.0F : c > 1.0F ? 1.0F : c;
	return acosf(c);
}

spel_vec3 spel_vec3_normalize_safe(spel_vec3 v, spel_vec3 fallback)
{
	float l = spel_vec3_len(v);
	return l > spel_epsilon ? spel_vec3_scale(v, 1.0F / l) : fallback;
}
spel_vec3 spel_vec3_lerp(spel_vec3 a, spel_vec3 b, float t)
{
	return spel_vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}
spel_vec3 spel_vec3_slerp(spel_vec3 a, spel_vec3 b, float t)
{
	float omega = spel_vec3_angle_between(a, b);
	if (omega < spel_epsilon)
	{
		return spel_vec3_lerp(a, b, t);
	}
	float s = sinf(omega);
	spel_vec3 r0 = spel_vec3_scale(a, sinf((1.0F - t) * omega) / s);
	spel_vec3 r1 = spel_vec3_scale(b, sinf(t * omega) / s);
	return spel_vec3_add(r0, r1);
}
spel_vec3 spel_vec3_clamp_len(spel_vec3 v, float maxLen)
{
	float l = spel_vec3_len(v);
	return (l > maxLen && l > spel_epsilon) ? spel_vec3_scale(v, maxLen / l) : v;
}
spel_vec3 spel_vec3_clamp(spel_vec3 v, spel_vec3 lo, spel_vec3 hi)
{
	return spel_vec3(v.x < lo.x	  ? lo.x
					 : v.x > hi.x ? hi.x
								  : v.x,
					 v.y < lo.y	  ? lo.y
					 : v.y > hi.y ? hi.y
								  : v.y,
					 v.z < lo.z	  ? lo.z
					 : v.z > hi.z ? hi.z
								  : v.z);
}
spel_vec3 spel_vec3_min(spel_vec3 a, spel_vec3 b)
{
	return spel_vec3(fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z));
}
spel_vec3 spel_vec3_max(spel_vec3 a, spel_vec3 b)
{
	return spel_vec3(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z));
}
spel_vec3 spel_vec3_abs(spel_vec3 v)
{
	return spel_vec3(fabsf(v.x), fabsf(v.y), fabsf(v.z));
}
spel_vec3 spel_vec3_floor(spel_vec3 v)
{
	return spel_vec3(floorf(v.x), floorf(v.y), floorf(v.z));
}
spel_vec3 spel_vec3_ceil(spel_vec3 v)
{
	return spel_vec3(ceilf(v.x), ceilf(v.y), ceilf(v.z));
}
spel_vec3 spel_vec3_round(spel_vec3 v)
{
	return spel_vec3(roundf(v.x), roundf(v.y), roundf(v.z));
}
spel_vec3 spel_vec3_reflect(spel_vec3 v, spel_vec3 n)
{
	float d = 2.0F * spel_vec3_dot(v, n);
	return spel_vec3_sub(v, spel_vec3_scale(n, d));
}
spel_vec3 spel_vec3_refract(spel_vec3 v, spel_vec3 n, float eta)
{
	float dot = spel_vec3_dot(v, n);
	float k = 1.0F - (eta * eta * (1.0F - dot * dot));
	if (k < 0.0F)
	{
		return spel_vec3_zero;
	}
	return spel_vec3_sub(spel_vec3_scale(v, eta),
						 spel_vec3_scale(n, (eta * dot) + sqrtf(k)));
}
spel_vec3 spel_vec3_project(spel_vec3 v, spel_vec3 onto)
{
	float d = spel_vec3_dot(onto, onto);
	if (d < spel_epsilon)
	{
		return spel_vec3_zero;
	}
	return spel_vec3_scale(onto, spel_vec3_dot(v, onto) / d);
}
spel_vec3 spel_vec3_project_plane(spel_vec3 v, spel_vec3 normal)
{
	return spel_vec3_sub(v, spel_vec3_project(v, normal));
}
spel_vec3 spel_vec3_approach(spel_vec3 current, spel_vec3 target, float step)
{
	spel_vec3 d = spel_vec3_sub(target, current);
	float dist = spel_vec3_len(d);
	if (dist <= step)
	{
		return target;
	}
	return spel_vec3_add(current, spel_vec3_scale(d, step / dist));
}

spel_vec2 spel_vec3_xy(spel_vec3 v)
{
	return spel_vec2(v.x, v.y);
}
spel_vec2 spel_vec3_xz(spel_vec3 v)
{
	return spel_vec2(v.x, v.z);
}

int spel_vec3_eq(spel_vec3 a, spel_vec3 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}
int spel_vec3_nearly_eq(spel_vec3 a, spel_vec3 b, float eps)
{
	return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps && fabsf(a.z - b.z) <= eps;
}

// ============================================================
//  VEC4
// ============================================================

spel_vec4 spel_vec4_add(spel_vec4 a, spel_vec4 b)
{
	return spel_vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}
spel_vec4 spel_vec4_sub(spel_vec4 a, spel_vec4 b)
{
	return spel_vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}
spel_vec4 spel_vec4_mul(spel_vec4 a, spel_vec4 b)
{
	return spel_vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}
spel_vec4 spel_vec4_div(spel_vec4 a, spel_vec4 b)
{
	return spel_vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}
spel_vec4 spel_vec4_scale(spel_vec4 v, float s)
{
	return spel_vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}
spel_vec4 spel_vec4_neg(spel_vec4 v)
{
	return spel_vec4(-v.x, -v.y, -v.z, -v.w);
}
float spel_vec4_dot(spel_vec4 a, spel_vec4 b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}
float spel_vec4_len_sq(spel_vec4 v)
{
	return spel_vec4_dot(v, v);
}
float spel_vec4_len(spel_vec4 v)
{
	return sqrtf(spel_vec4_len_sq(v));
}
spel_vec4 spel_vec4_normalize(spel_vec4 v)
{
	float l = spel_vec4_len(v);
	return l > spel_epsilon ? spel_vec4_scale(v, 1.0F / l) : spel_vec4_zero;
}
spel_vec4 spel_vec4_lerp(spel_vec4 a, spel_vec4 b, float t)
{
	return spel_vec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t,
					 a.w + (b.w - a.w) * t);
}
spel_vec4 spel_vec4_min(spel_vec4 a, spel_vec4 b)
{
	return spel_vec4(fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z), fminf(a.w, b.w));
}
spel_vec4 spel_vec4_max(spel_vec4 a, spel_vec4 b)
{
	return spel_vec4(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z), fmaxf(a.w, b.w));
}
spel_vec4 spel_vec4_abs(spel_vec4 v)
{
	return spel_vec4(fabsf(v.x), fabsf(v.y), fabsf(v.z), fabsf(v.w));
}
spel_vec3 spel_vec4_xyz(spel_vec4 v)
{
	return spel_vec3(v.x, v.y, v.z);
}
spel_vec2 spel_vec4_xy(spel_vec4 v)
{
	return spel_vec2(v.x, v.y);
}
int spel_vec4_eq(spel_vec4 a, spel_vec4 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
int spel_vec4_nearly_eq(spel_vec4 a, spel_vec4 b, float eps)
{
	return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps &&
		   fabsf(a.z - b.z) <= eps && fabsf(a.w - b.w) <= eps;
}

// ============================================================
//  MAT4
// ============================================================

// Element access: column-major  m[col * 4 + row]
#define M4(mt, r, c) ((mt).m[(c) * 4 + (r)])

spel_mat4 spel_mat4_identity(void)
{
	spel_mat4 r;
	memset(&r, 0, sizeof(r));
	r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0F;
	return r;
}
spel_mat4 spel_mat4_zero(void)
{
	spel_mat4 r;
	memset(&r, 0, sizeof(r));
	return r;
}

spel_mat4 spel_mat4_mul(spel_mat4 a, spel_mat4 b)
{
	spel_mat4 r;
	for (int col = 0; col < 4; col++)
	{
		for (int row = 0; row < 4; row++)
		{
			float s = 0.0F;
			for (int k = 0; k < 4; k++)
			{
				s += M4(a, row, k) * M4(b, k, col);
			}
			M4(r, row, col) = s;
		}
	}
	return r;
}
spel_vec4 spel_mat4_mul_vec4(spel_mat4 m, spel_vec4 v)
{
	return spel_vec4(
		M4(m, 0, 0) * v.x + M4(m, 0, 1) * v.y + M4(m, 0, 2) * v.z + M4(m, 0, 3) * v.w,
		M4(m, 1, 0) * v.x + M4(m, 1, 1) * v.y + M4(m, 1, 2) * v.z + M4(m, 1, 3) * v.w,
		M4(m, 2, 0) * v.x + M4(m, 2, 1) * v.y + M4(m, 2, 2) * v.z + M4(m, 2, 3) * v.w,
		M4(m, 3, 0) * v.x + M4(m, 3, 1) * v.y + M4(m, 3, 2) * v.z + M4(m, 3, 3) * v.w);
}
spel_vec3 spel_mat4_mul_vec3(spel_mat4 m, spel_vec3 v, float w)
{
	spel_vec4 r = spel_mat4_mul_vec4(m, spel_vec4(v.x, v.y, v.z, w));
	return spel_vec3(r.x, r.y, r.z);
}

spel_mat4 spel_mat4_transpose(spel_mat4 m)
{
	spel_mat4 r;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
			M4(r, i, j) = M4(m, j, i);
	}
	return r;
}

float spel_mat4_determinant(spel_mat4 m)
{
	float A2323 = M4(m, 2, 2) * M4(m, 3, 3) - M4(m, 2, 3) * M4(m, 3, 2);
	float A1323 = M4(m, 2, 1) * M4(m, 3, 3) - M4(m, 2, 3) * M4(m, 3, 1);
	float A1223 = M4(m, 2, 1) * M4(m, 3, 2) - M4(m, 2, 2) * M4(m, 3, 1);
	float A0323 = M4(m, 2, 0) * M4(m, 3, 3) - M4(m, 2, 3) * M4(m, 3, 0);
	float A0223 = M4(m, 2, 0) * M4(m, 3, 2) - M4(m, 2, 2) * M4(m, 3, 0);
	float A0123 = M4(m, 2, 0) * M4(m, 3, 1) - M4(m, 2, 1) * M4(m, 3, 0);
	return M4(m, 0, 0) *
			   (M4(m, 1, 1) * A2323 - M4(m, 1, 2) * A1323 + M4(m, 1, 3) * A1223) -
		   M4(m, 0, 1) *
			   (M4(m, 1, 0) * A2323 - M4(m, 1, 2) * A0323 + M4(m, 1, 3) * A0223) +
		   M4(m, 0, 2) *
			   (M4(m, 1, 0) * A1323 - M4(m, 1, 1) * A0323 + M4(m, 1, 3) * A0123) -
		   M4(m, 0, 3) *
			   (M4(m, 1, 0) * A1223 - M4(m, 1, 1) * A0223 + M4(m, 1, 2) * A0123);
}

spel_mat4 spel_mat4_inverse(spel_mat4 m)
{
	float A2323 = M4(m, 2, 2) * M4(m, 3, 3) - M4(m, 2, 3) * M4(m, 3, 2);
	float A1323 = M4(m, 2, 1) * M4(m, 3, 3) - M4(m, 2, 3) * M4(m, 3, 1);
	float A1223 = M4(m, 2, 1) * M4(m, 3, 2) - M4(m, 2, 2) * M4(m, 3, 1);
	float A0323 = M4(m, 2, 0) * M4(m, 3, 3) - M4(m, 2, 3) * M4(m, 3, 0);
	float A0223 = M4(m, 2, 0) * M4(m, 3, 2) - M4(m, 2, 2) * M4(m, 3, 0);
	float A0123 = M4(m, 2, 0) * M4(m, 3, 1) - M4(m, 2, 1) * M4(m, 3, 0);
	float A2313 = M4(m, 1, 2) * M4(m, 3, 3) - M4(m, 1, 3) * M4(m, 3, 2);
	float A1313 = M4(m, 1, 1) * M4(m, 3, 3) - M4(m, 1, 3) * M4(m, 3, 1);
	float A1213 = M4(m, 1, 1) * M4(m, 3, 2) - M4(m, 1, 2) * M4(m, 3, 1);
	float A2312 = M4(m, 1, 2) * M4(m, 2, 3) - M4(m, 1, 3) * M4(m, 2, 2);
	float A1312 = M4(m, 1, 1) * M4(m, 2, 3) - M4(m, 1, 3) * M4(m, 2, 1);
	float A1212 = M4(m, 1, 1) * M4(m, 2, 2) - M4(m, 1, 2) * M4(m, 2, 1);
	float A0313 = M4(m, 1, 0) * M4(m, 3, 3) - M4(m, 1, 3) * M4(m, 3, 0);
	float A0213 = M4(m, 1, 0) * M4(m, 3, 2) - M4(m, 1, 2) * M4(m, 3, 0);
	float A0312 = M4(m, 1, 0) * M4(m, 2, 3) - M4(m, 1, 3) * M4(m, 2, 0);
	float A0212 = M4(m, 1, 0) * M4(m, 2, 2) - M4(m, 1, 2) * M4(m, 2, 0);
	float A0113 = M4(m, 1, 0) * M4(m, 3, 1) - M4(m, 1, 1) * M4(m, 3, 0);
	float A0112 = M4(m, 1, 0) * M4(m, 2, 1) - M4(m, 1, 1) * M4(m, 2, 0);

	float det =
		M4(m, 0, 0) * (M4(m, 1, 1) * A2323 - M4(m, 1, 2) * A1323 + M4(m, 1, 3) * A1223) -
		M4(m, 0, 1) * (M4(m, 1, 0) * A2323 - M4(m, 1, 2) * A0323 + M4(m, 1, 3) * A0223) +
		M4(m, 0, 2) * (M4(m, 1, 0) * A1323 - M4(m, 1, 1) * A0323 + M4(m, 1, 3) * A0123) -
		M4(m, 0, 3) * (M4(m, 1, 0) * A1223 - M4(m, 1, 1) * A0223 + M4(m, 1, 2) * A0123);

	if (fabsf(det) < spel_epsilon)
	{
		return spel_mat4_identity();
	}
	float inv = 1.0F / det;

	spel_mat4 r;
	M4(r, 0, 0) = inv * (M4(m, 1, 1) * A2323 - M4(m, 1, 2) * A1323 + M4(m, 1, 3) * A1223);
	M4(r, 0, 1) =
		inv * -(M4(m, 0, 1) * A2323 - M4(m, 0, 2) * A1323 + M4(m, 0, 3) * A1223);
	M4(r, 0, 2) = inv * (M4(m, 0, 1) * A2313 - M4(m, 0, 2) * A1313 + M4(m, 0, 3) * A1213);
	M4(r, 0, 3) =
		inv * -(M4(m, 0, 1) * A2312 - M4(m, 0, 2) * A1312 + M4(m, 0, 3) * A1212);
	M4(r, 1, 0) =
		inv * -(M4(m, 1, 0) * A2323 - M4(m, 1, 2) * A0323 + M4(m, 1, 3) * A0223);
	M4(r, 1, 1) = inv * (M4(m, 0, 0) * A2323 - M4(m, 0, 2) * A0323 + M4(m, 0, 3) * A0223);
	M4(r, 1, 2) =
		inv * -(M4(m, 0, 0) * A2313 - M4(m, 0, 2) * A0313 + M4(m, 0, 3) * A0213);
	M4(r, 1, 3) = inv * (M4(m, 0, 0) * A2312 - M4(m, 0, 2) * A0312 + M4(m, 0, 3) * A0212);
	M4(r, 2, 0) = inv * (M4(m, 1, 0) * A1323 - M4(m, 1, 1) * A0323 + M4(m, 1, 3) * A0123);
	M4(r, 2, 1) =
		inv * -(M4(m, 0, 0) * A1323 - M4(m, 0, 1) * A0323 + M4(m, 0, 3) * A0123);
	M4(r, 2, 2) = inv * (M4(m, 0, 0) * A1313 - M4(m, 0, 1) * A0313 + M4(m, 0, 3) * A0113);
	M4(r, 2, 3) =
		inv * -(M4(m, 0, 0) * A1312 - M4(m, 0, 1) * A0312 + M4(m, 0, 3) * A0112);
	M4(r, 3, 0) =
		inv * -(M4(m, 1, 0) * A1223 - M4(m, 1, 1) * A0223 + M4(m, 1, 2) * A0123);
	M4(r, 3, 1) = inv * (M4(m, 0, 0) * A1223 - M4(m, 0, 1) * A0223 + M4(m, 0, 2) * A0123);
	M4(r, 3, 2) =
		inv * -(M4(m, 0, 0) * A1213 - M4(m, 0, 1) * A0213 + M4(m, 0, 2) * A0113);
	M4(r, 3, 3) = inv * (M4(m, 0, 0) * A1212 - M4(m, 0, 1) * A0112 + M4(m, 0, 2) * A0112);
	return r;
}

spel_mat4 spel_mat4_translate(float x, float y, float z)
{
	// Column-major: translation lives in column 3 (m[12], m[13], m[14])
	spel_mat4 r = spel_mat4_identity();
	M4(r, 0, 3) = x;
	M4(r, 1, 3) = y;
	M4(r, 2, 3) = z;
	return r;
}
spel_mat4 spel_mat4_translate_v(spel_vec3 v)
{
	return spel_mat4_translate(v.x, v.y, v.z);
}

spel_mat4 spel_mat4_scale(float x, float y, float z)
{
	spel_mat4 r = spel_mat4_zero();
	M4(r, 0, 0) = x;
	M4(r, 1, 1) = y;
	M4(r, 2, 2) = z;
	M4(r, 3, 3) = 1.0f;
	return r;
}
spel_mat4 spel_mat4_scale_v(spel_vec3 v)
{
	return spel_mat4_scale(v.x, v.y, v.z);
}
spel_mat4 spel_mat4_scale_uniform(float s)
{
	return spel_mat4_scale(s, s, s);
}

spel_mat4 spel_mat4_rotate_x(float radians)
{
	float c = cosf(radians);
	float s = sinf(radians);
	spel_mat4 m = spel_mat4_identity();
	M4(m, 1, 1) = c;
	M4(m, 1, 2) = -s;
	M4(m, 2, 1) = s;
	M4(m, 2, 2) = c;
	return m;
}
spel_mat4 spel_mat4_rotate_y(float radians)
{
	float c = cosf(radians);
	float s = sinf(radians);
	spel_mat4 m = spel_mat4_identity();
	M4(m, 0, 0) = c;
	M4(m, 0, 2) = s;
	M4(m, 2, 0) = -s;
	M4(m, 2, 2) = c;
	return m;
}
spel_mat4 spel_mat4_rotate_z(float radians)
{
	float c = cosf(radians);
	float s = sinf(radians);
	spel_mat4 m = spel_mat4_identity();
	M4(m, 0, 0) = c;
	M4(m, 0, 1) = -s;
	M4(m, 1, 0) = s;
	M4(m, 1, 1) = c;
	return m;
}
spel_mat4 spel_mat4_rotate_axis(spel_vec3 axis, float radians)
{
	spel_vec3 a = spel_vec3_normalize(axis);
	float c = cosf(radians);
	float s = sinf(radians);
	float t = 1.0F - c;
	spel_mat4 m = spel_mat4_identity();
	M4(m, 0, 0) = (t * a.x * a.x) + c;
	M4(m, 0, 1) = (t * a.x * a.y) - (s * a.z);
	M4(m, 0, 2) = (t * a.x * a.z) + (s * a.y);
	M4(m, 1, 0) = (t * a.x * a.y) + (s * a.z);
	M4(m, 1, 1) = (t * a.y * a.y) + c;
	M4(m, 1, 2) = (t * a.y * a.z) - (s * a.x);
	M4(m, 2, 0) = (t * a.x * a.z) - (s * a.y);
	M4(m, 2, 1) = (t * a.y * a.z) + (s * a.x);
	M4(m, 2, 2) = (t * a.z * a.z) + c;
	return m;
}
spel_mat4 spel_mat4_from_quat(spel_quat q)
{
	float xx = q.x * q.x;
	float yy = q.y * q.y;
	float zz = q.z * q.z;
	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float yz = q.y * q.z;
	float wx = q.w * q.x;
	float wy = q.w * q.y;
	float wz = q.w * q.z;
	spel_mat4 m = spel_mat4_identity();
	M4(m, 0, 0) = 1.0F - (2.0F * (yy + zz));
	M4(m, 0, 1) = 2.0F * (xy - wz);
	M4(m, 0, 2) = 2.0F * (xz + wy);
	M4(m, 1, 0) = 2.0F * (xy + wz);
	M4(m, 1, 1) = 1.0F - (2.0F * (xx + zz));
	M4(m, 1, 2) = 2.0F * (yz - wx);
	M4(m, 2, 0) = 2.0F * (xz - wy);
	M4(m, 2, 1) = 2.0F * (yz + wx);
	M4(m, 2, 2) = 1.0F - (2.0F * (xx + yy));
	return m;
}
spel_mat4 spel_mat4_trs(spel_vec3 t, spel_quat r, spel_vec3 s)
{
	spel_mat4 T = spel_mat4_translate_v(t);
	spel_mat4 R = spel_mat4_from_quat(r);
	spel_mat4 S = spel_mat4_scale_v(s);
	return spel_mat4_mul(T, spel_mat4_mul(R, S));
}

spel_mat4 spel_mat4_ortho(float l, float r, float b, float t, float n, float f)
{
	spel_mat4 m = spel_mat4_zero();
	M4(m, 0, 0) = 2.0F / (r - l);
	M4(m, 1, 1) = 2.0F / (t - b);
	M4(m, 2, 2) = -2.0F / (f - n);
	// Translation goes in column 3
	M4(m, 0, 3) = -(r + l) / (r - l);
	M4(m, 1, 3) = -(t + b) / (t - b);
	M4(m, 2, 3) = -(f + n) / (f - n);
	M4(m, 3, 3) = 1.0F;
	return m;
}
spel_mat4 spel_mat4_ortho_2d(float width, float height)
{
	return spel_mat4_ortho(0.0F, width, height, 0.0F, -1.0F, 1.0F);
}
spel_mat4 spel_mat4_perspective(float fovY, float aspect, float n, float f)
{
	float tan_half = tanf(fovY * 0.5F);
	spel_mat4 m = spel_mat4_zero();
	M4(m, 0, 0) = 1.0F / (aspect * tan_half);
	M4(m, 1, 1) = 1.0F / tan_half;
	M4(m, 2, 2) = -(f + n) / (f - n);
	M4(m, 2, 3) = -(2.0F * f * n) / (f - n); // stored in col 3, row 2
	M4(m, 3, 2) = -1.0F;					 // stored in col 2, row 3
	return m;
}
spel_mat4 spel_mat4_perspective_fov(float fovY, float w, float h, float n, float f)
{
	return spel_mat4_perspective(fovY, w / h, n, f);
}
spel_mat4 spel_mat4_frustum(float l, float r, float b, float t, float n, float f)
{
	spel_mat4 m = spel_mat4_zero();
	M4(m, 0, 0) = 2.0F * n / (r - l);
	M4(m, 0, 2) = (r + l) / (r - l);
	M4(m, 1, 1) = 2.0F * n / (t - b);
	M4(m, 1, 2) = (t + b) / (t - b);
	M4(m, 2, 2) = -(f + n) / (f - n);
	M4(m, 2, 3) = -2.0F * f * n / (f - n);
	M4(m, 3, 2) = -1.0F;
	return m;
}
spel_mat4 spel_mat4_look_at(spel_vec3 eye, spel_vec3 target, spel_vec3 up)
{
	spel_vec3 f = spel_vec3_normalize(spel_vec3_sub(target, eye));
	spel_vec3 s = spel_vec3_normalize(spel_vec3_cross(f, up));
	spel_vec3 u = spel_vec3_cross(s, f);
	spel_mat4 m = spel_mat4_identity();
	M4(m, 0, 0) = s.x;
	M4(m, 0, 1) = s.y;
	M4(m, 0, 2) = s.z;
	M4(m, 0, 3) = -spel_vec3_dot(s, eye);
	M4(m, 1, 0) = u.x;
	M4(m, 1, 1) = u.y;
	M4(m, 1, 2) = u.z;
	M4(m, 1, 3) = -spel_vec3_dot(u, eye);
	M4(m, 2, 0) = -f.x;
	M4(m, 2, 1) = -f.y;
	M4(m, 2, 2) = -f.z;
	M4(m, 2, 3) = spel_vec3_dot(f, eye);
	return m;
}

// Column 3 holds translation in column-major layout
spel_vec3 spel_mat4_get_translation(spel_mat4 m)
{
	return spel_vec3(M4(m, 0, 3), M4(m, 1, 3), M4(m, 2, 3));
}
// Scale = length of each basis column vector (columns 0-2)
spel_vec3 spel_mat4_get_scale(spel_mat4 m)
{
	return spel_vec3(sqrtf(M4(m, 0, 0) * M4(m, 0, 0) + M4(m, 1, 0) * M4(m, 1, 0) +
						   M4(m, 2, 0) * M4(m, 2, 0)),
					 sqrtf(M4(m, 0, 1) * M4(m, 0, 1) + M4(m, 1, 1) * M4(m, 1, 1) +
						   M4(m, 2, 1) * M4(m, 2, 1)),
					 sqrtf(M4(m, 0, 2) * M4(m, 0, 2) + M4(m, 1, 2) * M4(m, 1, 2) +
						   M4(m, 2, 2) * M4(m, 2, 2)));
}

int spel_mat4_eq(spel_mat4 a, spel_mat4 b)
{
	return memcmp(a.m, b.m, sizeof(a.m)) == 0;
}
int spel_mat4_nearly_eq(spel_mat4 a, spel_mat4 b, float eps)
{
	for (int i = 0; i < 16; i++)
	{
		if (fabsf(a.m[i] - b.m[i]) > eps)
		{
			return 0;
		}
	}
	return 1;
}

// ============================================================
//  MAT3
// ============================================================

#define M3(mt, r, c) ((mt).m[(c) * 3 + (r)])

spel_mat3 spel_mat3_identity(void)
{
	spel_mat3 r;
	memset(&r, 0, sizeof(r));
	r.m[0] = r.m[4] = r.m[8] = 1.0F;
	return r;
}
spel_mat3 spel_mat3_zero(void)
{
	spel_mat3 r;
	memset(&r, 0, sizeof(r));
	return r;
}

spel_mat3 spel_mat3_mul(spel_mat3 a, spel_mat3 b)
{
	spel_mat3 r;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			float s = 0.0F;
			for (int k = 0; k < 3; k++)
			{
				s += M3(a, i, k) * M3(b, k, j);
			}
			M3(r, i, j) = s;
		}
	}
	return r;
}
spel_vec3 spel_mat3_mul_vec3(spel_mat3 m, spel_vec3 v)
{
	return spel_vec3(M3(m, 0, 0) * v.x + M3(m, 0, 1) * v.y + M3(m, 0, 2) * v.z,
					 M3(m, 1, 0) * v.x + M3(m, 1, 1) * v.y + M3(m, 1, 2) * v.z,
					 M3(m, 2, 0) * v.x + M3(m, 2, 1) * v.y + M3(m, 2, 2) * v.z);
}
spel_mat3 spel_mat3_transpose(spel_mat3 m)
{
	spel_mat3 r;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			M3(r, i, j) = M3(m, j, i);
		}
	}
	return r;
}
float spel_mat3_determinant(spel_mat3 m)
{
	return M3(m, 0, 0) * (M3(m, 1, 1) * M3(m, 2, 2) - M3(m, 1, 2) * M3(m, 2, 1)) -
		   M3(m, 0, 1) * (M3(m, 1, 0) * M3(m, 2, 2) - M3(m, 1, 2) * M3(m, 2, 0)) +
		   M3(m, 0, 2) * (M3(m, 1, 0) * M3(m, 2, 1) - M3(m, 1, 1) * M3(m, 2, 0));
}
spel_mat3 spel_mat3_inverse(spel_mat3 m)
{
	float det = spel_mat3_determinant(m);
	if (fabsf(det) < spel_epsilon)
	{
		return spel_mat3_identity();
	}
	float inv = 1.0F / det;
	spel_mat3 r;
	M3(r, 0, 0) = inv * (M3(m, 1, 1) * M3(m, 2, 2) - M3(m, 1, 2) * M3(m, 2, 1));
	M3(r, 0, 1) = inv * (M3(m, 0, 2) * M3(m, 2, 1) - M3(m, 0, 1) * M3(m, 2, 2));
	M3(r, 0, 2) = inv * (M3(m, 0, 1) * M3(m, 1, 2) - M3(m, 0, 2) * M3(m, 1, 1));
	M3(r, 1, 0) = inv * (M3(m, 1, 2) * M3(m, 2, 0) - M3(m, 1, 0) * M3(m, 2, 2));
	M3(r, 1, 1) = inv * (M3(m, 0, 0) * M3(m, 2, 2) - M3(m, 0, 2) * M3(m, 2, 0));
	M3(r, 1, 2) = inv * (M3(m, 0, 2) * M3(m, 1, 0) - M3(m, 0, 0) * M3(m, 1, 2));
	M3(r, 2, 0) = inv * (M3(m, 1, 0) * M3(m, 2, 1) - M3(m, 1, 1) * M3(m, 2, 0));
	M3(r, 2, 1) = inv * (M3(m, 0, 1) * M3(m, 2, 0) - M3(m, 0, 0) * M3(m, 2, 1));
	M3(r, 2, 2) = inv * (M3(m, 0, 0) * M3(m, 1, 1) - M3(m, 0, 1) * M3(m, 1, 0));
	return r;
}
spel_mat3 spel_mat3_from_mat4(spel_mat4 m)
{
	spel_mat3 r;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			M3(r, i, j) = M4(m, i, j);
		}
	}
	return r;
}
spel_mat3 spel_mat3_normal_matrix(spel_mat4 model)
{
	return spel_mat3_transpose(spel_mat3_inverse(spel_mat3_from_mat4(model)));
}
spel_mat3 spel_mat3_rotate(float r)
{
	float c = cosf(r);
	float s = sinf(r);
	spel_mat3 m = spel_mat3_identity();
	M3(m, 0, 0) = c;
	M3(m, 0, 1) = -s;
	M3(m, 1, 0) = s;
	M3(m, 1, 1) = c;
	return m;
}

// ============================================================
//  MAT2
// ============================================================

#define M2(mt, r, c) ((mt).m[(c) * 2 + (r)])

spel_mat2 spel_mat2_identity(void)
{
	return (spel_mat2){{1, 0, 0, 1}};
}
spel_mat2 spel_mat2_zero(void)
{
	return (spel_mat2){{0, 0, 0, 0}};
}

spel_mat2 spel_mat2_mul(spel_mat2 a, spel_mat2 b)
{
	return (spel_mat2){{M2(a, 0, 0) * M2(b, 0, 0) + M2(a, 0, 1) * M2(b, 1, 0),
						M2(a, 0, 0) * M2(b, 0, 1) + M2(a, 0, 1) * M2(b, 1, 1),
						M2(a, 1, 0) * M2(b, 0, 0) + M2(a, 1, 1) * M2(b, 1, 0),
						M2(a, 1, 0) * M2(b, 0, 1) + M2(a, 1, 1) * M2(b, 1, 1)}};
}
spel_vec2 spel_mat2_mul_vec2(spel_mat2 m, spel_vec2 v)
{
	return spel_vec2(M2(m, 0, 0) * v.x + M2(m, 0, 1) * v.y,
					 M2(m, 1, 0) * v.x + M2(m, 1, 1) * v.y);
}
spel_mat2 spel_mat2_transpose(spel_mat2 m)
{
	return (spel_mat2){{m.m[0], m.m[2], m.m[1], m.m[3]}};
}
float spel_mat2_determinant(spel_mat2 m)
{
	return m.m[0] * m.m[3] - m.m[1] * m.m[2];
}
spel_mat2 spel_mat2_inverse(spel_mat2 m)
{
	float det = spel_mat2_determinant(m);
	if (fabsf(det) < spel_epsilon)
		return spel_mat2_identity();
	float inv = 1.0f / det;
	return (spel_mat2){{m.m[3] * inv, -m.m[1] * inv, -m.m[2] * inv, m.m[0] * inv}};
}
spel_mat2 spel_mat2_rotate(float r)
{
	float c = cosf(r), s = sinf(r);
	return (spel_mat2){{c, -s, s, c}};
}

// ============================================================
//  QUATERNION
// ============================================================

spel_quat spel_quat_from_axis_angle(spel_vec3 axis, float angle)
{
	float half = angle * 0.5f;
	float s = sinf(half);
	spel_vec3 a = spel_vec3_normalize(axis);
	return (spel_quat){a.x * s, a.y * s, a.z * s, cosf(half)};
}
spel_quat spel_quat_from_euler(float pitch, float yaw, float roll)
{
	float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
	float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
	float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
	return (spel_quat){sr * cp * cy - cr * sp * sy, cr * sp * cy + sr * cp * sy,
					   cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy};
}
spel_quat spel_quat_from_mat3(spel_mat3 m)
{
	float trace = M3(m, 0, 0) + M3(m, 1, 1) + M3(m, 2, 2);
	spel_quat q;
	if (trace > 0.0f)
	{
		float s = 0.5f / sqrtf(trace + 1.0f);
		q.w = 0.25f / s;
		q.x = (M3(m, 2, 1) - M3(m, 1, 2)) * s;
		q.y = (M3(m, 0, 2) - M3(m, 2, 0)) * s;
		q.z = (M3(m, 1, 0) - M3(m, 0, 1)) * s;
	}
	else if (M3(m, 0, 0) > M3(m, 1, 1) && M3(m, 0, 0) > M3(m, 2, 2))
	{
		float s = 2.0f * sqrtf(1.0f + M3(m, 0, 0) - M3(m, 1, 1) - M3(m, 2, 2));
		q.w = (M3(m, 2, 1) - M3(m, 1, 2)) / s;
		q.x = 0.25f * s;
		q.y = (M3(m, 0, 1) + M3(m, 1, 0)) / s;
		q.z = (M3(m, 0, 2) + M3(m, 2, 0)) / s;
	}
	else if (M3(m, 1, 1) > M3(m, 2, 2))
	{
		float s = 2.0f * sqrtf(1.0f + M3(m, 1, 1) - M3(m, 0, 0) - M3(m, 2, 2));
		q.w = (M3(m, 0, 2) - M3(m, 2, 0)) / s;
		q.x = (M3(m, 0, 1) + M3(m, 1, 0)) / s;
		q.y = 0.25f * s;
		q.z = (M3(m, 1, 2) + M3(m, 2, 1)) / s;
	}
	else
	{
		float s = 2.0f * sqrtf(1.0f + M3(m, 2, 2) - M3(m, 0, 0) - M3(m, 1, 1));
		q.w = (M3(m, 1, 0) - M3(m, 0, 1)) / s;
		q.x = (M3(m, 0, 2) + M3(m, 2, 0)) / s;
		q.y = (M3(m, 1, 2) + M3(m, 2, 1)) / s;
		q.z = 0.25f * s;
	}
	return q;
}
spel_quat spel_quat_from_mat4(spel_mat4 m)
{
	return spel_quat_from_mat3(spel_mat3_from_mat4(m));
}

spel_quat spel_quat_from_to(spel_vec3 from, spel_vec3 to)
{
	spel_vec3 f = spel_vec3_normalize(from);
	spel_vec3 t = spel_vec3_normalize(to);
	float d = spel_vec3_dot(f, t);
	if (d >= 1.0f - spel_epsilon)
		return spel_quat_identity;
	if (d <= -1.0f + spel_epsilon)
	{
		spel_vec3 perp = spel_vec3_cross(spel_vec3_up, f);
		if (spel_vec3_len_sq(perp) < spel_epsilon)
			perp = spel_vec3_cross(spel_vec3_right, f);
		return spel_quat_from_axis_angle(spel_vec3_normalize(perp), spel_pi);
	}
	spel_vec3 axis = spel_vec3_cross(f, t);
	return spel_quat_normalize((spel_quat){axis.x, axis.y, axis.z, 1.0f + d});
}
spel_quat spel_quat_look_at(spel_vec3 forward, spel_vec3 up)
{
	spel_vec3 f = spel_vec3_normalize(forward);
	spel_vec3 r = spel_vec3_normalize(spel_vec3_cross(up, f));
	spel_vec3 u = spel_vec3_cross(f, r);
	spel_mat3 m;
	M3(m, 0, 0) = r.x;
	M3(m, 0, 1) = r.y;
	M3(m, 0, 2) = r.z;
	M3(m, 1, 0) = u.x;
	M3(m, 1, 1) = u.y;
	M3(m, 1, 2) = u.z;
	M3(m, 2, 0) = f.x;
	M3(m, 2, 1) = f.y;
	M3(m, 2, 2) = f.z;
	return spel_quat_from_mat3(m);
}

spel_quat spel_quat_mul(spel_quat a, spel_quat b)
{
	return (spel_quat){a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
					   a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
					   a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
					   a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
spel_vec3 spel_quat_mul_vec3(spel_quat q, spel_vec3 v)
{
	spel_vec3 qv = {q.x, q.y, q.z};
	spel_vec3 t = spel_vec3_scale(spel_vec3_cross(qv, v), 2.0f);
	return spel_vec3_add(v,
						 spel_vec3_add(spel_vec3_scale(t, q.w), spel_vec3_cross(qv, t)));
}
spel_quat spel_quat_conjugate(spel_quat q)
{
	return (spel_quat){-q.x, -q.y, -q.z, q.w};
}
spel_quat spel_quat_inverse(spel_quat q)
{
	float ls = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
	if (ls < spel_epsilon)
		return spel_quat_identity;
	float inv = 1.0f / ls;
	return (spel_quat){-q.x * inv, -q.y * inv, -q.z * inv, q.w * inv};
}
float spel_quat_dot(spel_quat a, spel_quat b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
float spel_quat_len_sq(spel_quat q)
{
	return spel_quat_dot(q, q);
}
float spel_quat_len(spel_quat q)
{
	return sqrtf(spel_quat_len_sq(q));
}
spel_quat spel_quat_normalize(spel_quat q)
{
	float l = spel_quat_len(q);
	if (l < spel_epsilon)
		return spel_quat_identity;
	return (spel_quat){q.x / l, q.y / l, q.z / l, q.w / l};
}
spel_quat spel_quat_nlerp(spel_quat a, spel_quat b, float t)
{
	if (spel_quat_dot(a, b) < 0.0f)
		b = (spel_quat){-b.x, -b.y, -b.z, -b.w};
	return spel_quat_normalize((spel_quat){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
										   a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t});
}
spel_quat spel_quat_slerp(spel_quat a, spel_quat b, float t)
{
	float d = spel_quat_dot(a, b);
	if (d < 0.0f)
	{
		b = (spel_quat){-b.x, -b.y, -b.z, -b.w};
		d = -d;
	}
	if (d > 1.0f - spel_epsilon)
		return spel_quat_nlerp(a, b, t);
	float omega = acosf(d);
	float s = sinf(omega);
	float wa = sinf((1.0f - t) * omega) / s;
	float wb = sinf(t * omega) / s;
	return spel_quat_normalize((spel_quat){wa * a.x + wb * b.x, wa * a.y + wb * b.y,
										   wa * a.z + wb * b.z, wa * a.w + wb * b.w});
}
void spel_quat_to_axis_angle(spel_quat q, spel_vec3* axis, float* radians)
{
	spel_quat nq = spel_quat_normalize(q);
	float s = sqrtf(1.0f - nq.w * nq.w);
	*radians = 2.0f * acosf(nq.w);
	if (s < spel_epsilon)
	{
		*axis = spel_vec3(1, 0, 0);
		return;
	}
	*axis = spel_vec3(nq.x / s, nq.y / s, nq.z / s);
}
spel_vec3 spel_quat_to_euler(spel_quat q)
{
	float sinp = 2.0f * (q.w * q.y - q.z * q.x);
	float pitch = fabsf(sinp) >= 1.0f ? copysignf(spel_pi / 2.0f, sinp) : asinf(sinp);
	float yaw =
		atan2f(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y * q.y + q.z * q.z));
	float roll =
		atan2f(2.0f * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y));
	return spel_vec3(pitch, yaw, roll);
}
spel_mat4 spel_quat_to_mat4(spel_quat q)
{
	return spel_mat4_from_quat(q);
}
int spel_quat_eq(spel_quat a, spel_quat b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
int spel_quat_nearly_eq(spel_quat a, spel_quat b, float eps)
{
	return fabsf(a.x - b.x) <= eps && fabsf(a.y - b.y) <= eps &&
		   fabsf(a.z - b.z) <= eps && fabsf(a.w - b.w) <= eps;
}

// ============================================================
//  RECT
// ============================================================

spel_rect spel_rect_make(int x, int y, int w, int h)
{
	return (spel_rect){x, y, w, h};
}
spel_rect spel_rect_from_points(int x0, int y0, int x1, int y1)
{
	int xl = x0 < x1 ? x0 : x1, yl = y0 < y1 ? y0 : y1;
	return (spel_rect){xl, yl, (x0 > x1 ? x0 : x1) - xl, (y0 > y1 ? y0 : y1) - yl};
}
int spel_rect_right(spel_rect r)
{
	return r.x + r.width;
}
int spel_rect_bottom(spel_rect r)
{
	return r.y + r.height;
}
spel_vec2 spel_rect_center(spel_rect r)
{
	return spel_vec2(r.x + r.width * 0.5f, r.y + r.height * 0.5f);
}
spel_vec2 spel_rect_size(spel_rect r)
{
	return spel_vec2((float)r.width, (float)r.height);
}

int spel_rect_contains_point(spel_rect r, int x, int y)
{
	return x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height;
}
int spel_rect_contains_rect(spel_rect outer, spel_rect inner)
{
	return inner.x >= outer.x && inner.y >= outer.y &&
		   spel_rect_right(inner) <= spel_rect_right(outer) &&
		   spel_rect_bottom(inner) <= spel_rect_bottom(outer);
}
int spel_rect_intersects(spel_rect a, spel_rect b)
{
	return a.x < spel_rect_right(b) && spel_rect_right(a) > b.x &&
		   a.y < spel_rect_bottom(b) && spel_rect_bottom(a) > b.y;
}
spel_rect spel_rect_intersection(spel_rect a, spel_rect b)
{
	int x0 = a.x > b.x ? a.x : b.x, y0 = a.y > b.y ? a.y : b.y;
	int x1 =
		spel_rect_right(a) < spel_rect_right(b) ? spel_rect_right(a) : spel_rect_right(b);
	int y1 = spel_rect_bottom(a) < spel_rect_bottom(b) ? spel_rect_bottom(a)
													   : spel_rect_bottom(b);
	if (x1 <= x0 || y1 <= y0)
		return (spel_rect){0, 0, 0, 0};
	return (spel_rect){x0, y0, x1 - x0, y1 - y0};
}
spel_rect spel_rect_union(spel_rect a, spel_rect b)
{
	int x0 = a.x < b.x ? a.x : b.x, y0 = a.y < b.y ? a.y : b.y;
	int x1 =
		spel_rect_right(a) > spel_rect_right(b) ? spel_rect_right(a) : spel_rect_right(b);
	int y1 = spel_rect_bottom(a) > spel_rect_bottom(b) ? spel_rect_bottom(a)
													   : spel_rect_bottom(b);
	return (spel_rect){x0, y0, x1 - x0, y1 - y0};
}
spel_rect spel_rect_expand(spel_rect r, int amount)
{
	return (spel_rect){r.x - amount, r.y - amount, r.width + 2 * amount,
					   r.height + 2 * amount};
}
spel_rect spel_rect_shrink(spel_rect r, int amount)
{
	return spel_rect_expand(r, -amount);
}
spel_rect spel_rect_translate(spel_rect r, int dx, int dy)
{
	return (spel_rect){r.x + dx, r.y + dy, r.width, r.height};
}
spel_rect spel_rect_center_in(spel_rect inner, spel_rect outer)
{
	return (spel_rect){outer.x + (outer.width - inner.width) / 2,
					   outer.y + (outer.height - inner.height) / 2, inner.width,
					   inner.height};
}
int spel_rect_eq(spel_rect a, spel_rect b)
{
	return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

// ============================================================
//  AABB
// ============================================================

spel_aabb spel_aabb_make(spel_vec3 min, spel_vec3 max)
{
	return (spel_aabb){min, max};
}
spel_aabb spel_aabb_from_center(spel_vec3 c, spel_vec3 he)
{
	return (spel_aabb){spel_vec3_sub(c, he), spel_vec3_add(c, he)};
}
spel_vec3 spel_aabb_center(spel_aabb b)
{
	return spel_vec3_scale(spel_vec3_add(b.min, b.max), 0.5f);
}
spel_vec3 spel_aabb_size(spel_aabb b)
{
	return spel_vec3_sub(b.max, b.min);
}
spel_vec3 spel_aabb_extents(spel_aabb b)
{
	return spel_vec3_scale(spel_aabb_size(b), 0.5f);
}

int spel_aabb_contains_point(spel_aabb b, spel_vec3 p)
{
	return p.x >= b.min.x && p.x <= b.max.x && p.y >= b.min.y && p.y <= b.max.y &&
		   p.z >= b.min.z && p.z <= b.max.z;
}
int spel_aabb_intersects(spel_aabb a, spel_aabb b)
{
	return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y &&
		   a.max.y >= b.min.y && a.min.z <= b.max.z && a.max.z >= b.min.z;
}
spel_aabb spel_aabb_union(spel_aabb a, spel_aabb b)
{
	return (spel_aabb){spel_vec3_min(a.min, b.min), spel_vec3_max(a.max, b.max)};
}
spel_aabb spel_aabb_expand(spel_aabb b, spel_vec3 amount)
{
	return (spel_aabb){spel_vec3_sub(b.min, amount), spel_vec3_add(b.max, amount)};
}
spel_aabb spel_aabb_transform(spel_aabb b, spel_mat4 m)
{
	// Transform all 8 corners, return enclosing AABB
	spel_vec3 verts[8] = {{b.min.x, b.min.y, b.min.z}, {b.max.x, b.min.y, b.min.z},
						  {b.min.x, b.max.y, b.min.z}, {b.max.x, b.max.y, b.min.z},
						  {b.min.x, b.min.y, b.max.z}, {b.max.x, b.min.y, b.max.z},
						  {b.min.x, b.max.y, b.max.z}, {b.max.x, b.max.y, b.max.z}};
	spel_vec3 mn = spel_mat4_mul_vec3(m, verts[0], 1.0f);
	spel_vec3 mx = mn;
	for (int i = 1; i < 8; i++)
	{
		spel_vec3 p = spel_mat4_mul_vec3(m, verts[i], 1.0f);
		mn = spel_vec3_min(mn, p);
		mx = spel_vec3_max(mx, p);
	}
	return (spel_aabb){mn, mx};
}

// ============================================================
//  CIRCLE
// ============================================================

int spel_circle_contains_point(spel_circle c, spel_vec2 p)
{
	return spel_vec2_dist_sq(c.center, p) <= c.radius * c.radius;
}
int spel_circle_intersects_circle(spel_circle a, spel_circle b)
{
	float rd = a.radius + b.radius;
	return spel_vec2_dist_sq(a.center, b.center) <= rd * rd;
}
int spel_circle_intersects_rect(spel_circle c, spel_rect r)
{
	float cx = c.center.x, cy = c.center.y;
	float nx = cx < r.x ? r.x : cx > r.x + r.width ? (float)(r.x + r.width) : cx;
	float ny = cy < r.y ? r.y : cy > r.y + r.height ? (float)(r.y + r.height) : cy;
	float dx = cx - nx, dy = cy - ny;
	return dx * dx + dy * dy <= c.radius * c.radius;
}

// ============================================================
//  RAY
// ============================================================

spel_vec3 spel_ray_at(spel_ray ray, float t)
{
	return spel_vec3_add(ray.origin, spel_vec3_scale(ray.direction, t));
}
int spel_ray_intersects_plane(spel_ray ray, spel_plane plane, float* out_t)
{
	float denom = spel_vec3_dot(plane.normal, ray.direction);
	if (fabsf(denom) < spel_epsilon)
		return 0;
	float t = -(spel_vec3_dot(plane.normal, ray.origin) + plane.d) / denom;
	if (t < 0.0f)
		return 0;
	if (out_t)
		*out_t = t;
	return 1;
}
int spel_ray_intersects_sphere(spel_ray ray, spel_vec3 center, float radius, float* out_t)
{
	spel_vec3 oc = spel_vec3_sub(ray.origin, center);
	float a = spel_vec3_dot(ray.direction, ray.direction);
	float b = 2.0f * spel_vec3_dot(oc, ray.direction);
	float c = spel_vec3_dot(oc, oc) - radius * radius;
	float disc = b * b - 4.0f * a * c;
	if (disc < 0.0f)
		return 0;
	float t = (-b - sqrtf(disc)) / (2.0f * a);
	if (t < 0.0f)
		t = (-b + sqrtf(disc)) / (2.0f * a);
	if (t < 0.0f)
		return 0;
	if (out_t)
		*out_t = t;
	return 1;
}
int spel_ray_intersects_aabb(spel_ray ray, spel_aabb box, float* out_t)
{
	float tmin = -spel_inf, tmax = spel_inf;
	float *orig = &ray.origin.x, *dir = &ray.direction.x;
	float *bmin = &box.min.x, *bmax = &box.max.x;
	for (int i = 0; i < 3; i++)
	{
		if (fabsf(dir[i]) < spel_epsilon)
		{
			if (orig[i] < bmin[i] || orig[i] > bmax[i])
				return 0;
		}
		else
		{
			float t0 = (bmin[i] - orig[i]) / dir[i];
			float t1 = (bmax[i] - orig[i]) / dir[i];
			if (t0 > t1)
			{
				float tmp = t0;
				t0 = t1;
				t1 = tmp;
			}
			tmin = tmin > t0 ? tmin : t0;
			tmax = tmax < t1 ? tmax : t1;
			if (tmin > tmax)
				return 0;
		}
	}
	if (tmax < 0.0f)
		return 0;
	if (out_t)
		*out_t = tmin < 0.0f ? tmax : tmin;
	return 1;
}
int spel_ray_intersects_triangle(spel_ray ray, spel_vec3 v0, spel_vec3 v1, spel_vec3 v2,
								 float* out_t)
{
	// Möller–Trumbore
	spel_vec3 e1 = spel_vec3_sub(v1, v0);
	spel_vec3 e2 = spel_vec3_sub(v2, v0);
	spel_vec3 h = spel_vec3_cross(ray.direction, e2);
	float a = spel_vec3_dot(e1, h);
	if (fabsf(a) < spel_epsilon)
		return 0;
	float f = 1.0f / a;
	spel_vec3 s = spel_vec3_sub(ray.origin, v0);
	float u = f * spel_vec3_dot(s, h);
	if (u < 0.0f || u > 1.0f)
		return 0;
	spel_vec3 q = spel_vec3_cross(s, e1);
	float v = f * spel_vec3_dot(ray.direction, q);
	if (v < 0.0f || u + v > 1.0f)
		return 0;
	float t = f * spel_vec3_dot(e2, q);
	if (t < spel_epsilon)
		return 0;
	if (out_t)
		*out_t = t;
	return 1;
}

// ============================================================
//  PLANE
// ============================================================

spel_plane spel_plane_make(spel_vec3 normal, spel_vec3 point)
{
	spel_vec3 n = spel_vec3_normalize(normal);
	return (spel_plane){n, -spel_vec3_dot(n, point)};
}
float spel_plane_dist_to_point(spel_plane p, spel_vec3 point)
{
	return spel_vec3_dot(p.normal, point) + p.d;
}
spel_vec3 spel_plane_project_point(spel_plane p, spel_vec3 point)
{
	float d = spel_plane_dist_to_point(p, point);
	return spel_vec3_sub(point, spel_vec3_scale(p.normal, d));
}
int spel_plane_side(spel_plane p, spel_vec3 point)
{
	float d = spel_plane_dist_to_point(p, point);
	return d > spel_epsilon ? 1 : d < -spel_epsilon ? -1 : 0;
}

// ============================================================
//  COLOR
// ============================================================

static float hue_to_rgb(float p, float q, float t)
{
	if (t < 0.0f)
		t += 1.0f;
	if (t > 1.0f)
		t -= 1.0f;
	if (t < 1.0f / 6.0f)
		return p + (q - p) * 6.0f * t;
	if (t < 0.5f)
		return q;
	if (t < 2.0f / 3.0f)
		return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
	return p;
}

spel_color spel_color_hsl(int h, float s, float l, uint8_t a)
{
	float hf = ((float)(h % 360)) / 360.0f;
	float r, g, b;
	if (s == 0.0f)
	{
		r = g = b = l;
	}
	else
	{
		float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
		float p = 2.0f * l - q;
		r = hue_to_rgb(p, q, hf + 1.0f / 3.0f);
		g = hue_to_rgb(p, q, hf);
		b = hue_to_rgb(p, q, hf - 1.0f / 3.0f);
	}
	return (spel_color){(uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), a};
}
spel_color spel_color_hsv(int h, float s, float v, uint8_t a)
{
	float hf = ((float)(h % 360)) / 60.0f;
	int i = (int)hf;
	float f = hf - (float)i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));
	float r, g, b;
	switch (i % 6)
	{
	case 0:
		r = v;
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = v;
		b = p;
		break;
	case 2:
		r = p;
		g = v;
		b = t;
		break;
	case 3:
		r = p;
		g = q;
		b = v;
		break;
	case 4:
		r = t;
		g = p;
		b = v;
		break;
	default:
		r = v;
		g = p;
		b = q;
		break;
	}
	return (spel_color){(uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255), a};
}
spel_color spel_color_mix(spel_color a, spel_color b, float t)
{
	return (spel_color){
		(uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t),
		(uint8_t)(a.b + (b.b - a.b) * t), (uint8_t)(a.a + (b.a - a.a) * t)};
}
spel_color spel_color_invert(spel_color c)
{
	return (spel_color){255 - c.r, 255 - c.g, 255 - c.b, c.a};
}
spel_color spel_color_alpha(spel_color c, uint8_t a)
{
	return (spel_color){c.r, c.g, c.b, a};
}

spel_color spel_color_darken(spel_color c, float amount)
{
	float t = 1.0f - (amount < 0 ? 0 : amount > 1 ? 1 : amount);
	return (spel_color){(uint8_t)(c.r * t), (uint8_t)(c.g * t), (uint8_t)(c.b * t), c.a};
}
spel_color spel_color_lighten(spel_color c, float amount)
{
	float a = amount < 0 ? 0 : amount > 1 ? 1 : amount;
	return (spel_color){(uint8_t)(c.r + (255 - c.r) * a),
						(uint8_t)(c.g + (255 - c.g) * a),
						(uint8_t)(c.b + (255 - c.b) * a), c.a};
}
spel_color spel_color_grayscale(spel_color c)
{
	uint8_t g = (uint8_t)(0.299f * c.r + 0.587f * c.g + 0.114f * c.b);
	return (spel_color){g, g, g, c.a};
}
spel_color spel_color_premultiply_alpha(spel_color c)
{
	float a = c.a / 255.0f;
	return (spel_color){(uint8_t)(c.r * a), (uint8_t)(c.g * a), (uint8_t)(c.b * a), c.a};
}
spel_color spel_color_saturate(spel_color c, float amount)
{
	spel_color gs = spel_color_grayscale(c);
	return spel_color_mix(gs, c, 1.0f + amount);
}
spel_color spel_color_desaturate(spel_color c, float amount)
{
	return spel_color_mix(c, spel_color_grayscale(c), amount);
}

spel_vec4 spel_color_to_vec4(spel_color c)
{
	return spel_vec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}
spel_color spel_color_from_vec4(spel_vec4 v)
{
	return (spel_color){(uint8_t)(v.x * 255), (uint8_t)(v.y * 255), (uint8_t)(v.z * 255),
						(uint8_t)(v.w * 255)};
}
uint32_t spel_color_to_u32_rgba(spel_color c)
{
	return ((uint32_t)c.r << 24) | ((uint32_t)c.g << 16) | ((uint32_t)c.b << 8) |
		   (uint32_t)c.a;
}
uint32_t spel_color_to_u32_argb(spel_color c)
{
	return ((uint32_t)c.a << 24) | ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) |
		   (uint32_t)c.b;
}
spel_color spel_color_from_u32_rgba(uint32_t v)
{
	return (spel_color){(v >> 24) & 0xFF, (v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF};
}
float* spel_color_array(spel_color c)
{
	// NOTE: Returns pointer to a static buffer — not thread-safe.
	// For multi-threaded use, copy into caller-owned storage.
	static float buf[4];
	buf[0] = c.r / 255.0f;
	buf[1] = c.g / 255.0f;
	buf[2] = c.b / 255.0f;
	buf[3] = c.a / 255.0f;
	return buf;
}

// ============================================================
//  EASING
// ============================================================

float spel_ease_linear(float t)
{
	return t;
}

float spel_ease_in_sine(float t)
{
	return 1.0f - cosf(t * spel_pi * 0.5f);
}
float spel_ease_out_sine(float t)
{
	return sinf(t * spel_pi * 0.5f);
}
float spel_ease_in_out_sine(float t)
{
	return 0.5f * (1.0f - cosf(spel_pi * t));
}

float spel_ease_in_quad(float t)
{
	return t * t;
}
float spel_ease_out_quad(float t)
{
	return t * (2.0f - t);
}
float spel_ease_in_out_quad(float t)
{
	return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float spel_ease_in_cubic(float t)
{
	return t * t * t;
}
float spel_ease_out_cubic(float t)
{
	float u = t - 1.0f;
	return u * u * u + 1.0f;
}
float spel_ease_in_out_cubic(float t)
{
	return t < 0.5f ? 4.0f * t * t * t
					: (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

float spel_ease_in_quart(float t)
{
	return t * t * t * t;
}
float spel_ease_out_quart(float t)
{
	float u = t - 1.0f;
	return 1.0f - u * u * u * u;
}
float spel_ease_in_out_quart(float t)
{
	return t < 0.5f ? 8.0f * t * t * t * t
					: 1.0f - 8.0f * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) * (t - 1.0f);
}

float spel_ease_in_quint(float t)
{
	return t * t * t * t * t;
}
float spel_ease_out_quint(float t)
{
	float u = t - 1.0f;
	return u * u * u * u * u + 1.0f;
}
float spel_ease_in_out_quint(float t)
{
	return t < 0.5f ? 16.0f * t * t * t * t * t
					: 1.0f + 16.0f * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) *
								 (t - 1.0f);
}

float spel_ease_in_expo(float t)
{
	return t == 0.0f ? 0.0f : powf(2.0f, 10.0f * (t - 1.0f));
}
float spel_ease_out_expo(float t)
{
	return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
}
float spel_ease_in_out_expo(float t)
{
	if (t == 0.0f || t == 1.0f)
		return t;
	return t < 0.5f ? powf(2.0f, 20.0f * t - 10.0f) * 0.5f
					: (2.0f - powf(2.0f, -20.0f * t + 10.0f)) * 0.5f;
}

float spel_ease_in_circ(float t)
{
	return 1.0f - sqrtf(1.0f - t * t);
}
float spel_ease_out_circ(float t)
{
	float u = t - 1.0f;
	return sqrtf(1.0f - u * u);
}
float spel_ease_in_out_circ(float t)
{
	return t < 0.5f ? (1.0f - sqrtf(1.0f - 4.0f * t * t)) * 0.5f
					: (sqrtf(1.0f - (2.0f * t - 2.0f) * (2.0f * t - 2.0f)) + 1.0f) * 0.5f;
}

float spel_ease_in_back(float t)
{
	float c = 1.70158f;
	return t * t * ((c + 1.0f) * t - c);
}
float spel_ease_out_back(float t)
{
	float c = 1.70158f, u = t - 1.0f;
	return u * u * ((c + 1.0f) * u + c) + 1.0f;
}
float spel_ease_in_out_back(float t)
{
	float c = 1.70158f * 1.525f;
	return t < 0.5f ? (4.0f * t * t * ((c + 1.0f) * 2.0f * t - c)) * 0.5f
					: ((2.0f * t - 2.0f) * (2.0f * t - 2.0f) *
						   ((c + 1.0f) * (2.0f * t - 2.0f) + c) +
					   2.0f) *
						  0.5f;
}

float spel_ease_out_bounce(float t)
{
	if (t < 1.0f / 2.75f)
		return 7.5625f * t * t;
	else if (t < 2.0f / 2.75f)
	{
		t -= 1.5f / 2.75f;
		return 7.5625f * t * t + 0.75f;
	}
	else if (t < 2.5f / 2.75f)
	{
		t -= 2.25f / 2.75f;
		return 7.5625f * t * t + 0.9375f;
	}
	else
	{
		t -= 2.625f / 2.75f;
		return 7.5625f * t * t + 0.984375f;
	}
}
float spel_ease_in_bounce(float t)
{
	return 1.0f - spel_ease_out_bounce(1.0f - t);
}
float spel_ease_in_out_bounce(float t)
{
	return t < 0.5f ? spel_ease_in_bounce(2.0f * t) * 0.5f
					: spel_ease_out_bounce(2.0f * t - 1.0f) * 0.5f + 0.5f;
}

float spel_ease_out_elastic(float t)
{
	if (t == 0.0f || t == 1.0f)
		return t;
	return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * (spel_tau / 3.0f)) + 1.0f;
}
float spel_ease_in_elastic(float t)
{
	return 1.0f - spel_ease_out_elastic(1.0f - t);
}
float spel_ease_in_out_elastic(float t)
{
	if (t == 0.0f || t == 1.0f)
		return t;
	return t < 0.5f ? -(powf(2.0f, 20.0f * t - 10.0f) *
						sinf((20.0f * t - 11.125f) * (spel_tau / 4.5f))) *
						  0.5f
					: (powf(2.0f, -20.0f * t + 10.0f) *
					   sinf((20.0f * t - 11.125f) * (spel_tau / 4.5f))) *
							  0.5f +
						  1.0f;
}

// ============================================================
//  RANDOM  (PCG32)
// ============================================================

spel_rng spel_rng_make(uint64_t seed)
{
	return (spel_rng){seed + 1442695040888963407ULL};
}

uint32_t spel_rng_next(spel_rng* rng)
{
	uint64_t old = rng->state;
	rng->state = old * 6364136223846793005ULL + 1442695040888963407ULL;
	uint32_t xsh = (uint32_t)(((old >> 18u) ^ old) >> 27u);
	uint32_t rot = (uint32_t)(old >> 59u);
	return (xsh >> rot) | (xsh << ((-rot) & 31));
}
float spel_rng_float(spel_rng* rng)
{
	return (float)(spel_rng_next(rng) >> 8) * (1.0f / 16777216.0f);
}
float spel_rng_float_range(spel_rng* rng, float lo, float hi)
{
	return lo + spel_rng_float(rng) * (hi - lo);
}
int spel_rng_int_range(spel_rng* rng, int lo, int hi)
{
	return lo + (int)(spel_rng_next(rng) % (uint32_t)(hi - lo + 1));
}
spel_vec2 spel_rng_vec2_unit(spel_rng* rng)
{
	float a = spel_rng_float_range(rng, 0.0f, spel_tau);
	return spel_vec2(cosf(a), sinf(a));
}
spel_vec3 spel_rng_vec3_unit(spel_rng* rng)
{
	float z = spel_rng_float_range(rng, -1.0f, 1.0f);
	float a = spel_rng_float_range(rng, 0.0f, spel_tau);
	float r = sqrtf(1.0f - z * z);
	return spel_vec3(r * cosf(a), r * sinf(a), z);
}
spel_color spel_rng_color(spel_rng* rng, uint8_t alpha)
{
	uint32_t v = spel_rng_next(rng);
	return (spel_color){(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), alpha};
}

// ============================================================
//  NOISE
// ============================================================

// Permutation table (Ken Perlin's original)
static const uint8_t PERM[512] = {
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103,
	30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197,
	62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20,
	125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231,
	83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102,
	143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200,
	196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
	250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16,
	58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221,
	153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232,
	178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179,
	162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
	184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114,
	67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
	// repeat
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103,
	30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197,
	62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20,
	125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231,
	83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102,
	143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200,
	196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226,
	250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16,
	58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221,
	153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232,
	178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179,
	162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
	184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114,
	67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180};

static float fade(float t)
{
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}
static float grad1(int h, float x)
{
	return (h & 1) ? -x : x;
}
static float grad2(int h, float x, float y)
{
	return ((h & 1) ? -x : x) + ((h & 2) ? -y : y);
}

float spel_noise_value(float x)
{
	int xi = (int)floorf(x) & 255;
	float xf = x - floorf(x);
	float u = fade(xf);
	return spel_math_lerp(grad1(PERM[xi], xf), grad1(PERM[xi + 1], xf - 1.0f), u);
}

float spel_noise_value2(float x, float y)
{
	int xi = (int)floorf(x) & 255;
	int yi = (int)floorf(y) & 255;
	float xf = x - floorf(x);
	float yf = y - floorf(y);
	float u = fade(xf), v = fade(yf);
	int aa = PERM[PERM[xi] + yi], ab = PERM[PERM[xi] + yi + 1];
	int ba = PERM[PERM[xi + 1] + yi], bb = PERM[PERM[xi + 1] + yi + 1];
	float x0 = spel_math_lerp(grad2(aa, xf, yf), grad2(ba, xf - 1.0f, yf), u);
	float x1 =
		spel_math_lerp(grad2(ab, xf, yf - 1.0f), grad2(bb, xf - 1.0f, yf - 1.0f), u);
	return spel_math_lerp(x0, x1, v);
}

float spel_noise_perlin(float x, float y)
{
	return spel_noise_value2(x, y);
}

float spel_noise_fbm(float x, float y, int octaves, float lacunarity, float gain)
{
	float value = 0.0f, amplitude = 0.5f, frequency = 1.0f;
	for (int i = 0; i < octaves; i++)
	{
		value += amplitude * spel_noise_perlin(x * frequency, y * frequency);
		amplitude *= gain;
		frequency *= lacunarity;
	}
	return value;
}