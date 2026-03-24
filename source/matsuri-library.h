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
	STATE_DEAD // Completely dead, an hard stop, not a release
};


struct OscillatorProgram
{
	float zeta;
	float sweep_step;
	float sweep_target;
};

struct OscillatorState
{
	int delay;
	float omega;
	float v;
	float x;
	float sweep;
};

void OscillatorSetProgram(float sampling_frequency, float decay_ms, float sweep_factor, struct OscillatorProgram* p);
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

void EnvelopeSetProgram(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms, float sustain,
                        float decay2_ms, float amplitude, struct EnvelopeProgram* p);
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

void ShapedEnvelopeSetProgram(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms,
                              float attack_shape, float decay_shape, struct ShapedEnvelopeProgram* p);
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

void FilterSetProgram(float sampling_frequency, enum Filter12dbType type, float cutoff, float resonance,
                      struct FilterProgram* p);
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

void SquareX6SetProgram(float sampling_frequency, float amplitude, float frequency1, float frequency2, float frequency3,
                        float frequency4, float frequency5, float frequency6, struct SquareX6Program* p);
void SquareX6SetState(struct SquareX6State* s);
float SquareX6Step(const struct SquareX6Program* p, struct SquareX6State* s);


float CheapDistortion(float x, float f);
float FancyDistortion(float x, float f);


struct TailProgram
{
	float c;
};

struct TailState
{
	float x;
};

void TailSetProgram(float sampling_frequency, float decay_ms, struct TailProgram* p);
void TailSetState(struct TailState* s);
void TailAccumulate(struct TailState* s, float signal);
float TailStep(struct TailProgram* p, struct TailState* s);


struct KickProgram
{
	struct ShapedEnvelopeProgram env;
	struct OscillatorProgram osc[2];
};

struct KickState
{
	struct ShapedEnvelopeState env;
	struct OscillatorState osc[2];
	float distortion;
	float click_amplify;
};

void KickSetProgram(float sampling_frequency, struct KickProgram* p);
float KickSetState(enum StateState, float sampling_frequency, float velocity, struct KickState* s);

float RenderKick(float amplify, const struct KickProgram* p, struct KickState* s, float* out, const float* out_end);
float RenderAdditiveKick(float amplify, const struct KickProgram* p, struct KickState* s, float* out,
                         const float* out_end);


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
	float distortion;
	float noise_amplify;
};

void SnareSetProgram(float sampling_frequency, struct SnareProgram* p);
float SnareSetState(enum StateState, float sampling_frequency, float velocity, struct SnareState* s);

float RenderSnare(float amplify, const struct SnareProgram* p, struct SnareState* s, float* out, const float* out_end);
float RenderAdditiveSnare(float amplify, const struct SnareProgram* p, struct SnareState* s, float* out,
                          const float* out_end);


struct HatProgram
{
	struct SquareX6Program sqr;

	struct FilterProgram bp[3]; // Bandpass is asymmetrical, 24 of highpass, 12 of lowpass,
	                            // shared between all hats and cymbal

	struct FilterProgram hp; // For noise added by distorting
	struct FilterProgram lp; // Final filter, mostly shapes white noise

	float long_gain;
	float short_gain;
	float noise_gain;

	float fade_out_in_c;
};

struct HatState
{
	struct SquareX6State sqr;
	struct FilterState bp[3];

	struct ShapedEnvelopeProgram env_long_p; // Envelope has a weird shape emulated here by using two of them,
	                                         // also, while both are exponential, one is more "lineal" than
	struct EnvelopeProgram env_short_p;      // the other, that's why the different types
	struct ShapedEnvelopeState env_long;
	struct EnvelopeState env_short;

	struct FilterState hp;
	struct FilterState lp;

	struct NoiseState noise;

	float final_amplify;
	float fade_out_in;
};

enum HatType
{
	OPEN_HAT,
	CLOSED_HAT,
	CYMBAL
};

void HatSetProgram(float sampling_frequency, enum HatType type, struct HatProgram* p);
float HatSetState(enum StateState, float sampling_frequency, enum HatType type, float velocity, struct HatState* s);

float RenderHat(float amplify, const struct HatProgram* p, struct HatState* s, float* out, const float* out_end);
float RenderAdditiveHat(float amplify, const struct HatProgram* p, struct HatState* s, float* out,
                        const float* out_end);

#endif
