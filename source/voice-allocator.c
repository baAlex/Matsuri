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


void VoiceAllocatorSet(struct VoiceAllocator* self, float sampling_frequency, int max_items)
{
	assert(max_items <= MAX_MAX_ITEMS);

	self->sampling_frequency = sampling_frequency;
	self->max_items = (uint32_t)(max_items);

	sMemset(self->voices, 0, sizeof(struct VoiceAllocatorVoice) * (size_t)(max_items));
	sMemset(self->states, 0, sizeof(struct VoiceAllocatorState) * (size_t)(max_items));

	TailSetProgram(sampling_frequency, 10.0f, &self->tail_p);
	TailSetState(&self->tail_s);

	KickSetProgram(sampling_frequency, &self->program.kick);
	SnareSetProgram(sampling_frequency, &self->program.snare);
	HatSetProgram(sampling_frequency, OPEN_HAT, &self->program.open_hat);
	HatSetProgram(sampling_frequency, CLOSED_HAT, &self->program.closed_hat);
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
                        enum VoiceAllocatorVoiceType type)
{
	const uint32_t item = sFindAndSetQueueItem(self, strategy, id);
	if (item == MAX_MAX_ITEMS)
		return;

	self->states[item].type = type;

	switch (type)
	{
	case TYPE_KICK:
		KickSetState(STATE_START, self->sampling_frequency, &self->states[item].state.kick);
		self->voices[item].remaining = (uint32_t)((KickDuration() * self->sampling_frequency) / 1000.0f);
		break;
	case TYPE_SNARE:
		SnareSetState(STATE_START, self->sampling_frequency, &self->states[item].state.snare);
		self->voices[item].remaining = (uint32_t)((SnareDuration() * self->sampling_frequency) / 1000.0f);
		break;
	case TYPE_OPEN_HAT:
		HatSetState(STATE_START, &self->states[item].state.hat);
		self->voices[item].remaining = (uint32_t)((HatDuration(OPEN_HAT) * self->sampling_frequency) / 1000.0f);
		break;
	case TYPE_CLOSED_HAT:
		HatSetState(STATE_START, &self->states[item].state.hat);
		self->voices[item].remaining = (uint32_t)((HatDuration(CLOSED_HAT) * self->sampling_frequency) / 1000.0f);
	}
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


void VoiceAllocatorRender(struct VoiceAllocator* self, float amplify, uint32_t samples, float* out)
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
		// TODO, hardcoded volumes
		const uint32_t samples_to_render = sMin(q->remaining, samples);

		switch (i->type)
		{
		case TYPE_KICK:
			i->last_signal =
			    RenderAdditiveKick(amplify * 1.0f, &self->program.kick, &i->state.kick, out, out + samples_to_render);
			break;
		case TYPE_SNARE:
			i->last_signal = RenderAdditiveSnare(amplify * 1.0f, &self->program.snare, &i->state.snare, out,
			                                     out + samples_to_render);
			break;
		case TYPE_OPEN_HAT:
			i->last_signal =
			    RenderAdditiveHat(amplify * 0.4f, &self->program.open_hat, &i->state.hat, out, out + samples_to_render);
			break;
		case TYPE_CLOSED_HAT:
			i->last_signal = RenderAdditiveHat(amplify * 0.2f, &self->program.closed_hat, &i->state.hat, out,
			                                   out + samples_to_render);
		}

		// Update item
		q->remaining -= samples_to_render;
	}
}
