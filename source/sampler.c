/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "sampler.h"
#include <stddef.h>


#define SAMPLER_MASK 16777215
#define SAMPLER_MAX 16777216.0f
#define SAMPLER_INV 0.000000059604645f // 1.0 / SAMPLER_MAX
#define SAMPLER_SHIFT 24


static size_t sMinZU(size_t a, size_t b)
{
	return (a < b) ? a : b;
}


#define LINEAR_INTERPOLATION

float SamplerSetState(enum SamplerStateState state_state, float sampling_frequency, float sample_frequency,
                      const float* sample_start, const float* sample_end, struct SamplerState* s)
{
	const size_t len = (size_t)(sample_end - sample_start);

	switch (state_state)
	{
	case SAMPLER_STATE_START:
		s->decimal = 0;
		s->step = (uint32_t)((sample_frequency / sampling_frequency) * SAMPLER_MAX);
		s->cursor = sample_start;

		// Hacky, it saves one multiplication per Render() call, also, the
		// less one is because interpolation always reads one extra sample:
#ifdef LINEAR_INTERPOLATION
		s->end = sample_start + (uint32_t)((float)(len - 1) * (sampling_frequency / sample_frequency));
#else
		s->end = sample_start + (uint32_t)((float)(len) * (sampling_frequency / sample_frequency));
#endif
		break;
	case SAMPLER_STATE_DEAD:
		s->cursor = NULL; //
		s->end = NULL;
	}

	return ((float)(len) * 1000.0f) / sample_frequency;
}


float SamplerRenderAdditive(struct SamplerState* s, float* out, const float* out_end)
{
	const float* end = out + sMinZU((size_t)(s->end - s->cursor), (size_t)(out_end - out));
	float signal = 0.0f;

#ifdef LINEAR_INTERPOLATION
	if (s->step < SAMPLER_MASK)
	{
		for (; out < end; out += 1)
		{
			s->decimal += s->step;
			s->cursor += s->decimal >> SAMPLER_SHIFT;
			s->decimal &= SAMPLER_MASK;

			signal = s->cursor[0] + (s->cursor[1] - s->cursor[0]) * (float)(s->decimal) * SAMPLER_INV;
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
			s->decimal &= SAMPLER_MASK;

			signal = s->cursor[0];
			*out += signal;
		}
	}

	return signal;
}
