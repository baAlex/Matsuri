/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#ifndef FREESTANDING
#include <assert.h>
#include <stdio.h>
#else
#define assert(e) // Nothing
#endif

#ifndef GODBOLT
#include "matsuri-library.h"
#endif

#ifdef NDEBUG
#pragma message("Doing a release build, be sure of check all magic numbers")
#endif


#define PI_TWO 6.28318530718f
#define HALF_PI 1.57079632679f
#define LOG_60DB -6.90775527898f        // ln(0.001)
#define LOG_100_PERCENT -4.60517018599f // ln(0.01) = ln(1.0 / 100.0)
#define LOG_2_SEMITONE 0.05776226504f   // ln(2) / 12
#define NOISE_SCALE 1.1920929e-7f       // 1 / ((2 ^ 24) / 2)


float OscillatorStep(const struct OscillatorProgram* restrict p, struct OscillatorState* restrict s)
{
	const int go = (s->delay < 0) ? 1 : 0;

	s->v -= (float)(go) * (s->v * p->zeta + s->x * s->omega * s->sweep);
	s->x += (float)(go) * (s->v * s->sweep);

	s->sweep += (p->sweep_target - s->sweep) * p->sweep_step * (float)(go);
	s->delay -= 1 - go;

	return s->x;
}


float EnvelopeStep(const struct EnvelopeProgram* restrict p, struct EnvelopeState* restrict s)
{
	const uint32_t stage = s->stage; // Compiler wants these to be loaded in a
	const uint32_t x = s->x;         // register in order to emit a CMOV

	{
		const uint32_t done = (x + 1 >= (uint32_t)(p->durations[stage])) & (stage < 3);
		const uint32_t mask = -done;

		s->x = (x + 1) & (~mask);
		s->stage = (uint8_t)(stage + done);
	}

	// Duration needs to be loaded again, as may have been updated in previous
	// step, that's is also the reason on why I'm not keeping it around
	{
		const float target_y = p->levels[stage];
		const float duration = p->durations[stage];

		if (1)
			s->y += (target_y - s->y) * ((-LOG_100_PERCENT) / duration);
		else
			s->y += (target_y - s->y) * 1.0f; // Developer purposes
	}

	// Bye!
	return s->y;
}


static float sAbs(float x)
{
	return (x < 0.0f) ? -x : x;
}

static float sEasing(float x, float f)
{
	assert(f > -0.999f && f < 0.999f);
	return (x - f * x) / (f - 2.0f * f * sAbs(x) + 1.0f);
}

float ShapedEnvelopeStep(const struct ShapedEnvelopeProgram* restrict p, struct ShapedEnvelopeState* restrict s)
{
	const uint32_t stage = s->stage;
	const uint32_t x = s->x;

	const uint32_t done = (x + 1 >= p->durations[stage]) & (stage < 3);
	const uint32_t mask = -done;

	s->x = (x + 1) & (~mask);
	s->stage = (uint8_t)(stage + done);

	//
	s->y += p->steps[stage];
	return sEasing(s->y, p->shapes[(stage - 1) & 1]);
}


static uint32_t sXorshift(uint32_t x)
{
	// https://en.wikipedia.org/wiki/Xorshift#Example_implementation
	x ^= (uint32_t)((x) << 13);
	x ^= (uint32_t)((x) >> 17);
	x ^= (uint32_t)((x) << 5);
	return x;
}

float NoiseStep(struct NoiseState* restrict s)
{
	s->x = sXorshift(s->x);
	return (float)((s->x) >> 8) * NOISE_SCALE - 1.0f;
}


#define X1 0 // To use as indices
#define Y1 1
#define X2 2
#define Y2 3

#define B1 X1 // Arranged to make Simd easier
#define A1 Y1
#define B2 X2
#define A2 Y2

float FilterStep(float x, const struct FilterProgram* restrict p, struct FilterState* restrict s)
{
	const float y = (p->c_b0 * x)                                   //
	                + (p->c[B1] * s->s[X1]) + (p->c[B2] * s->s[X2]) // [a]
	                + (p->c[A1] * s->s[Y1]) + (p->c[A2] * s->s[Y2]);

	s->s[Y2] = s->s[Y1];
	s->s[X2] = s->s[X1];
	s->s[Y1] = y;
	s->s[X1] = x;

	return y;
}


#define MASK 16777215 // ((1 << PRECISION) - 1) (for PRECISION = 24)
#define SHIFT 22      // (PRECISION - 2)        (for PRECISION = 24)

float SquareX6Step(const struct SquareX6Program* restrict p, struct SquareX6State* restrict s)
{
	s->phase[0] = (s->phase[0] + p->step[0]) & MASK;
	s->phase[1] = (s->phase[1] + p->step[1]) & MASK;
	s->phase[2] = (s->phase[2] + p->step[2]) & MASK;
	s->phase[3] = (s->phase[3] + p->step[3]) & MASK;
	s->phase[4] = (s->phase[4] + p->step[4]) & MASK;
	s->phase[5] = (s->phase[5] + p->step[5]) & MASK;

	float add;
	add = (float)(((s->phase[0] >> SHIFT) & 2) - 1);
	add += (float)(((s->phase[1] >> SHIFT) & 2) - 1);
	add += (float)(((s->phase[2] >> SHIFT) & 2) - 1);
	add += (float)(((s->phase[3] >> SHIFT) & 2) - 1);
	add += (float)(((s->phase[4] >> SHIFT) & 2) - 1);
	add += (float)(((s->phase[5] >> SHIFT) & 2) - 1);

	return add * p->amplitude;
}


float CheapDistortion(float x, float f)
{
	return sEasing(x, f);
}

static float sFastNegExp(float x)
{
	assert(x >= 0.0f);

	/*
	https://x.com/Mr_Rowl/status/1577454895652708352

	Danny Chapman
	@Mr_Rowl

	1. Approximations for exp(-x). A simple 3rd order Taylor expansion (red) is OK when x=1,
	but has an error of about 50% out at x=4, where exp(-x) is still non-negligible. 4th
	order is better, but maybe you don't want to be calculating that 4th power...

	2. You can do better if you just optimise the coefficients of the 3rd order expansion:
	y = 1 / (1 + a x + b x^2 + c x^3) by minimising the error. If you do that in the
	range 0 < x< 4 then we get the blue line here, with a=1.119186 b=0.097090 c=0.489114
	- it beats 4th order Taylor

	3. In many cases (e.g. exponential decay over a single timestep) x is less than 1, and the
	values  a=1.006722 b=0.453568 c=0.254559 are slightly more accurate. Green line
	(obscuring the exact/black curve here!)

	4. If you want to be really cheap, then even the 2nd order approximation is very good for
	0 < x < 1. Here y = 1 / (1 + a x + b x^2) with a=0.943620 b=0.728478 (red, with the exact
	in black)

	5. It's often tempting to just write exp(-x) in code, but these approximations (in 3 or 4
	above) are so simple, accurate and well behaved, it's worth dropping them in when you
	know you're in an exponential decay situation.
	*/

	return 1.0f / (1.0f + x * (1.119186f + x * (0.097090f + x * 0.489114f)));

	// Found in:
	// Daniel Holden. "Spring-It-On: The Game Developer's Spring-Roll-Call" (04/03/2021)
	// https://theorangeduck.com/page/spring-roll-call
}

float FancyDistortion(float x, float f)
{
	assert(x >= -1.0f && x <= 1.0f);
	assert(f > 0.001f);
	const float v = (sFastNegExp(sAbs(x) * f) - 1.0f) / (sFastNegExp(f) - 1.0f);
	return (x > 0.0f) ? v : -v;
}


float TailStep(struct TailProgram* p, struct TailState* s)
{
	s->x *= p->c;
	return s->x;
}


//


static float sMax(float a, float b)
{
	return (a > b) ? a : b;
}

static float sRound(float x)
{
	return (x >= 0.0f) ? (float)((int)(x + 0.5f)) : (float)((int)(x - 0.5f));
}

static float sSemitoneDetune(float x)
{
	const float a = sFastNegExp(LOG_2_SEMITONE * sAbs(x));
	return (x < 0.0f) ? a : 1.0f / a;
}


void OscillatorSetProgram(float sampling_frequency, float decay_ms, float sweep_factor,
                          struct OscillatorProgram* restrict p)
{
	assert(sampling_frequency >= 0.0f);
	assert(decay_ms >= 0.0f);

	const float decay_samples = (decay_ms * sampling_frequency) / 1000.0f;

	p->zeta = (1.0f - sFastNegExp((-LOG_60DB) / decay_samples)) * 2.0f;

	p->sweep_target = sSemitoneDetune(sweep_factor);
	p->sweep_step = 1.0f - sFastNegExp((-LOG_100_PERCENT) / decay_samples);
}

void OscillatorSetState(enum StateState state_state, float sampling_frequency, float frequency, float delay_ms,
                        float amplitude, struct OscillatorState* restrict s)
{
	assert(sampling_frequency >= 0.0f);
	assert(frequency >= 0.0f);
	assert(delay_ms >= 0.0f);

	switch (state_state)
	{
	case STATE_START:
		s->delay = (int)((delay_ms * sampling_frequency) / 1000.0f) - 1;
		s->omega = (frequency / sampling_frequency) * PI_TWO;
		s->omega = s->omega * s->omega;
		s->v = ((frequency / sampling_frequency) * PI_TWO) * amplitude; // It's omega before its ^2
		s->x = 0.0f;
		s->sweep = 1.0;
		break;
	case STATE_DEAD:
		s->delay = 0;
		s->omega = (frequency / sampling_frequency) * PI_TWO;
		s->omega = s->omega * s->omega;
		s->v = 0.0f;
		s->x = 0.0f;
		s->sweep = 0.0;
	}
}


void EnvelopeSetProgram(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms, float sustain,
                        float decay2_ms, float amplitude, struct EnvelopeProgram* restrict p)
{
	assert(sampling_frequency >= 0.0f);
	assert(delay_ms >= 0.0f);
	assert(attack_ms >= 0.0f);
	assert(decay_ms >= 0.0f);

	p->durations[0] = sMax((-LOG_100_PERCENT), (delay_ms * sampling_frequency) / 1000.0f);
	p->durations[1] = sMax((-LOG_100_PERCENT), (attack_ms * sampling_frequency) / 1000.0f);
	p->durations[2] = sMax((-LOG_100_PERCENT), (decay_ms * sampling_frequency) / 1000.0f);
	p->durations[3] = sMax((-LOG_100_PERCENT), (decay2_ms * sampling_frequency) / 1000.0f);

	p->levels[0] = 0.0f;
	p->levels[1] = amplitude;
	p->levels[2] = sustain * amplitude;
	p->levels[3] = (decay2_ms <= 0.0f) ? (sustain * amplitude) : 0.0f;
}

void EnvelopeSetState(enum StateState state_state, struct EnvelopeState* restrict s)
{
	switch (state_state)
	{
	case STATE_START:
		s->x = 0;
		s->y = 0.0f;
		s->stage = 0;
		break;
	case STATE_DEAD:
		s->x = 0;
		s->y = 0.0f;
		s->stage = 3;
	}
}


void ShapedEnvelopeSetProgram(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms,
                              float attack_shape, float decay_shape, struct ShapedEnvelopeProgram* restrict p)
{
	assert(sampling_frequency >= 0.0f);
	assert(delay_ms >= 0.0f);
	assert(attack_ms >= 0.0f);
	assert(decay_ms >= 0.0f);

	p->durations[0] = (uint32_t)((delay_ms * sampling_frequency) / 1000.0f);
	p->durations[1] = (uint32_t)((attack_ms * sampling_frequency) / 1000.0f);
	p->durations[2] = (uint32_t)((decay_ms * sampling_frequency) / 1000.0f);
	p->durations[3] = 0.0f;

	p->steps[0] = 0.0f;
	p->steps[1] = (1.0f / (float)(p->durations[1]));
	p->steps[2] = (decay_ms > 0.0f) ? (-(1.0f / (float)(p->durations[2]))) : 0.0f;
	p->steps[3] = 0.0f;

	p->shapes[0] = attack_shape;
	p->shapes[1] = decay_shape;
}

void ShapedEnvelopeSetState(enum StateState state_state, struct ShapedEnvelopeState* restrict s)
{
	switch (state_state)
	{
	case STATE_START:
		s->x = 0;
		s->y = 0.0f;
		s->stage = 0;
		break;
	case STATE_DEAD:
		s->x = 0;
		s->y = 0.0f;
		s->stage = 3;
	}
}


void NoiseSet(uint32_t seed, struct NoiseState* s)
{
	assert(seed != 0);
	s->x = seed;
}


#define C1 0.999999060898976336474926982596043563f
#define C2 -0.166655540927576933646197607200949732f
#define C3 0.00831189980138987918776159520367912155f
#define C4 0.000184881402886071911033139680005197992f

static float sSin(float x)
{
	// Lasse Schlör. Fast MiniMax Polynomial Approximations of Sine and Cosine.
	// https://gist.github.com/publik-void/067f7f2fef32dbe5c27d6e215f824c91#sin-rel-error-minimized-degree-7

	// Turn x into a triangle
	x += HALF_PI;
	x = sAbs(x - PI_TWO * sRound(x / PI_TWO)) - HALF_PI;

	// Polynomial'ise it
	const float xx = x * x;
	return x * (C1 + xx * (C2 + xx * (C3 - C4 * xx)));
}

static float sCos(float x)
{
	return sSin(x + HALF_PI);
}


void FilterSetProgram(float sampling_frequency, enum Filter12dbType type, float cutoff, float resonance, float amplify,
                      struct FilterProgram* restrict p)
{
	assert(sampling_frequency >= 0.0f);
	assert(cutoff >= 0.0f);
	assert(cutoff <= sampling_frequency / 2.0f);
	assert(resonance >= 0.0f);

	// Cookbook formulae for audio equalizer biquad filter coefficients
	// Robert Bristow-Johnson
	// https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html

	const float wo = PI_TWO * (cutoff / sampling_frequency);
	const float alpha = sSin(wo) / (2.0f * resonance);
	float a0 = 1.0f;

	switch (type)
	{
		// It's better to emulate 6db filters. It keeps the same API.
		// Step() function is mostly the same (one multiplication,
		// a shuffle, and a horizontal addition, just using packed
		// floats rather than single floats). And it's less code.
		// Only con is that it uses more coefficients

	case RC_HIGHPASS_6DB: // Mine, common sense
	{
		p->c_b0 = sFastNegExp(wo);
		p->c[B1] = -sFastNegExp(wo);
		p->c[B2] = 0.0f;

		a0 = 1.0f;
		p->c[A1] = -sFastNegExp(wo);
		p->c[A2] = 0.0f;
		break;
	}
	case RC_LOWPASS_6DB: // Mine, common sense
	{
		const float a = sFastNegExp(wo);
		p->c_b0 = 1.0f - a;
		p->c[B1] = 0.0f;
		p->c[B2] = 0.0f;

		a0 = 1.0f;
		p->c[A1] = -a;
		p->c[A2] = 0.0f;
		break;
	}
	case RC_LOWPASS_12DB: // Mine, brute force
	{
		const float a = sFastNegExp(wo);
		p->c_b0 = (1.0f - a) * (1.0f - a);
		p->c[B1] = 0.0f;
		p->c[B2] = 0.0f;

		a0 = 1.0f;
		p->c[A1] = -a * 2.0f;
		p->c[A2] = a * a;
		break;
	}
	case BANDPASS_12DB: // Robert Bristow-Johnson
	{
		p->c_b0 = sSin(wo) / 2.0f;
		p->c[B1] = 0.0f;
		p->c[B2] = (-sSin(wo)) / 2.0f;

		a0 = 1.0f + alpha;
		p->c[A1] = -2.0f * sCos(wo);
		p->c[A2] = 1.0f - alpha;
		break;
	}
	case HIGHPASS_12DB: // Robert Bristow-Johnson
	{
		p->c_b0 = (1.0f + sCos(wo)) / 2.0f;
		p->c[B1] = -(1.0f + sCos(wo));
		p->c[B2] = (1.0f + sCos(wo)) / 2.0f;

		a0 = 1.0f + alpha;
		p->c[A1] = -2.0f * sCos(wo);
		p->c[A2] = 1.0f - alpha;
		break;
	}
	default: // LOWPASS_12DB, Robert Bristow-Johnson
	{
		p->c_b0 = (1.0f - sCos(wo)) / 2.0f;
		p->c[B1] = 1.0f - sCos(wo);
		p->c[B2] = (1.0f - sCos(wo)) / 2.0f;

		a0 = 1.0f + alpha;
		p->c[A1] = -2.0f * sCos(wo);
		p->c[A2] = 1.0f - alpha;
	}
	}

	// Optimization, division used to be in loop body [a]
	p->c_b0 = (p->c_b0 / a0) * amplify;
	p->c[B1] = (p->c[B1] / a0) * amplify;
	p->c[B2] = (p->c[B2] / a0) * amplify;
	p->c[A1] /= a0;
	p->c[A2] /= a0;

	// Flip, in [a] used to be subtractions
	p->c[A1] = -p->c[A1];
	p->c[A2] = -p->c[A2];
}

void FilterSetState(struct FilterState* restrict s)
{
	s->s[0] = 0.0f;
	s->s[1] = 0.0f;
	s->s[2] = 0.0f;
	s->s[3] = 0.0f;
}


void SquareX6SetProgram(float sampling_frequency, float amplitude, float frequency1, float frequency2, float frequency3,
                        float frequency4, float frequency5, float frequency6, struct SquareX6Program* restrict p)
{
	p->step[0] = (int32_t)((frequency1 / sampling_frequency) * (float)(MASK));
	p->step[1] = (int32_t)((frequency2 / sampling_frequency) * (float)(MASK));
	p->step[2] = (int32_t)((frequency3 / sampling_frequency) * (float)(MASK));
	p->step[3] = (int32_t)((frequency4 / sampling_frequency) * (float)(MASK));
	p->step[4] = (int32_t)((frequency5 / sampling_frequency) * (float)(MASK));
	p->step[5] = (int32_t)((frequency6 / sampling_frequency) * (float)(MASK));

	p->amplitude = (1.0f / 6.0f) * amplitude;
}

void SquareX6SetState(uint32_t seed, struct SquareX6State* restrict s)
{
	assert(seed != 0);

	const uint32_t p0 = sXorshift(seed);
	const uint32_t p1 = sXorshift(p0);
	const uint32_t p2 = sXorshift(p1);
	const uint32_t p3 = sXorshift(p2);
	const uint32_t p4 = sXorshift(p3);
	const uint32_t p5 = sXorshift(p4);

	s->phase[0] = p0 & MASK;
	s->phase[1] = p1 & MASK;
	s->phase[2] = p2 & MASK;
	s->phase[3] = p3 & MASK;
	s->phase[4] = p4 & MASK;
	s->phase[5] = p5 & MASK;
}


void TailSetProgram(float sampling_frequency, float decay_ms, struct TailProgram* restrict p)
{
	const float decay_samples = (decay_ms * sampling_frequency) / 1000.0f;
	p->c = sFastNegExp((-LOG_100_PERCENT) / decay_samples);
}

void TailSetState(struct TailState* restrict s)
{
	s->x = 0.0f;
}

void TailAccumulate(struct TailState* restrict s, float signal)
{
	s->x += signal;
}


void KickSetProgram(float sampling_frequency, struct KickProgram* restrict p)
{
	ShapedEnvelopeSetProgram(sampling_frequency, 0.0f, 1.13f, 2.58f - 1.13f, -0.4f, 0.2f, &p->env);
	OscillatorSetProgram(sampling_frequency, 270.0f, 0.0f, &p->osc[0]);
	OscillatorSetProgram(sampling_frequency, 65.0f, 0.0f, &p->osc[1]);
}

static float sMap(float in_a, float in_b, float out_a, float out_b, float f)
{
	return out_a + (out_b - out_a) * ((f - in_a) / (in_b - in_a));
}

static float sMix(float a, float b, float f)
{
	return a + (b - a) * f;
}

float KickSetState(enum StateState state_state, float sampling_frequency, float velocity, float vel_amp_mod,
                   struct KickState* restrict s)
{
	assert(velocity >= 0.0f && velocity <= 1.0f);
	assert(vel_amp_mod >= 0.0f && vel_amp_mod <= 1.0f);

	const float general_amplify = sMix(1.0f, velocity * velocity, vel_amp_mod);

	s->distortion = sMap(0.5f, 1.0f, 0.0f, -0.25f, sMax(velocity, 0.5f)); // Distortion is linear
	s->click_amplify = general_amplify;

	const float detune = sSemitoneDetune(sMap(0.5f, 1.0f, 0.0f, 2.0f, sMax(velocity, 0.5f))); // Linear as well

	ShapedEnvelopeSetState(state_state, &s->env);
	OscillatorSetState(state_state, sampling_frequency, 60.0f * detune, 2.58f, 0.7f * general_amplify, &s->osc[0]);
	OscillatorSetState(state_state, sampling_frequency, 120.0f * detune, 2.58f, 0.3f * general_amplify, &s->osc[1]);

	return (2.58f + 270.0f) + ((2.58f + 270.0f) * 10.0f) / 100.0f;
}

static float sKickStep(const struct KickProgram* restrict p, struct KickState* restrict s)
{
	float signal = -ShapedEnvelopeStep(&p->env, &s->env) * s->click_amplify; // Initial click
	signal += OscillatorStep(&p->osc[0], &s->osc[0]);
	signal += OscillatorStep(&p->osc[1], &s->osc[1]);
	return CheapDistortion(signal, s->distortion);
}

float RenderKick(float mixer_amplify, const struct KickProgram* restrict p, struct KickState* restrict s, float* out,
                 const float* out_end)
{
#ifndef NDEBUG
	float max_level = 0.0f;
#endif

	float signal;
	for (float* sample = out; sample < out_end; sample += 1)
	{
		signal = sKickStep(p, s) * mixer_amplify;
		*sample = signal;

#ifndef NDEBUG
		max_level = sMax(sAbs(signal), max_level);
#endif
	}

#ifndef NDEBUG
#ifndef FREESTANDING
	printf("Normalisation of x%f required at the end\n", 1.0f / max_level);
#endif
#endif

	return signal;
}

float RenderAdditiveKick(float mixer_amplify, const struct KickProgram* restrict p, struct KickState* restrict s,
                         float* out, const float* out_end)
{
	float signal;
	for (float* sample = out; sample < out_end; sample += 1)
	{
		signal = sKickStep(p, s) * mixer_amplify;
		*sample += signal;
	}
	return signal;
}


void SnareSetProgram(float sampling_frequency, struct SnareProgram* restrict p)
{
	OscillatorSetProgram(sampling_frequency, 140.0f, -12.0f, &p->osc);
	EnvelopeSetProgram(sampling_frequency, 0.0f, 2.0f, 60.0f, 0.05f, 50.0f, 3.75f, &p->env);
	FilterSetProgram(sampling_frequency, HIGHPASS_12DB, 3500.0f, 0.6f, 1.0f, &p->filter[0]);
	FilterSetProgram(sampling_frequency, RC_LOWPASS_6DB, 500.0f, 0.0f, 1.0f, &p->filter[1]);
}

float SnareSetState(enum StateState state_state, float sampling_frequency, uint32_t seed, float velocity,
                    float vel_amp_mod, struct SnareState* restrict s)
{
	assert(velocity >= 0.0f && velocity <= 1.0f);
	assert(vel_amp_mod >= 0.0f && vel_amp_mod <= 1.0f);

	const float general_amplify = sMix(1.0f, velocity * velocity, vel_amp_mod);

	s->distortion = sMap(0.5f, 1.0f, 0.0f, -0.3f, sMax(velocity, 0.5f)); // Distortion is linear
	s->noise_amplify = sMap(0.25f, 1.0f, 1.0f, 1.75f, sMax(velocity * velocity, 0.25f)) * general_amplify;
	const float osc_amplify = sMap(0.25f, 1.0f, 1.0f, 0.6f, sMax(velocity * velocity, 0.25f)) * general_amplify;

	const float detune = sSemitoneDetune(sMap(0.5f, 1.0f, 0.0f, -2.0f, sMax(velocity, 0.5f))); // Linear as well

	OscillatorSetState(state_state, sampling_frequency, 310.0f * detune, 1.0f, 0.75f * osc_amplify, &s->osc);
	NoiseSet(seed, &s->noise);
	EnvelopeSetState(state_state, &s->env);
	FilterSetState(&s->filter[0]);
	FilterSetState(&s->filter[1]);

	return 140.0f + (140.0f * 10.0f) / 100.0f;
}

static float sSnareStep(const struct SnareProgram* restrict p, struct SnareState* restrict s)
{
	float signal = OscillatorStep(&p->osc, &s->osc);

	const float noise = NoiseStep(&s->noise) * EnvelopeStep(&p->env, &s->env);
	signal +=
	    FilterStep(FilterStep(noise, &p->filter[0], &s->filter[0]), &p->filter[1], &s->filter[1]) * s->noise_amplify;

	return CheapDistortion(signal, s->distortion);
}

float RenderSnare(float mixer_amplify, const struct SnareProgram* restrict p, struct SnareState* restrict s, float* out,
                  const float* out_end)
{
#ifndef NDEBUG
	float max_level = 0.0f;
#endif

	float signal;
	for (float* sample = out; sample < out_end; sample += 1)
	{
		signal = sSnareStep(p, s) * mixer_amplify;
		*sample = signal;

#ifndef NDEBUG
		max_level = sMax(sAbs(signal), max_level);
#endif
	}

#ifndef NDEBUG
#ifndef FREESTANDING
	printf("Normalisation of x%f required at the end\n", 1.0f / max_level);
#endif
#endif

	return signal;
}

float RenderAdditiveSnare(float mixer_amplify, const struct SnareProgram* restrict p, struct SnareState* restrict s,
                          float* out, const float* out_end)
{
	float signal;
	for (float* sample = out; sample < out_end; sample += 1)
	{
		signal = sSnareStep(p, s) * mixer_amplify;
		*sample += signal;
	}
	return signal;
}


void HatSetProgram(float sampling_frequency, enum HatType type, struct HatProgram* restrict p)
{
	float magic_metallic_normalisation;

	switch (type)
	{
	case OPEN_HAT:
	{
		magic_metallic_normalisation = 3.0f; // Obtained in [b]
		p->fade_out_in_c = 1.0f;
		break;
	}
	case CLOSED_HAT:
	{
		magic_metallic_normalisation = 3.0f; // Obtained in [b]
		p->fade_out_in_c = 1.0f;
		break;
	}
	case CYMBAL:
	{
		magic_metallic_normalisation = 4.5f; // Obtained in [b]
		const float fade_out_in_samples = (3000.0f * sampling_frequency) / 1000.0f;
		p->fade_out_in_c = sFastNegExp((-LOG_100_PERCENT) / fade_out_in_samples);
		break;
	}
	}

	SquareX6SetProgram(sampling_frequency, magic_metallic_normalisation, 684.35f, 511.97f, 305.88f, 420.2f, 271.14f,
	                   201.23f, &p->sqr);

	FilterSetProgram(sampling_frequency, BANDPASS_12DB, 7100.0f, 3.0f, 1.0, &p->bp[0]);
	FilterSetProgram(sampling_frequency, HIGHPASS_12DB, 7100.0f, 0.5f, 1.0, &p->bp[1]);

	FilterSetProgram(sampling_frequency, RC_HIGHPASS_6DB, 7100.0f, 0.5f, 1.0f, &p->hp);
	FilterSetProgram(sampling_frequency, RC_LOWPASS_12DB, 7100.0f, 0.0f, 1.0f, &p->lp);

	FilterSetProgram(sampling_frequency, BANDPASS_12DB, 3400.0f, 4.0f, 0.125f, &p->bp[2]);
	// FilterSetProgram(sampling_frequency, BANDPASS_12DB, 3400.0f, 4.0f, 0.1875f, &p->bp[2]);
	// FilterSetProgram(sampling_frequency, BANDPASS_12DB, 3400.0f, 4.0f, 0.25f, &p->bp[2]); // Up to here cymbal is
	// tolerable
}

float HatSetState(enum StateState state_state, float sampling_frequency, enum HatType type, uint32_t seed,
                  float velocity, float vel_amp_mod, struct HatState* restrict s)
{
	assert(velocity >= 0.0f && velocity <= 1.0f);
	assert(vel_amp_mod >= 0.0f && vel_amp_mod <= 1.0f);

	float short_attack;
	float long_attack;
	float short_length;
	float long_length;
	float long_shape;

	const float general_amplify = sMix(1.0f, velocity * velocity, vel_amp_mod);

	switch (type)
	{
	case OPEN_HAT:
	{
		s->final_amplify =
		    sMap(0.5f, 1.0f, 2.1f, 1.9f, sMax(velocity * velocity, 0.5)) * general_amplify; // Obtained in [c]
		s->long_gain = 1.2f;
		s->noise_gain = 0.038f;

		short_attack = sMap(0.5f, 1.0f, 1.25f, 10.0f, sMax(velocity * velocity, 0.5));
		long_attack = short_attack;
		short_length = sMap(0.25f, 1.0f, 500.0f, 2000.0f, sMax(velocity * velocity, 0.25));
		s->fade_out_in = 1.0f;

		// Following two are allowed to be shorter at low velocities, that's why
		// the 0.125 rather than common sense 0.25
		long_length = sMap(0.25f, 1.0f, 1500.0f, 2000.0f, sMax(velocity * velocity, 0.125));
		long_shape = sMap(0.25f, 1.0f, 0.4f, 0.1f, sMax(velocity * velocity, 0.125));
		break;
	}
	case CLOSED_HAT:
	{
		s->final_amplify =
		    sMap(0.5f, 1.0f, 5.5f, 4.5f, sMax(velocity * velocity, 0.5)) * general_amplify; // Obtained in [c]
		s->long_gain = 0.0f;
		s->noise_gain = 0.0f;

		short_attack = sMap(0.5f, 1.0f, 2.5f, 20.0f, sMax(velocity * velocity, 0.5));
		long_attack = short_attack;
		short_length = sMap(0.25f, 1.0f, 150.0f, 160.0f, sMax(velocity * velocity, 0.25));
		s->fade_out_in = 1.0f;

		long_length = 0.0f;
		long_shape = 0.0f;
		break;
	}
	case CYMBAL:
	{
		s->final_amplify =
		    sMap(0.5f, 1.0f, 5.0f, 2.6f, sMax(velocity * velocity, 0.5)) * general_amplify; // Obtained in [c]
		s->long_gain = sMap(0.5f, 1.0f, 0.18f, 0.8f, sMax(velocity * velocity, 0.5));
		s->noise_gain = sMap(0.5f, 1.0f, 0.0f, 0.2f, sMax(velocity * velocity, 0.5));

		short_attack = sMap(0.5f, 1.0f, 5.0f, 5.0f, sMax(velocity * velocity, 0.5));
		long_attack = sMap(0.5f, 1.0f, 10.0f, 10.0f, sMax(velocity * velocity, 0.5));
		short_length = sMap(0.25f, 1.0f, 150.0f, 150.0f, sMax(velocity * velocity, 0.25));
		s->fade_out_in = sMap(0.25f, 1.0f, 0.75f, 0.75f, sMax(velocity * velocity, 0.25));

		// Same length logic as open hat (with different values)
		long_length = sMap(0.25f, 1.0f, 1400.0f, 2500.0f, sMax(velocity * velocity, 0.125));
		long_shape = sMap(0.25f, 1.0f, 0.6f, 0.6f, sMax(velocity * velocity, 0.125));
		break;
	}
	}

	SquareX6SetState(seed, &s->sqr);

	FilterSetState(&s->bp[0]);
	FilterSetState(&s->bp[1]);
	FilterSetState(&s->bp[2]);

	ShapedEnvelopeSetProgram(sampling_frequency, 0.0f, long_attack, long_length, 0.4f, long_shape, &s->env_long_p);
	EnvelopeSetProgram(sampling_frequency, 0.0f, short_attack, 0.0f, 1.0f, short_length, 1.0f, &s->env_short_p);
	ShapedEnvelopeSetState(state_state, &s->env_long);
	EnvelopeSetState(state_state, &s->env_short);

	FilterSetState(&s->hp);
	FilterSetState(&s->lp);

	NoiseSet(seed, &s->noise);

	switch (type)
	{
	case OPEN_HAT: return (short_attack + long_length) + ((short_attack + long_length) * 10.0f) / 100.0f;
	case CLOSED_HAT: return (short_attack + short_length) + ((short_attack + short_length) * 10.0f) / 100.0f;
	case CYMBAL: return (long_attack + long_length) + ((long_attack + long_length) * 10.0f) / 100.0f;
	}
	return 0.0f;
}

float RenderHat(float amplify, const struct HatProgram* restrict p, struct HatState* restrict s, float* out,
                const float* out_end)
{
#ifndef NDEBUG
	float max_level_metallic = 0.0f;
	float max_level_final = 0.0f;
#endif

	float signal;
	for (float* sample = out; sample < out_end; sample += 1)
	{
		// Render bandpass filtered metallic noise
		const float square = SquareX6Step(&p->sqr, &s->sqr);
		const float high = FilterStep(FilterStep(square, &p->bp[0], &s->bp[0]), &p->bp[1], &s->bp[1]);
		const float low = FilterStep(square, &p->bp[2], &s->bp[2]);

		s->fade_out_in *= p->fade_out_in_c;
		signal = low + (high - low) * s->fade_out_in;

#ifndef NDEBUG
		max_level_metallic = sMax(sAbs(signal), max_level_metallic); // [b]
#endif

		// Distort, and filter noise added by it (we want low frequencies clean)
		signal = CheapDistortion(signal, -0.6f);
		signal = FilterStep(signal, &p->hp, &s->hp);

		// Envelope it
		signal *= ShapedEnvelopeStep(&s->env_long_p, &s->env_long) * s->long_gain //
		          + EnvelopeStep(&s->env_short_p, &s->env_short);

		// Add transient white noise
		signal += NoiseStep(&s->noise) * EnvelopeStep(&s->env_short_p, &s->env_short) * s->noise_gain;

		// Final filter
		*sample = FilterStep(signal, &p->lp, &s->lp) * amplify * s->final_amplify;

#ifndef NDEBUG
		max_level_final = sMax(sAbs(*sample), max_level_final); // [c]
#endif
	}

#ifndef NDEBUG
#ifndef FREESTANDING
	printf("Normalisation of x%f required after metallic noise\n", 1.0f / max_level_metallic);
	printf("Normalisation of x%f required at the end\n", 1.0f / max_level_final);
#endif
#endif

	return signal;
}

float RenderAdditiveHat(float amplify, const struct HatProgram* restrict p, struct HatState* restrict s, float* out,
                        const float* out_end)
{
	float signal;
	for (float* sample = out; sample < out_end; sample += 1)
	{
		// Render bandpass filtered metallic noise
		const float square = SquareX6Step(&p->sqr, &s->sqr);
		const float high = FilterStep(FilterStep(square, &p->bp[0], &s->bp[0]), &p->bp[1], &s->bp[1]);
		const float low = FilterStep(square, &p->bp[2], &s->bp[2]);

		s->fade_out_in *= p->fade_out_in_c;
		signal = low + (high - low) * s->fade_out_in;

		// Distort, and filter noise added by it (we want low frequencies clean)
		signal = CheapDistortion(signal, -0.6f);
		signal = FilterStep(signal, &p->hp, &s->hp);

		// Envelope it
		signal *= ShapedEnvelopeStep(&s->env_long_p, &s->env_long) * s->long_gain //
		          + EnvelopeStep(&s->env_short_p, &s->env_short);

		// Add transient white noise
		signal += NoiseStep(&s->noise) * EnvelopeStep(&s->env_short_p, &s->env_short) * s->noise_gain;

		// Final filter
		signal = FilterStep(signal, &p->lp, &s->lp) * amplify * s->final_amplify;
		*sample += signal;
	}

	return signal;
}


void TomSetProgram(float sampling_frequency, enum TomType type, struct TomProgram* restrict p)
{
	float length;

	switch (type)
	{
	case LOW_TOM: length = 390.0f; break;
	case HIGH_TOM: length = 250.0f; break;
	}

	ShapedEnvelopeSetProgram(sampling_frequency, 0.0f, 0.5f, 1.5f - 0.5f, 0.8f, -0.2f, &p->env);
	OscillatorSetProgram(sampling_frequency, length, -1.0f, &p->osc);

	FilterSetProgram(sampling_frequency, LOWPASS_12DB, 150.0f, 4.0f, 1.0f, &p->lp);
	EnvelopeSetProgram(sampling_frequency, 0.0f, 10.0f, 0.0f, 1.0f, length - 10.0f, 0.5f, &p->env2);
}

float TomSetState(enum StateState state_state, float sampling_frequency, enum TomType type, uint32_t seed,
                  float velocity, float vel_amp_mod, struct TomState* restrict s)
{
	assert(velocity >= 0.0f && velocity <= 1.0f);
	assert(vel_amp_mod >= 0.0f && vel_amp_mod <= 1.0f);

	// TODO, I cannot hear velocity affecting tone. I can see more transient on the spectrum, but
	// it feels more like artifacts in the samples, which are normalised, than anything else.
	// Also, I'm also old :,(
	const float general_amplify = sMix(1.0f, velocity * velocity, vel_amp_mod);

	float frequency;
	switch (type)
	{
	case LOW_TOM: frequency = 150.0f; break;
	case HIGH_TOM: frequency = 210.0f; break;
	}

	s->click_amplify = general_amplify;
	ShapedEnvelopeSetState(state_state, &s->env);
	OscillatorSetState(state_state, sampling_frequency, frequency, 1.5f, 1.0f * general_amplify, &s->osc);

	NoiseSet(seed, &s->noise);
	FilterSetState(&s->lp);
	EnvelopeSetState(STATE_START, &s->env2);

	switch (type)
	{
	case LOW_TOM: return (1.5f + 390.0f) + ((1.5f + 390.0f) * 10.0f) / 100.0f; break;
	case HIGH_TOM: return (1.5f + 250.0f) + ((1.5f + 250.0f) * 10.0f) / 100.0f; break;
	}

	return 0.0f;
}

static float sTomStep(const struct TomProgram* restrict p, struct TomState* restrict s)
{
	float signal = -ShapedEnvelopeStep(&p->env, &s->env) * s->click_amplify;
	signal += OscillatorStep(&p->osc, &s->osc);
	signal += FilterStep(NoiseStep(&s->noise), &p->lp, &s->lp) * EnvelopeStep(&p->env2, &s->env2);
	return signal;
}

float RenderTom(float mixer_amplify, const struct TomProgram* restrict p, struct TomState* restrict s, float* out,
                const float* out_end)
{
#ifndef NDEBUG
	float max_level = 0.0f;
#endif

	float signal;
	for (float* sample = out; sample < out_end; sample += 1)
	{
		signal = sTomStep(p, s) * mixer_amplify;
		*sample = signal;

#ifndef NDEBUG
		max_level = sMax(sAbs(signal), max_level);
#endif
	}

#ifndef NDEBUG
#ifndef FREESTANDING
	printf("Normalisation of x%f required at the end\n", 1.0f / max_level);
#endif
#endif

	return signal;
}

float RenderAdditiveTom(float mixer_amplify, const struct TomProgram* restrict p, struct TomState* restrict s,
                        float* out, const float* out_end)
{
	float signal;
	for (float* sample = out; sample < out_end; sample += 1)
	{
		signal = sTomStep(p, s) * mixer_amplify;
		*sample += signal;
	}
	return signal;
}
