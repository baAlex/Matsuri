/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#ifndef MATSURI_LIBRARY_H
#define MATSURI_LIBRARY_H

#include <stdint.h>


enum StateState
{
	STATE_START,
	STATE_DEAD // Completely dead, not a soft release
};


struct OscillatorProgram
{
	float zeta;
	float omega;
	float sweep_step;
	float sweep_target;
};

struct OscillatorState
{
	int delay;
	float v;
	float x;
	float sweep;
};

void OscillatorSet(enum StateState, float sampling_frequency, float frequency, float decay_ms, float delay_ms,
                   float amplitude, float sweep_factor, struct OscillatorProgram* p, struct OscillatorState* s);
void OscillatorSetState(enum StateState, float sampling_frequency, float frequency, float delay_ms, float amplitude,
                        struct OscillatorState* s);
float OscillatorStep(const struct OscillatorProgram* p, struct OscillatorState* s);


struct EnvelopeProgram
{
	float durations[4];
	float levels[4];
};

struct EnvelopeState
{
	uint32_t x;
	float y;
	uint8_t stage;
};

void EnvelopeSet(enum StateState, float sampling_frequency, float delay_ms, float attack_ms, float decay_ms,
                 float sustain, float decay2_ms, float amplitude, struct EnvelopeProgram* p, struct EnvelopeState* s);
void EnvelopeSetState(enum StateState, struct EnvelopeState* s);
float EnvelopeStep(const struct EnvelopeProgram* p, struct EnvelopeState* s);


struct ShapedEnvelopeProgram
{
	uint32_t durations[4];
	float steps[4];
	float shapes[2];
};

struct ShapedEnvelopeState
{
	uint32_t x;
	float y;
	uint8_t stage;
};

void ShapedEnvelopeSet(enum StateState, float sampling_frequency, float delay_ms, float attack_ms, float decay_ms,
                       float attack_shape, float decay_shape, struct ShapedEnvelopeProgram* p,
                       struct ShapedEnvelopeState* s);
void ShapedEnvelopeSetState(enum StateState, struct ShapedEnvelopeState* s);
float ShapedEnvelopeStep(const struct ShapedEnvelopeProgram* p, struct ShapedEnvelopeState* s);


struct NoiseState
{
	uint32_t x;
};

void NoiseSet(uint32_t seed, struct NoiseState* s);
float NoiseStep(struct NoiseState* s);


struct FilterProgram
{
	float c[4];
	float c_b0;
};

struct FilterState
{
	float s[4];
};

enum Filter12dbType
{
	LOWPASS_12DB,
	HIGHPASS_12DB,
	BANDPASS_12DB,
	RC_LOWPASS_6DB,
	RC_HIGHPASS_6DB,
	RC_LOWPASS_12DB
};

void FilterSet(float sampling_frequency, enum Filter12dbType type, float cutoff, float resonance,
               struct FilterProgram* p, struct FilterState* s);
void FilterSetState(struct FilterState* s);
float FilterStep(float x, const struct FilterProgram* p, struct FilterState* s);


struct SquareX6Program
{
	int32_t step[6];
	float amplitude;
};

struct SquareX6State
{
	int32_t phase[6];
};

void SquareX6Set(float sampling_frequency, float amplitude, float frequency1, float frequency2, float frequency3,
                 float frequency4, float frequency5, float frequency6, struct SquareX6Program* p,
                 struct SquareX6State* s);
void SquareX6SetState(struct SquareX6State* s);
float SquareX6Step(const struct SquareX6Program* p, struct SquareX6State* s);


float CheapDistortion(float x, float f);
float FancyDistortion(float x, float f);


struct KickProgram
{
	struct ShapedEnvelopeProgram env;
	struct OscillatorProgram osc[2];
};

struct KickState
{
	struct ShapedEnvelopeState env;
	struct OscillatorState osc[2];
};

void KickSet(enum StateState, float sampling_frequency, struct KickProgram* p, struct KickState* s);
void KickSetState(enum StateState, float sampling_frequency, struct KickState* s);
void RenderKick(const struct KickProgram* p, struct KickState* s, float* out, const float* out_end);
void RenderAdditiveKick(const struct KickProgram* p, struct KickState* s, float* out, const float* out_end);


struct SnareProgram
{
	struct OscillatorProgram osc;
	struct EnvelopeProgram env;
	struct FilterProgram filter[2];
};

struct SnareState
{
	struct OscillatorState osc;
	struct EnvelopeState env;
	struct NoiseState noise;
	struct FilterState filter[2];
};

void SnareSet(enum StateState, float sampling_frequency, struct SnareProgram* p, struct SnareState* s);
void SnareSetState(enum StateState, float sampling_frequency, struct SnareState* s);
void RenderSnare(const struct SnareProgram* p, struct SnareState* s, float* out, const float* out_end);
void RenderAdditiveSnare(const struct SnareProgram* p, struct SnareState* s, float* out, const float* out_end);


struct HatProgram
{
	struct SquareX6Program sqr;

	struct FilterProgram bp[2]; // Bandpass is asymmetrical, 24 of highpass, 12 of lowpass,
	                            // shared between all hats and cymbal

	struct ShapedEnvelopeProgram env_long; // Envelope has a weird shape emulated here by using two of them,
	                                       // also, while both are exponential, one is more "lineal" than
	struct EnvelopeProgram env_short;      // the other, that's why the different types

	struct FilterProgram hp; // For noise added by distorting
	struct FilterProgram lp; // Final filter, mostly shapes white noise

	float long_gain;
	float short_gain;
	float noise_gain;

	float magic_normalisation2;
};

struct HatState
{
	struct SquareX6State sqr;
	struct FilterState bp[2];

	struct ShapedEnvelopeState env_long;
	struct EnvelopeState env_short;

	struct FilterState hp;
	struct FilterState lp;

	struct NoiseState noise;
};

enum HatType
{
	OPEN_HAT,
	CLOSED_HAT
};

void HatSet(enum StateState, float sampling_frequency, enum HatType type, struct HatProgram* p, struct HatState* s);
void HatSetState(enum StateState, struct HatState* s);
void RenderHat(const struct HatProgram* p, struct HatState* s, float* out, const float* out_end);
void RenderAdditiveHat(const struct HatProgram* p, struct HatState* s, float* out, const float* out_end);

#endif
