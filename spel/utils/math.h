#pragma once
#include "core/macros.h"
#include <stdint.h>

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} spel_color;

sp_color_declare(black);
sp_color_declare(white);

sp_color_declare(red); // primary colors
sp_color_declare(green);
sp_color_declare(blue);

sp_color_declare(cyan); // secondary colors
sp_color_declare(magenta);
sp_color_declare(yellow);

sp_api spel_color spel_color_hsl(int h, float s, float l, uint8_t a);
sp_api spel_color spel_color_hsv(int h, float s, float v, uint8_t a);

sp_api spel_color spel_color_mix(spel_color a, spel_color b, float t);
sp_api spel_color spel_color_invert(spel_color value);

#define sp_e 2.71828182845904523536
#define sp_log2e 1.44269504088896340736
#define sp_log10e 0.43429448190325182765
#define sp_ln2 0.69314718055994530942
#define sp_ln10 2.30258509299404568402
#define sp_pi 3.14159265358979323846
#define sp_pi_2 1.57079632679489661923 // half of a pie
#define sp_pi_4 0.78539816339744830962 // a quarter pie slice
#define sp_1_pi 0.31830988618379067154 // 1/pi
#define sp_2_pi 0.63661977236758134308 // 2/pi
#define sp_sqrt2 1.41421356237309504880
#define sp_sqrt1_2 0.70710678118654752440 // 1/sqrt(2)

#define sp_inf (1.0 / 0.0)
#define sp_nan (0.0 / 0.0)
#define sp_inf_f (1.0F / 0.0F)
#define sp_nan_f (0.0F / 0.0F)

#define sp_tau 6.28318530717958647692
#define sp_deg2rad (sp_pi / 180.0)
#define sp_rad2deg (180.0 / sp_pi)

#define sp_epsilon 1e-15
#define sp_epsilonf 1e-6F
#define sp_neg_inf (-1.0 / 0.0)
#define sp_neg_inf_f (-1.0F / 0.0F)

#define sp_e 2.71828182845904523536
#define sp_sqrt3 1.73205080756887729352

#define sp_golden_ratio 1.61803398874989484820
#define sp_silver_ratio 2.41421356237309504880
#define sp_bronze_ratio 1.32471795724474602596
#define sp_golden_angle_rad 2.39996322972865332223
#define sp_golden_angle_deg 137.507764050037854646

#define sp_sqrt5 2.23606797749978969640
#define sp_fourth_root_two 1.18920711500272106672
#define sp_sqrt_pi 1.77245385090551602729

#define sp_eps_angle 0.0001
#define sp_unit_xy_angle 0.78539816339744830962 /* 45° */

#define sp_log2_10 3.32192809488736234787
#define sp_log10_2 0.30102999566398119521

#define sp_gamma_half 1.77245385090551602729 /* sqrt(pi) */

#define sp_int32_max 2147483647
#define sp_int32_min (-2147483648)
#define sp_int64_max 9223372036854775807LL
#define sp_int64_min (-9223372036854775808LL)

#define sp_fnv_offset_basis 1469598103934665603ULL
#define sp_fnv_prime 1099511628211ULL
#define sp_murmur_c1 0xcc9e2d51
#define sp_murmur_c2 0x1b873593

#define sp_standard_gravity 9.80665
#define sp_speed_of_light 299792458.0
#define sp_human_reaction_time_ms 250

#define sp_feigenbaum_delta 4.66920160910299067185
#define sp_feigenbaum_alpha 2.50290787509589282228
#define sp_khinchin_constant 2.68545200106530644531
#define sp_apery_constant 1.20205690315959428540
#define sp_euler_mascheroni 0.57721566490153286060
#define sp_catalan_constant 0.91596559417721901505

#define sp_inv_sqrt_pi 0.39894228040143267794
#define sp_gaussian_norm 0.39894228040143267794 /* same thing */

#define sp_circle_area_coeff sp_pi
#define sp_sphere_volume_coeff 4.18879020478639098462	/* 4*pi/3 */
#define sp_sphere_surface_coeff 12.56637061435917295385 /* 4*pi */
#define sp_deg120 2.09439510239319549231				/* 2*pi/3 */
#define sp_deg60 1.04719755119659774615					/* pi/3 */

#define sp_plastic_constant 1.32471795724474602596
#define sp_landau_ramanujan_const 0.76422365358922066299
#define sp_viswanath_constant 1.1319882487943
#define sp_glivenko_cantelli_const 0.409
#define sp_li2_pi2 0.822467033024 /* pi^2/12 */
#define sp_ln_pi 1.14472988584940017414
#define sp_ln_ln_2 (-0.3665129205816643)

#define sp_artin_constant 0.37395581361920228805
#define sp_primorial_e_constant 1.943596436813827

int spel_math_isnan(double x);
int spel_math_isinf(double x);
int spel_math_isfinite(double x);

