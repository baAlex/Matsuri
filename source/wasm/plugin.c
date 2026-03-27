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

static int sMin(int a, int b)
{
	return (a < b) ? a : b;
}


const float* Initialise(float sampling_frequency)
{
	VoiceAllocatorSet(&s_allocator, sampling_frequency, MAX_MAX_ITEMS);
	return s_buffer;
}


void Midi(int byte0, int byte1, int byte2)
{
	// Per MIDI protocol, any error is silently ignored

	byte0 &= 0xFF; // We accept integers because is friendly with WASM,
	byte1 &= 0xFF; // but we don't want any higher bit
	byte2 &= 0xFF;

	// "Note Off" message
	if ((byte0 >> 4) == 8)
	{
		// Ignoring these
	}

	// "Note On" message
	else if ((byte0 >> 4) == 9) // Ignoring channel
	{
		if (byte1 < 128) // Valid note range
		{
			if (byte2 == 0)
			{
				// Velocity = 0 = Implicit "Note Off" -> Ignoring it
			}
			else
			{
				// Finally "Note On":

				// General MIDI mappings
				// https://upload.wikimedia.org/wikipedia/commons/c/c2/GM_Standard_Drum_Map_on_the_keyboard.svg

				const float vel_float = (float)(sMin(byte2, 127)) / 127.0f;

				switch (byte1)
				{
				default: break;
				case 35: // fallthrough
				case 36: //
					VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 1, TYPE_KICK, vel_float);
					break;
				case 38: // fallthrough
				case 40: //
					VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 2, TYPE_SNARE, vel_float);
					break;
				case 42: //
					VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 3, TYPE_CLOSED_HAT, vel_float);
					break;
				case 46: //
					VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 3, TYPE_OPEN_HAT, vel_float);
					break;
				case 49: //
					VoiceAllocatorPlay(&s_allocator, STRATEGY_STEAL, 4, TYPE_CYMBAL, vel_float);
					break;
				case 41: // fallthrough
				case 43: // fallthrough
				case 45: //
					VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 5, TYPE_LOW_TOM, vel_float);
					break;
				case 47: // fallthrough
				case 48: // fallthrough
				case 50: //
					VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 6, TYPE_HIGH_TOM, vel_float);
					break;
				}
			}
		}
	}
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
