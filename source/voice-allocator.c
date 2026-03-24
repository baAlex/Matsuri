/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#include <stddef.h>

#include "voice-allocator.h"

#ifndef FREESTANDING
#include <assert.h>
#else
#define assert(e) // Nothing
#endif


static void* sMemset(void* output, int c, size_t len)
{
	uint8_t* out = (uint8_t*)(output);

	for (; len != 0; len -= 1, out += 1)
		*out = (uint8_t)(c);

	return output;
}

static uint32_t sMin(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}

static uint32_t sXorshift(uint32_t x)
{
	x ^= (uint32_t)((x) << 13);
	x ^= (uint32_t)((x) >> 17);
	x ^= (uint32_t)((x) << 5);
	return x;
}


void VoiceAllocatorSet(struct VoiceAllocator* self, float sampling_frequency, int max_items)
{
	assert(max_items <= MAX_MAX_ITEMS);

	self->sampling_frequency = sampling_frequency;
	self->max_items = (uint32_t)(max_items);

	self->rng = 666;
	self->vel_amp_mod = 1.0f;

	for (int i = 0; i < 5; i += 1)
		self->amplify[i] = 1.0f;

	sMemset(self->voices, 0, sizeof(struct VoiceAllocatorVoice) * (size_t)(max_items));
	sMemset(self->states, 0, sizeof(struct VoiceAllocatorState) * (size_t)(max_items));

	TailSetProgram(sampling_frequency, 10.0f, &self->tail_p);
	TailSetState(&self->tail_s);

	KickSetProgram(sampling_frequency, &self->program.kick);
	SnareSetProgram(sampling_frequency, &self->program.snare);
	HatSetProgram(sampling_frequency, OPEN_HAT, &self->program.open_hat);
	HatSetProgram(sampling_frequency, CLOSED_HAT, &self->program.closed_hat);
	HatSetProgram(sampling_frequency, CYMBAL, &self->program.cymbal);
}


static uint32_t sFindAndSetQueueItem(struct VoiceAllocator* self, enum AllocationStrategy strategy, uint32_t id)
{
	uint32_t item_to_use = 0;

	switch (strategy)
	{
	case STRATEGY_STEAL:
		// We try to find in order:
		// - The oldest stoped item, if available ('remaining' == 0)
		// - An in-use item, the oldest one (smallest 'remaining')

		for (uint32_t i = 0; i < self->max_items; i += 1)
		{
			if (self->voices[i].remaining == 0)
			{
				item_to_use = i;
				break;
			}
			else if (self->voices[i].remaining < self->voices[item_to_use].remaining)
				item_to_use = i;
		}
		break;
	case STRATEGY_CHOKE:
		// As above, except that we stop all items with equal id, and
		// reuse last of them

		for (uint32_t i = 0; i < self->max_items; i += 1)
		{
			if (self->voices[i].id == id)
			{
				TailAccumulate(&self->tail_s, self->states[i].last_signal);
				self->voices[i].remaining = 0;
				item_to_use = i;
			}

			if (self->voices[i].remaining == 0 || //
			    self->voices[i].remaining < self->voices[item_to_use].remaining)
				item_to_use = i;
		}
		break;
	case STRATEGY_COPE:
		for (uint32_t i = 0; i < self->max_items; i += 1)
		{
			if (self->voices[i].remaining == 0)
			{
				item_to_use = i;
				goto found;
			}
		}
		return MAX_MAX_ITEMS; // An invalid index
	found:
		break;
	}

	// Bye!
	self->voices[item_to_use].id = id;
	return item_to_use;
}


void VoiceAllocatorPlay(struct VoiceAllocator* self, enum AllocationStrategy strategy, uint32_t id,
                        enum VoiceAllocatorVoiceType type, float velocity)
{
	const uint32_t item = sFindAndSetQueueItem(self, strategy, id);
	if (item == MAX_MAX_ITEMS)
		return;

	self->rng = sXorshift(self->rng);
	self->states[item].type = type;

	float duration;
	switch (type)
	{
	case TYPE_KICK:
		duration = KickSetState(STATE_START, self->sampling_frequency, velocity, self->vel_amp_mod,
		                        &self->states[item].state.kick);
		break;
	case TYPE_SNARE:
		duration = SnareSetState(STATE_START, self->sampling_frequency, self->rng, velocity, self->vel_amp_mod,
		                         &self->states[item].state.snare);
		break;
	case TYPE_OPEN_HAT:
		duration = HatSetState(STATE_START, self->sampling_frequency, OPEN_HAT, self->rng, velocity, self->vel_amp_mod,
		                       &self->states[item].state.hat);
		break;
	case TYPE_CLOSED_HAT:
		duration = HatSetState(STATE_START, self->sampling_frequency, CLOSED_HAT, self->rng, velocity,
		                       self->vel_amp_mod, &self->states[item].state.hat);
		break;
	case TYPE_CYMBAL:
		duration = HatSetState(STATE_START, self->sampling_frequency, CYMBAL, self->rng, velocity, self->vel_amp_mod,
		                       &self->states[item].state.hat);
	}

	self->voices[item].remaining = (uint32_t)((duration * self->sampling_frequency) / 1000.0f);
}


void VoiceAllocatorStop(struct VoiceAllocator* self, uint32_t id)
{
	// We iterate all items since user may set multiple of them with the same id.
	// And we need to stop all them.

	for (uint32_t item = 0; item < self->max_items; item += 1)
	{
		struct VoiceAllocatorVoice* v = self->voices + item;
		struct VoiceAllocatorState* s = self->states + item;

		if (v->id == id && v->remaining != 0)
		{
			TailAccumulate(&self->tail_s, s->last_signal);
			v->remaining = 0;
		}
	}
}


void VoiceAllocatorConfigureVoice(struct VoiceAllocator* self, enum VoiceAllocatorVoiceType type, float amplify)
{
	self->amplify[(int)(type)] = amplify;
}

void VoiceAllocatorConfigure(struct VoiceAllocator* self, float vel_amp_mod)
{
	self->vel_amp_mod = vel_amp_mod;
}


void VoiceAllocatorRender(struct VoiceAllocator* self, uint32_t samples, float* out)
{
	// Render tails
	for (float* sample = out; sample < out + samples; sample += 1)
		*sample = TailStep(&self->tail_p, &self->tail_s);

	// Render items
	for (uint32_t item = 0; item < self->max_items; item += 1)
	{
		struct VoiceAllocatorVoice* q = self->voices + item;
		struct VoiceAllocatorState* i = self->states + item;

		// Should we render it?
		if (q->remaining == 0)
			continue;

		// Render
		const uint32_t samples_to_render = sMin(q->remaining, samples);

		switch (i->type)
		{
		case TYPE_KICK:
			i->last_signal = RenderAdditiveKick(self->amplify[TYPE_KICK], &self->program.kick, &i->state.kick, out,
			                                    out + samples_to_render);
			break;
		case TYPE_SNARE:
			i->last_signal = RenderAdditiveSnare(self->amplify[TYPE_SNARE], &self->program.snare, &i->state.snare, out,
			                                     out + samples_to_render);
			break;
		case TYPE_OPEN_HAT:
			i->last_signal = RenderAdditiveHat(self->amplify[TYPE_OPEN_HAT], &self->program.open_hat, &i->state.hat,
			                                   out, out + samples_to_render);
			break;
		case TYPE_CLOSED_HAT:
			i->last_signal = RenderAdditiveHat(self->amplify[TYPE_CLOSED_HAT], &self->program.closed_hat, &i->state.hat,
			                                   out, out + samples_to_render);
			break;
		case TYPE_CYMBAL:
			i->last_signal = RenderAdditiveHat(self->amplify[TYPE_CYMBAL], &self->program.cymbal, &i->state.hat, out,
			                                   out + samples_to_render);
		}

		// Update item
		q->remaining -= samples_to_render;
	}
}
