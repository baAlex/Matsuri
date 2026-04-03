/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
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
    0.000000f, 0.001840f, 0.003783f, 0.005836f, 0.008004f, 0.010295f, 0.012715f, 0.015271f, 0.017971f, 0.020824f,
    0.023837f, 0.027020f, 0.030383f, 0.033935f, 0.037687f, 0.041651f, 0.045838f, 0.050261f, 0.054933f, 0.059869f,
    0.065083f, 0.070591f, 0.076409f, 0.082555f, 0.089048f, 0.095907f, 0.103152f, 0.110805f, 0.118890f, 0.127430f,
    0.136452f, 0.145983f, 0.156050f, 0.166685f, 0.177920f, 0.189787f, 0.202324f, 0.215567f, 0.229556f, 0.244334f,
    0.259945f, 0.276436f, 0.293856f, 0.312258f, 0.331697f, 0.352232f, 0.373924f, 0.396839f, 0.421046f, 0.446617f,
    0.473629f, 0.502163f, 0.532306f, 0.564147f, 0.597784f, 0.633316f, 0.670851f, 0.710501f, 0.752387f, 0.796632f,
    0.843372f, 0.892746f, 0.944903f, 1.000000f, 1.000000f};

float ExponentialVolumeEasing(float x)
{
	x = MaxF(0.0f, MinF(1.0f, x));

	const int i = (int)(x * (float)(TABLE_LEN));
	const float f = (x * (float)(TABLE_LEN)) - (float)(i);

	return s_table[i] + (s_table[i + 1] - s_table[i]) * f;
}
