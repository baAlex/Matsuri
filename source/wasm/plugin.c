/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#include <stddef.h>

#include "../voice-allocator.h"


#define BUFFER_LEN (96000 * 5)
static float s_buffer[BUFFER_LEN];
static struct VoiceAllocator s_allocator;


const float* Initialise(float sampling_frequency)
{
	VoiceAllocatorSet(&s_allocator, sampling_frequency, MAX_MAX_ITEMS);
	return s_buffer;
}


void Midi(int byte0, int byte1, int byte2)
{
	VoiceAllocatorMidi(&s_allocator, byte0, byte1, byte2);
}


const float* Render(float amplify, float kick_amplify, float snare_amplify, float closed_hat_amplify,
                    float open_hat_amplify, float cymbal_amplify, float low_tom_amplify, float high_tom_amplify,
                    float vel_amp_mod, float vel_tone_mod, float reference_velocity, uint32_t samples)
{
	if (samples > BUFFER_LEN)
		return NULL;

	VoiceAllocatorConfigure(&s_allocator, vel_amp_mod, vel_tone_mod, reference_velocity);

	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_KICK, amplify * kick_amplify);
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_SNARE, amplify * snare_amplify);
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_CLOSED_HAT, amplify * closed_hat_amplify);
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_OPEN_HAT, amplify * open_hat_amplify);
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_CYMBAL, amplify * cymbal_amplify);
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_LOW_TOM, amplify * low_tom_amplify);
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_HIGH_TOM, amplify * high_tom_amplify);

	VoiceAllocatorRender(&s_allocator, samples, s_buffer);

	return s_buffer;
}
