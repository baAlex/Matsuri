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

static float s_sampling_frequency;
static struct KickProgram s_kick_p;
static struct KickState s_kick_s;
static struct SnareProgram s_snare_p;
static struct SnareState s_snare_s;
static struct HatProgram s_closed_hat_p;
static struct HatState s_closed_hat_s;
static struct HatProgram s_open_hat_p;
static struct HatState s_open_hat_s;


static uint32_t sMin(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}


const float* Initialise(float sampling_frequency)
{
	s_sampling_frequency = sampling_frequency;

	KickInitialise(sampling_frequency, &s_kick_p, &s_kick_s);
	SnareInitialise(sampling_frequency, &s_snare_p, &s_snare_s);
	HatInitialise(sampling_frequency, CLOSED_HAT, &s_closed_hat_p, &s_closed_hat_s);
	HatInitialise(sampling_frequency, OPEN_HAT, &s_open_hat_p, &s_open_hat_s);

	return s_buffer;
}

const float* KeyOn(int key_no)
{
	switch (key_no)
	{
	default: KickInitialiseState(s_sampling_frequency, &s_kick_s); break;
	case 1: SnareInitialiseState(s_sampling_frequency, &s_snare_s); break;
	case 2: HatInitialiseState(&s_closed_hat_s); break;
	case 3: HatInitialiseState(&s_open_hat_s); break;
	}

	return s_buffer;
}

const float* Render(uint32_t samples)
{
	if (samples > BUFFER_LEN)
		return NULL;

	RenderKick(&s_kick_p, &s_kick_s, s_buffer, s_buffer + sMin(BUFFER_LEN, samples));
	RenderAdditiveSnare(&s_snare_p, &s_snare_s, s_buffer, s_buffer + sMin(BUFFER_LEN, samples));
	RenderAdditiveHat(&s_closed_hat_p, &s_closed_hat_s, s_buffer, s_buffer + sMin(BUFFER_LEN, samples));
	RenderAdditiveHat(&s_open_hat_p, &s_open_hat_s, s_buffer, s_buffer + sMin(BUFFER_LEN, samples));

	return s_buffer;
}
