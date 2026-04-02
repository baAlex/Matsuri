/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#include <stddef.h>

#include "misc.h"
#include "voice-allocator.h"

#ifndef FREESTANDING
#include <assert.h>
#else
#define assert(e) // Nothing
#endif


void VoiceAllocatorSet(struct VoiceAllocator* self, float sampling_frequency, int max_items)
{
	assert(max_items <= MAX_MAX_ITEMS);

	self->sampling_frequency = sampling_frequency;
	self->max_items = (uint32_t)(max_items);

	self->rng = 666;
	self->vel_amp_mod = 1.0f;

	self->amplify[(int)(TYPE_KICK)] = 1.0f;
	self->amplify[(int)(TYPE_SNARE)] = 1.0f;
	self->amplify[(int)(TYPE_OPEN_HAT)] = 1.0f;
	self->amplify[(int)(TYPE_CLOSED_HAT)] = 1.0f;
	self->amplify[(int)(TYPE_CYMBAL)] = 1.0f;
	self->amplify[(int)(TYPE_LOW_TOM)] = 1.0f;
	self->amplify[(int)(TYPE_HIGH_TOM)] = 1.0f;

	Memset(self->voices, 0, sizeof(struct VoiceAllocatorVoice) * (size_t)(max_items));
	Memset(self->states, 0, sizeof(struct VoiceAllocatorState) * (size_t)(max_items));

	TailSetProgram(sampling_frequency, 10.0f, &self->tail_p);
	TailSetState(&self->tail_s);
	self->tail_samples = (uint32_t)((10.0f * sampling_frequency) / 1000.0f);

	KickSetProgram(sampling_frequency, &self->program.kick);
	SnareSetProgram(sampling_frequency, &self->program.snare);
	HatSetProgram(sampling_frequency, OPEN_HAT, &self->program.open_hat);
	HatSetProgram(sampling_frequency, CLOSED_HAT, &self->program.closed_hat);
	HatSetProgram(sampling_frequency, CYMBAL, &self->program.cymbal);
	TomSetProgram(sampling_frequency, LOW_TOM, &self->program.low_tom);
	TomSetProgram(sampling_frequency, HIGH_TOM, &self->program.high_tom);
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

	self->rng = Xorshift(self->rng);
	self->states[item].type = type;

	float duration;
	switch (type)
	{
	case TYPE_KICK:
		duration = KickSetState(STATE_START, self->sampling_frequency, velocity, self->vel_amp_mod, self->vel_tone_mod,
		                        self->reference_vel, &self->states[item].state.kick);
		break;
	case TYPE_SNARE:
		duration = SnareSetState(STATE_START, self->sampling_frequency, self->rng, velocity, self->vel_amp_mod,
		                         self->vel_tone_mod, self->reference_vel, &self->states[item].state.snare);
		break;
	case TYPE_OPEN_HAT:
		duration = HatSetState(STATE_START, self->sampling_frequency, OPEN_HAT, self->rng, velocity, self->vel_amp_mod,
		                       self->vel_tone_mod, self->reference_vel, &self->states[item].state.hat);
		break;
	case TYPE_CLOSED_HAT:
		duration =
		    HatSetState(STATE_START, self->sampling_frequency, CLOSED_HAT, self->rng, velocity, self->vel_amp_mod,
		                self->vel_tone_mod, self->reference_vel, &self->states[item].state.hat);
		break;
	case TYPE_CYMBAL:
		duration = HatSetState(STATE_START, self->sampling_frequency, CYMBAL, self->rng, velocity, self->vel_amp_mod,
		                       self->vel_tone_mod, self->reference_vel, &self->states[item].state.hat);
		break;
	case TYPE_LOW_TOM:
		duration = TomSetState(STATE_START, self->sampling_frequency, LOW_TOM, velocity, self->vel_amp_mod,
		                       self->vel_tone_mod, self->reference_vel, &self->states[item].state.tom);
		break;
	case TYPE_HIGH_TOM:
		duration = TomSetState(STATE_START, self->sampling_frequency, HIGH_TOM, velocity, self->vel_amp_mod,
		                       self->vel_tone_mod, self->reference_vel, &self->states[item].state.tom);
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

void VoiceAllocatorConfigure(struct VoiceAllocator* self, float vel_amp_mod, float vel_tone_mod, float reference_vel)
{
	self->vel_amp_mod = vel_amp_mod;
	self->vel_tone_mod = vel_tone_mod;
	self->reference_vel = reference_vel;
}


void VoiceAllocatorRender(struct VoiceAllocator* self, uint32_t samples, float* out)
{
	// Render tails (and clean)
	{
		float* sample = out;
		for (; sample < out + MinU(self->tail_samples, samples); sample += 1)
			*sample = TailStep(&self->tail_p, &self->tail_s);
		for (; sample < out + samples; sample += 1)
			*sample = 0.0f;
	}

	// Render items
	for (uint32_t item = 0; item < self->max_items; item += 1)
	{
		struct VoiceAllocatorVoice* q = self->voices + item;
		struct VoiceAllocatorState* i = self->states + item;

		// Should we render it?
		if (q->remaining == 0)
			continue;

		// Render
		const uint32_t samples_to_render = MinU(q->remaining, samples);

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
			break;
		case TYPE_LOW_TOM:
			i->last_signal = RenderAdditiveTom(self->amplify[TYPE_LOW_TOM], &self->program.low_tom, &i->state.tom, out,
			                                   out + samples_to_render);
			break;
		case TYPE_HIGH_TOM:
			i->last_signal = RenderAdditiveTom(self->amplify[TYPE_HIGH_TOM], &self->program.high_tom, &i->state.tom,
			                                   out, out + samples_to_render);
		}

		// Update item
		q->remaining -= samples_to_render;
	}
}


void VoiceAllocatorMidi(struct VoiceAllocator* self, int byte0, int byte1, int byte2)
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

				const float vel_float = (float)(MinI(byte2, 127)) / 127.0f;

				switch (byte1)
				{
				default: break;
				case 35: // fallthrough
				case 36: //
					VoiceAllocatorPlay(self, STRATEGY_CHOKE, 1, TYPE_KICK, vel_float);
					break;
				case 38: // fallthrough
				case 40: //
					VoiceAllocatorPlay(self, STRATEGY_CHOKE, 2, TYPE_SNARE, vel_float);
					break;
				case 42: //
					VoiceAllocatorPlay(self, STRATEGY_CHOKE, 3, TYPE_CLOSED_HAT, vel_float);
					break;
				case 46: //
					VoiceAllocatorPlay(self, STRATEGY_CHOKE, 3, TYPE_OPEN_HAT, vel_float);
					break;
				case 49: //
					VoiceAllocatorPlay(self, STRATEGY_STEAL, 4, TYPE_CYMBAL, vel_float);
					break;
				case 41: // fallthrough
				case 43: // fallthrough
				case 45: //
					VoiceAllocatorPlay(self, STRATEGY_STEAL, 5, TYPE_LOW_TOM, vel_float);
					break;
				case 47: // fallthrough
				case 48: // fallthrough
				case 50: //
					VoiceAllocatorPlay(self, STRATEGY_STEAL, 6, TYPE_HIGH_TOM, vel_float);
					break;
				}
			}
		}
	}
}
