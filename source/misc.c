/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "misc.h"


void* Memset(void* output, int c, size_t len)
{
	uint8_t* out = (uint8_t*)(output);

	for (; len != 0; len -= 1, out += 1)
		*out = (uint8_t)(c);

	return output;
}


int MinI(int a, int b)
{
	return (a < b) ? a : b;
}

uint32_t MinU(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}

float MinF(float a, float b)
{
	return (a < b) ? a : b;
}

float MaxF(float a, float b)
{
	return (a > b) ? a : b;
}


uint32_t Xorshift(uint32_t x)
{
	x ^= (uint32_t)((x) << 13);
	x ^= (uint32_t)((x) >> 17);
	x ^= (uint32_t)((x) << 5);
	return x;
}


#define TABLE_LEN 64

// y = (expf(3.45f * fabsf(x)) - 1.0f) / (expf(3.45f) - 1.0f)
static const float s_table[TABLE_LEN + 1] = {
    0.000000f, 0.001810f, 0.003721f, 0.005737f, 0.007865f, 0.010111f, 0.012482f, 0.014985f, 0.017626f, 0.020413f,
    0.023355f, 0.026461f, 0.029738f, 0.033197f, 0.036848f, 0.040702f, 0.044769f, 0.049062f, 0.053593f, 0.058375f,
    0.063423f, 0.068750f, 0.074373f, 0.080308f, 0.086571f, 0.093183f, 0.100160f, 0.107525f, 0.115298f, 0.123503f,
    0.132162f, 0.141301f, 0.150948f, 0.161129f, 0.171875f, 0.183217f, 0.195187f, 0.207822f, 0.221158f, 0.235233f,
    0.250088f, 0.265768f, 0.282317f, 0.299783f, 0.318219f, 0.337677f, 0.358213f, 0.379889f, 0.402767f, 0.426914f,
    0.452400f, 0.479299f, 0.507690f, 0.537655f, 0.569282f, 0.602664f, 0.637896f, 0.675082f, 0.714331f, 0.755757f,
    0.799479f, 0.845627f, 0.894333f, 0.945741f, 1.000000f};

float ExponentialVolumeEasing(float x)
{
	x = MaxF(0.0f, MinF(1.0f, x));

	const int i = (int)(x * (float)(TABLE_LEN));
	const float f = (x * (float)(TABLE_LEN)) - (float)(i);

	return s_table[i] + (s_table[i + 1] - s_table[i]) * f;
}
