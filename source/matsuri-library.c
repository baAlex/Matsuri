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


float OscillatorStep(const struct OscillatorSettings* restrict p, struct OscillatorState* restrict s)
{
	const int go = (s->delay < 0) ? 1 : 0;

	s->v -= (float)(go) * (s->v * p->zeta + s->x * p->omega * s->sweep);
	s->x += (float)(go) * (s->v * s->sweep);

	s->sweep += (p->sweep_target - s->sweep) * p->sweep_step * (float)(go);
	s->delay -= 1 - go;

	return s->x;
}


float EnvelopeStep(const struct EnvelopeSettings* restrict p, struct EnvelopeState* restrict s)
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

float ShapedEnvelopeStep(const struct ShapedEnvelopeSettings* restrict p, struct ShapedEnvelopeState* restrict s)
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


float NoiseStep(struct NoiseState* restrict s)
{
	// https://en.wikipedia.org/wiki/Xorshift#Example_implementation

	s->x ^= (uint32_t)((s->x) << 13);
	s->x ^= (uint32_t)((s->x) >> 17);
	s->x ^= (uint32_t)((s->x) << 5);

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

float FilterStep(float x, const struct FilterSettings* restrict p, struct FilterState* restrict s)
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

float SquareX6Step(const struct SquareX6Settings* restrict p, struct SquareX6State* restrict s)
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


void OscillatorInitialise(float sampling_frequency, float frequency, float decay_ms, float delay_ms, float amplitude,
                          float sweep_factor, struct OscillatorSettings* restrict p, struct OscillatorState* restrict s)
{
	assert(sampling_frequency >= 0.0f);
	assert(frequency >= 0.0f);
	assert(decay_ms >= 0.0f);

	const float decay_frames = (decay_ms * sampling_frequency) / 1000.0f;

	p->zeta = (1.0f - sFastNegExp((-LOG_60DB) / decay_frames)) * 2.0f;
	p->omega = (frequency / sampling_frequency) * PI_TWO;
	p->omega = p->omega * p->omega;

	p->sweep_target = sSemitoneDetune(sweep_factor);
	p->sweep_step = 1.0f - sFastNegExp((-LOG_100_PERCENT) / decay_frames);

	OscillatorInitialiseState(sampling_frequency, frequency, delay_ms, amplitude, s);
}

void OscillatorInitialiseState(float sampling_frequency, float frequency, float delay_ms, float amplitude,
                               struct OscillatorState* restrict s)
{
	assert(sampling_frequency >= 0.0f);
	assert(frequency >= 0.0f);
	assert(delay_ms >= 0.0f);

	s->delay = (int)((delay_ms * sampling_frequency) / 1000.0f) - 1;
	s->v = ((frequency / sampling_frequency) * PI_TWO) * amplitude; // It's omega before its ^2
	s->x = 0.0f;

	s->sweep = 1.0;
}


void EnvelopeInitialise(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms, float sustain,
                        float decay2_ms, float amplitude, struct EnvelopeSettings* restrict p,
                        struct EnvelopeState* restrict s)
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

	EnvelopeInitialiseState(s);
}

void EnvelopeInitialiseState(struct EnvelopeState* restrict s)
{
	s->x = 0;
	s->y = 0.0f;
	s->stage = 0;
}


void ShapedEnvelopeInitialise(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms,
                              float attack_shape, float decay_shape, struct ShapedEnvelopeSettings* restrict p,
                              struct ShapedEnvelopeState* restrict s)
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

	ShapedEnvelopeInitialiseState(s);
}

void ShapedEnvelopeInitialiseState(struct ShapedEnvelopeState* restrict s)
{
	s->x = 0;
	s->y = 0.0f;
	s->stage = 0;
}


void NoiseInitialise(uint32_t seed, struct NoiseState* s)
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


void FilterInitialise(float sampling_frequency, enum Filter12dbType type, float cutoff, float resonance,
                      struct FilterSettings* restrict p, struct FilterState* restrict s)
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
		// It's better to emulate 6db filters. It keeps the same API
		// The Step() function is mostly the same (one multiplication,
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
	p->c_b0 /= a0;
	p->c[B1] /= a0;
	p->c[B2] /= a0;
	p->c[A1] /= a0;
	p->c[A2] /= a0;

	// Flip, in [a] used to be subtractions
	p->c[A1] = -p->c[A1];
	p->c[A2] = -p->c[A2];

	FilterInitialiseState(s);
}

void FilterInitialiseState(struct FilterState* restrict s)
{
	s->s[0] = 0.0f;
	s->s[1] = 0.0f;
	s->s[2] = 0.0f;
	s->s[3] = 0.0f;
}


void SquareX6Initialise(float sampling_frequency, float amplitude, float frequency1, float frequency2, float frequency3,
                        float frequency4, float frequency5, float frequency6, struct SquareX6Settings* restrict p,
                        struct SquareX6State* restrict s)
{
	p->step[0] = (int32_t)((frequency1 / sampling_frequency) * (float)(MASK));
	p->step[1] = (int32_t)((frequency2 / sampling_frequency) * (float)(MASK));
	p->step[2] = (int32_t)((frequency3 / sampling_frequency) * (float)(MASK));
	p->step[3] = (int32_t)((frequency4 / sampling_frequency) * (float)(MASK));
	p->step[4] = (int32_t)((frequency5 / sampling_frequency) * (float)(MASK));
	p->step[5] = (int32_t)((frequency6 / sampling_frequency) * (float)(MASK));

	p->amplitude = (1.0f / 6.0f) * amplitude;

	SquareX6InitialiseState(s);
}

void SquareX6InitialiseState(struct SquareX6State* restrict s)
{
	if (0)
	{
		s->phase[0] = 7968322;
		s->phase[1] = 14884375;
		s->phase[2] = 10058881;
		s->phase[3] = 13595220;
		s->phase[4] = 1841918;
		s->phase[5] = 3501030;
	}
	else
	{
		// Closed hat sound more natural, less clicky, which makes sense,
		// they are all in phase. That said real world 606 doesn't reset
		// phase or anything, its square oscillators are always running
		s->phase[0] = 0;
		s->phase[1] = 0;
		s->phase[2] = 0;
		s->phase[3] = 0;
		s->phase[4] = 0;
		s->phase[5] = 0;
	}
}


#ifndef NDEBUG
static int sEqual(float a, float b)
{
	// https://embeddeduse.com/2019/08/26/qt-compare-two-floats/

	if (sAbs(a - b) > 1.0e-5f * sMax(sAbs(a), sAbs(b)))
		return 1;
	return 0;
}
#endif


void KickInitialise(float sampling_frequency, struct KickSettings* restrict p, struct KickState* restrict s)
{
	ShapedEnvelopeInitialise(sampling_frequency, 0.0f, 1.13f, 2.58f - 1.13f, -0.4f, 0.2f, &p->env, &s->env);
	OscillatorInitialise(sampling_frequency, 60.0f, 270.0f, 2.58f, 0.7f, 0.0f, &p->osc[0], &s->osc[0]);
	OscillatorInitialise(sampling_frequency, 120.0f, 65.0f, 2.58f, 0.3f, 0.0f, &p->osc[1], &s->osc[1]);
}

void KickInitialiseState(float sampling_frequency, struct KickState* restrict s)
{
	ShapedEnvelopeInitialiseState(&s->env);
	OscillatorInitialiseState(sampling_frequency, 60.0f, 2.58f, 0.7f, &s->osc[0]);
	OscillatorInitialiseState(sampling_frequency, 120.0f, 2.58f, 0.3f, &s->osc[1]);
}

void RenderKick(const struct KickSettings* restrict p, struct KickState* restrict s, float* out, const float* out_end)
{
	for (float* sample = out; sample < out_end; sample += 1)
	{
		*sample = -ShapedEnvelopeStep(&p->env, &s->env); // Initial click
		*sample += OscillatorStep(&p->osc[0], &s->osc[0]);
		*sample += OscillatorStep(&p->osc[1], &s->osc[1]);
	}
}


void SnareInitialise(float sampling_frequency, struct SnareSettings* restrict p, struct SnareState* restrict s)
{
	OscillatorInitialise(sampling_frequency, 310.0f, 140.0f, 1.0f, 0.6f, -12.0f, &p->osc, &s->osc);
	NoiseInitialise(666, &s->noise);
	EnvelopeInitialise(sampling_frequency, 0.0f, 2.0f, 60.0f, 0.05f, 50.0f, 4.0f, &p->env, &s->env);
	FilterInitialise(sampling_frequency, HIGHPASS_12DB, 3500.0f, 0.6f, &p->filter[0], &s->filter[0]);
	FilterInitialise(sampling_frequency, RC_LOWPASS_6DB, 500.0f, 0.0f, &p->filter[1], &s->filter[1]);
}

void SnareInitialiseState(float sampling_frequency, struct SnareState* restrict s)
{
	OscillatorInitialiseState(sampling_frequency, 310.0f, 1.0f, 0.6f, &s->osc);
	NoiseInitialise(666, &s->noise);
	EnvelopeInitialiseState(&s->env);
	FilterInitialiseState(&s->filter[0]);
	FilterInitialiseState(&s->filter[1]);
}

void RenderSnare(const struct SnareSettings* restrict p, struct SnareState* restrict s, float* out,
                 const float* out_end)
{
	for (float* sample = out; sample < out_end; sample += 1)
	{
		const float noise = NoiseStep(&s->noise) * EnvelopeStep(&p->env, &s->env);

		*sample = OscillatorStep(&p->osc, &s->osc);
		*sample += FilterStep(FilterStep(noise, &p->filter[0], &s->filter[0]), &p->filter[1], &s->filter[1]);
	}
}


void HatInitialise(float sampling_frequency, enum HatType type, struct HatSettings* restrict p,
                   struct HatState* restrict s)
{
	const float magic_normalisation1 = 13.043748f; // Obtained in [b]
	float short_length = 500.0f;

	p->magic_normalisation2 = 1.985379f; // Obtained in [c]
	p->long_gain = 1.0f;
	p->short_gain = 0.8f;
	p->noise_gain = 0.04f * p->short_gain;

	if (type == CLOSED_HAT)
	{
		short_length = 150.0f;

		p->magic_normalisation2 = 4.958813f; // Obtained in [c]
		p->long_gain = 0.0f;
		p->short_gain = 1.0f;
		p->noise_gain = 0.0f;
	}

	SquareX6Initialise(sampling_frequency, magic_normalisation1, 684.35f, 511.97f, 305.88f, 420.2f, 271.14f, 201.23f,
	                   &p->sqr, &s->sqr);

	FilterInitialise(sampling_frequency, BANDPASS_12DB, 7100.0f, 2.5f, &p->bp[0], &s->bp[0]);
	FilterInitialise(sampling_frequency, HIGHPASS_12DB, 7100.0f, 0.125f, &p->bp[1],
	                 &s->bp[1]); // Resonance makes an hammer/anvil "clink" at the start
	                             // 0.125 = a lot
	                             // 0.5   = nothing (hat sounds more air-y)

	ShapedEnvelopeInitialise(sampling_frequency, 0.0f, 1.0f, 1500.0f, 0.4f, 0.4f, &p->env_long, &s->env_long);
	EnvelopeInitialise(sampling_frequency, 0.0f, 1.0f, 0.0f, 1.0f, short_length, 1.0f, &p->env_short, &s->env_short);

	FilterInitialise(sampling_frequency, HIGHPASS_12DB, 7100.0f, 0.5f, &p->hp, &s->hp);
	FilterInitialise(sampling_frequency, RC_LOWPASS_12DB, 7100.0f, 0.0f, &p->lp, &s->lp);

	NoiseInitialise(444, &s->noise);
}

void HatInitialiseState(struct HatState* restrict s)
{
	SquareX6InitialiseState(&s->sqr);

	FilterInitialiseState(&s->bp[0]);
	FilterInitialiseState(&s->bp[1]);

	ShapedEnvelopeInitialiseState(&s->env_long);
	EnvelopeInitialiseState(&s->env_short);

	FilterInitialiseState(&s->hp);
	FilterInitialiseState(&s->lp);

	NoiseInitialise(444, &s->noise);
}

void RenderHat(const struct HatSettings* restrict p, struct HatState* restrict s, float* out, const float* out_end)
{
#ifndef NDEBUG
	float max_level = 0.0f;
#endif

	for (float* sample = out; sample < out_end; sample += 1)
	{
		// Render bandpass filtered metallic noise
		*sample = SquareX6Step(&p->sqr, &s->sqr);
		*sample = FilterStep(*sample, &p->bp[0], &s->bp[0]);
		*sample = FilterStep(*sample, &p->bp[1], &s->bp[1]);

		// Normalisation
#ifndef NDEBUG
		max_level = sMax(sAbs(*sample), max_level);
	}

	if (sEqual(max_level, 1.0f) == 1)
	{
#ifndef FREESTANDING
		printf("Normalisation of x%f required after metallic noise\n", 1.0f / max_level); // [b]
#endif

		// Distortion depends on volume, so we want to normalise first
		for (float* sample = out; sample < out_end; sample += 1)
			*sample *= (1.0f / max_level);
	}

	max_level = 0.0f;
	for (float* sample = out; sample < out_end; sample += 1)
	{
#endif

		// Distort, and filter noise added by it (we want low frequencies clean)
		*sample = CheapDistortion(*sample, -0.6f);
		*sample = FilterStep(*sample, &p->hp, &s->hp);

		// Envelope it
		*sample *= ShapedEnvelopeStep(&p->env_long, &s->env_long) * p->long_gain //
		           + EnvelopeStep(&p->env_short, &s->env_short) * p->short_gain;

		// Add transient white noise
		*sample += NoiseStep(&s->noise) * EnvelopeStep(&p->env_short, &s->env_short) * p->noise_gain;

		// Final filter
		*sample = FilterStep(*sample, &p->lp, &s->lp) * p->magic_normalisation2;

		// Normalisation
#ifndef NDEBUG
		max_level = sMax(sAbs(*sample), max_level);
	}

	if (sEqual(max_level, 1.0f) == 1)
	{
#ifndef FREESTANDING
		printf("Normalisation of x%f required at the end\n", 1.0f / max_level); // [c]
#endif

		for (float* sample = out; sample < out_end; sample += 1)
			*sample *= (1.0f / max_level);
#endif
	}
}
