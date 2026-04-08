/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#include <stddef.h>

#include "../misc.h"
#include "../version.h"
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


const float* Render(float volume, float kick_volume, float snare_volume, float closed_hat_volume, float open_hat_volume,
                    float cymbal_volume, float low_tom_volume, float high_tom_volume, float vel_vol_mod,
                    float vel_tone_mod, float reference_velocity, uint32_t samples)
{
	if (samples > BUFFER_LEN)
		return NULL;

	VoiceAllocatorConfigure(&s_allocator, vel_vol_mod, vel_tone_mod, reference_velocity);

	volume = ExponentialVolumeEasing(volume);
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_KICK, volume * ExponentialVolumeEasing(kick_volume));
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_SNARE, volume * ExponentialVolumeEasing(snare_volume));
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_CLOSED_HAT, volume * ExponentialVolumeEasing(closed_hat_volume));
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_OPEN_HAT, volume * ExponentialVolumeEasing(open_hat_volume));
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_CYMBAL, volume * ExponentialVolumeEasing(cymbal_volume));
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_LOW_TOM, volume * ExponentialVolumeEasing(low_tom_volume));
	VoiceAllocatorConfigureVoice(&s_allocator, TYPE_HIGH_TOM, volume * ExponentialVolumeEasing(high_tom_volume));

	VoiceAllocatorRender(&s_allocator, samples, s_buffer);

	return s_buffer;
}

// clang-format off
const char* GetId(void )            { return MATSURI_URI; }
const char* GetName(void )          { return MATSURI_NAME; }
const char* GetVendor(void )        { return MATSURI_VENDOR; }
const char* GetUrl(void )           { return MATSURI_URL; }
const char* GetDescription(void )   { return MATSURI_DESCRIPTION; }
const char* GetVersionString(void ) { return MATSURI_VERSION_STRING; }
int GetVersionMajor(void )          { return MATSURI_VERSION_MAJOR; }
int GetVersionMinor(void )          { return MATSURI_VERSION_MINOR; }
const char* GetCopyright(void )     { return MATSURI_COPYRIGHT; }
// clang-format off
