/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef SAMPLER_H
#define SAMPLER_H

#include <stdint.h>

enum SamplerStateState
{
	SAMPLER_STATE_START,
	SAMPLER_STATE_DEAD
};

struct SamplerState
{
	const float* cursor;
	const float* end;
	uint32_t decimal;
	uint32_t step;
};

float SamplerSetState(enum SamplerStateState state_state, float sampling_frequency, float sample_frequency,
                      const float* sample_start, const float* sample_end, struct SamplerState* s);
float SamplerRenderAdditive(struct SamplerState* s, float* out, const float* out_end);

#endif
