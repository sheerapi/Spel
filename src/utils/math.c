#include "utils/math.h"
#include <math.h>

sp_color_define(black, 0, 0, 0);
sp_color_define(white, 255, 255, 255);

sp_color_define(red, 255, 0, 0);
sp_color_define(green, 0, 255, 0);
sp_color_define(blue, 0, 0, 255);

sp_color_define(cyan, 0, 255, 255);
sp_color_define(yellow, 255, 255, 0);
sp_color_define(magenta, 255, 0, 255);

sp_api spel_color spel_color_hsv(int h, float s, float v, uint8_t a)
{
	float C = v * s;
	float Hn = (float)h / 60.0F;
	float X = C * (1.0F - fabsf(fmodf(Hn, 2.0F) - 1.0F));
	float r = 0;
	float g = 0;
	float b = 0;

	if (0 <= Hn && Hn < 1)
	{
		r = C;
		g = X;
		b = 0;
	}
	else if (1 <= Hn && Hn < 2)
	{
		r = X;
		g = C;
		b = 0;
	}
	else if (2 <= Hn && Hn < 3)
	{
		r = 0;
		g = C;
		b = X;
	}
	else if (3 <= Hn && Hn < 4)
	{
		r = 0;
		g = X;
		b = C;
	}
	else if (4 <= Hn && Hn < 5)
	{
		r = X;
		g = 0;
		b = C;
	}
	else
	{
		r = C;
		g = 0;
		b = X;
	}

	float m = v - C;
	r += m;
	g += m;
	b += m;

	return (spel_color){(uint8_t)(r * 255.0F), (uint8_t)(g * 255.0F), (uint8_t)(b * 255.0F),
				   a};
}

sp_api spel_color spel_color_hsl(int h, float s, float l, uint8_t a)
{
	h = (int)fmodf((float)h, 360.0F);
	if (h < 0)
	{
		h += 360;
	}

	float C = (1.0F - fabsf((2.0F * l) - 1.0F)) * s;
	float Hn = (float)h / 60.0F;
	float X = C * (1.0F - fabsf(fmodf(Hn, 2.0F) - 1.0F));
	float r = 0;
	float g = 0;
	float b = 0;

	if (0 <= Hn && Hn < 1)
	{
		r = C;
		g = X;
		b = 0;
	}
	else if (1 <= Hn && Hn < 2)
	{
		r = X;
		g = C;
		b = 0;
	}
	else if (2 <= Hn && Hn < 3)
	{
		r = 0;
		g = C;
		b = X;
	}
	else if (3 <= Hn && Hn < 4)
	{
		r = 0;
		g = X;
		b = C;
	}
	else if (4 <= Hn && Hn < 5)
	{
		r = X;
		g = 0;
		b = C;
	}
	else
	{
		r = C;
		g = 0;
		b = X;
	}

	float m = l - (C * 0.5F);
	r += m;
	g += m;
	b += m;

	return (spel_color){(uint8_t)(r * 255.0F), (uint8_t)(g * 255.0F), (uint8_t)(b * 255.0F),
				   a};
}

sp_api spel_color spel_color_mix(spel_color a, spel_color b, float t)
{
	int ti = (int)(t * 255.0F);

	return (spel_color){(uint8_t)((a.r * (255 - ti) + b.r * ti) >> 8),
				   (uint8_t)((a.g * (255 - ti) + b.g * ti) >> 8),
				   (uint8_t)((a.b * (255 - ti) + b.b * ti) >> 8),
				   (uint8_t)((a.a * (255 - ti) + b.a * ti) >> 8)};
}

sp_api spel_color spel_color_invert(spel_color value)
{
	return (spel_color){255 - value.r, 255 - value.g, 255 - value.b,
				   value.a == 255 ? 255 : (255 - value.a)};
}

typedef union
{
	double d;
	unsigned long long u;
} spel_math_double_bits;

typedef union
{
	float f;
	unsigned int u;
} spel_math_float_bits;

double spel_math_fabs(double x)
{
	spel_math_double_bits bits = {.d = x};
	bits.u &= ~(1ULL << 63);
	return bits.d;
}

float spel_math_fabsf(float x)
{
	spel_math_float_bits bits = {.f = x};
	bits.u &= ~(1U << 31);
	return bits.f;
}

double spel_math_copysign(double x, double y)
{
	spel_math_double_bits xb = {.d = x};
	spel_math_double_bits yb = {.d = y};
	xb.u = (xb.u & ~(1ULL << 63)) | (yb.u & (1ULL << 63));
	return xb.d;
}

int spel_math_isnan(double x)
{
	spel_math_double_bits bits = {.d = x};
	unsigned long long exp = (bits.u >> 52) & 0x7FF;
	unsigned long long man = bits.u & 0xFFFFFFFFFFFFFULL;
	return exp == 0x7FF && man != 0;
}

int spel_math_isinf(double x)
{
	spel_math_double_bits bits = {.d = x};
	return ((bits.u & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL);
}

int spel_math_isfinite(double x)
{
	spel_math_double_bits bits = {.d = x};
	return ((bits.u >> 52) & 0x7FF) != 0x7FF;
}