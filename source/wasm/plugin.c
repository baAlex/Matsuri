/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#include <stddef.h>

#include "../matsuri-library.h"


#define BUFFER_LEN (44100 * 10)
static float s_buffer[BUFFER_LEN];

static struct SquareX6Settings s_p;
static struct SquareX6State s_s;

static uint32_t sMin(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}


void Initialise(float sampling_frequency)
{
	SquareX6Initialise(sampling_frequency, 1.0f, 684.35f, 511.97f, 305.88f, 420.2f, 271.14f, 201.23f, &s_p, &s_s);
}

const float* Render(uint32_t samples)
{
	if (samples > BUFFER_LEN)
		return NULL;

	for (float* sample = s_buffer; sample < s_buffer + sMin(BUFFER_LEN, samples); sample += 1)
	{
		*sample = SquareX6Step(&s_p, &s_s);
	}

	return s_buffer;
}
