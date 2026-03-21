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


void KeyOn(int key_no)
{
	// General MIDI mappings
	// https://upload.wikimedia.org/wikipedia/commons/c/c2/GM_Standard_Drum_Map_on_the_keyboard.svg

	switch (key_no)
	{
	default: break;
	case 35: // fallthrough
	case 36: //
		VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 1, TYPE_KICK);
		break;
	case 38: // fallthrough
	case 40: //
		VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 2, TYPE_SNARE);
		break;
	case 42: //
		VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 3, TYPE_CLOSED_HAT);
		break;
	case 46: //
		VoiceAllocatorPlay(&s_allocator, STRATEGY_CHOKE, 3, TYPE_OPEN_HAT);
		break;
	}
}


const float* Render(float amplify, uint32_t samples)
{
	if (samples > BUFFER_LEN)
		return NULL;

	VoiceAllocatorRender(&s_allocator, amplify, samples, s_buffer);

	return s_buffer;
}
