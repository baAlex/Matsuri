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
