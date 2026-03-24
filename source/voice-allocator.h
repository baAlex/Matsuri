/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#ifndef VOICE_ALLOCATOR_H
#define VOICE_ALLOCATOR_H

#include "matsuri-library.h"


#define MAX_MAX_ITEMS 16


struct VoiceAllocatorVoice // Should be tiny in size, we iterate it a lot
{
	uint32_t id;
	uint32_t remaining; // In samples
};

enum VoiceAllocatorVoiceType
{
	// A bit hardcoded, but, isn't like I'm making new instruments every day
	TYPE_KICK = 0,
	TYPE_SNARE,
	TYPE_OPEN_HAT,
	TYPE_CLOSED_HAT,
	TYPE_CYMBAL
};

struct VoiceAllocatorState // Big as instruments require it, we don't access this frequently
{
	enum VoiceAllocatorVoiceType type;
	union
	{
		struct KickState kick;
		struct SnareState snare;
		struct HatState hat;
	} state;

	float last_signal;
};

struct VoiceAllocator
{
	float sampling_frequency;
	uint32_t max_items;

	uint32_t rng;
	float vel_amp_mod;
	float amplify[5]; // One for each VoiceAllocatorVoiceType

	struct VoiceAllocatorVoice voices[MAX_MAX_ITEMS];
	struct VoiceAllocatorState states[MAX_MAX_ITEMS];

	struct TailProgram tail_p;
	struct TailState tail_s;

	struct
	{
		// Programs are shared between voices of the same instrument
		struct KickProgram kick;
		struct SnareProgram snare;
		struct HatProgram open_hat;
		struct HatProgram closed_hat;
		struct HatProgram cymbal;
	} program;
};

enum AllocationStrategy
{
	STRATEGY_STEAL, // Uses oldest stopped voice, if not available, stops and reuses the oldest one still playing

	STRATEGY_CHOKE, // Stops all voices with same id, reusing one of them, if not available, it fallbacks to steal
	                // strategy. This make ids to behave as choke groups.

	STRATEGY_COPE, // Only uses stopped voices, if not available, doesn't play anything new.
};

void VoiceAllocatorSet(struct VoiceAllocator* allocator, float sampling_frequency, int max_items);
void VoiceAllocatorPlay(struct VoiceAllocator* allocator, enum AllocationStrategy, uint32_t id,
                        enum VoiceAllocatorVoiceType type, float velocity);
void VoiceAllocatorConfigureVoice(struct VoiceAllocator* allocator, enum VoiceAllocatorVoiceType type, float amplify);
void VoiceAllocatorConfigure(struct VoiceAllocator* allocator, float vel_amp_mod);
void VoiceAllocatorStop(struct VoiceAllocator* allocator, uint32_t id);
void VoiceAllocatorRender(struct VoiceAllocator* allocator, uint32_t samples, float* out);

#endif
