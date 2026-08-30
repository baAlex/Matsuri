/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "sampler.h"


#define SAMPLER_SHIFT 24
#define SAMPLER_MAX ((1 << SAMPLER_SHIFT) - 1)
#define SAMPLER_INV (1.0f / (float)(SAMPLER_MAX))


static size_t sMinZU(size_t a, size_t b)
{
	return (a < b) ? a : b;
}


#define INTERPOLATION 2 // 0 = Nearest, 1 = Linear, 2 = Cubic

// With linear interpolation current code doesn't interpolate from last sample to zero, it
// simply ends sound there, so an extra zero sample is required at the end. But since most
// proper samples end in zero already (otherwise they produce a click), it doesn't matter
// that much.

// Same problem with cubic interpolation, but this time two extra zero samples are required
// at the end, and one at the start. The one at the start is really important, it will click
// if absent.

// Those problems are not bugs, neither hard to fix, is simply to save code and a bit of CPU
// juice. Samples must be well created/curated anyways.

float SamplerSetState(enum SamplerStateState state_state, float sampling_frequency, float sample_frequency,
                      const float* sample_start, size_t sample_len, struct SamplerState* s)
{
	switch (state_state)
	{
	case SAMPLER_STATE_START:
		s->decimal = 0;
		s->cursor = sample_start;

		// Using 'uint64_t' because WASM32 seems to have a 'size_t' of 32 bits
		// (TODO, move operations around so it fits on a 32bits register)
		s->step = (uint32_t)(((uint64_t)(sample_frequency) * (uint64_t)(SAMPLER_MAX)) / (uint64_t)(sampling_frequency));

#if INTERPOLATION == 0
		s->end = sample_start + sample_len;
		break;
#elif INTERPOLATION == 1
		s->remaining = (size_t)(((sample_len - 1) * (uint64_t)(sampling_frequency)) / (uint64_t)(sample_frequency));
		break;
#elif INTERPOLATION == 2
		s->remaining = (size_t)(((sample_len - 3) * (uint64_t)(sampling_frequency)) / (uint64_t)(sample_frequency));
		break;
#endif
	case SAMPLER_STATE_DEAD: s->remaining = 0;
	}

	return ((float)(sample_len) * 1000.0f) / sample_frequency;
}


#if INTERPOLATION == 2
static float sCubic(float xm1, float x0, float x1, float x2, float f)
{
	// TODO, check generated assembly, one form should be better than the others
	// (and there is many more in musicdsp.org)

#if 0
	// https://www.musicdsp.org/en/latest/Other/49-cubic-interpollation.html
	const float a = (3.0f * (x0 - x1) - xm1 + x2) / 2.0f;
	const float b = 2.0f * x1 + xm1 - (5.0f * x0 + x2) / 2.0f;
	const float c = (x1 - xm1) / 2.0f;
	return (((a * f) + b) * f + c) * f + x0;
#endif
#if 0
	// https://www.musicdsp.org/en/latest/Other/93-hermite-interpollation.html
	// 4-point, 3rd-order Hermite (x-form)
	const float c0 = x0;
	const float c1 = 0.5f * (x1 - xm1);
	const float c2 = xm1 - 2.5f * x0 + 2.f * x1 - 0.5f * x2;
	const float c3 = 1.5f * (x0 - x1) + 0.5f * (x2 - xm1);
	return ((c3 * f + c2) * f + c1) * f + c0;
#endif
#if 1
	// https://www.musicdsp.org/en/latest/Other/93-hermite-interpollation.html
	// 4-point, 3rd-order Hermite (x-form)
	const float c1 = 0.5f * (x1 - xm1);
	const float y0my1 = xm1 - x0;
	const float c3 = (x0 - x1) + 0.5f * (x2 - y0my1 - x1);
	const float c2 = y0my1 + c1 - c3;
	return ((c3 * f + c2) * f + c1) * f + x0;
#endif
}
#endif


float SamplerRenderAdditive(struct SamplerState* s, float* out, const float* out_end)
{
	const float* end = out + sMinZU(s->remaining, (size_t)(out_end - out));
	float signal = 0.0f;

#if INTERPOLATION > 0
	if (s->step < SAMPLER_MAX)
	{
		for (; out < end; out += 1)
		{
			s->decimal += s->step;
			s->cursor += s->decimal >> SAMPLER_SHIFT;
			s->decimal &= SAMPLER_MAX;
			s->remaining -= 1;

#if INTERPOLATION == 1
			signal = s->cursor[0] + (s->cursor[1] - s->cursor[0]) * (float)(s->decimal) * SAMPLER_INV;
#elif INTERPOLATION == 2
			signal = sCubic(s->cursor[0], s->cursor[1], s->cursor[2], s->cursor[3], (float)(s->decimal) * SAMPLER_INV);
#endif

			*out += signal;
		}
	}
	else
#endif
	{
		for (; out < end; out += 1)
		{
			s->decimal += s->step;
			s->cursor += s->decimal >> SAMPLER_SHIFT;
			s->decimal &= SAMPLER_MAX;
			s->remaining -= 1;

			signal = s->cursor[0];
			*out += signal;
		}
	}

	return signal;
}
