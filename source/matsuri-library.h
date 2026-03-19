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


struct OscillatorSettings
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

void OscillatorInitialise(float sampling_frequency, float frequency, float decay_ms, float delay_ms, float amplitude,
                          float sweep_factor, struct OscillatorSettings* p, struct OscillatorState* s);
void OscillatorInitialiseState(float sampling_frequency, float frequency, float delay_ms, float amplitude,
                               struct OscillatorState* s);
float OscillatorStep(const struct OscillatorSettings* p, struct OscillatorState* s);


struct EnvelopeSettings
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

void EnvelopeInitialise(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms, float sustain,
                        float decay2_ms, float amplitude, struct EnvelopeSettings* p, struct EnvelopeState* s);
void EnvelopeInitialiseState(struct EnvelopeState* s);
float EnvelopeStep(const struct EnvelopeSettings* p, struct EnvelopeState* s);


struct ShapedEnvelopeSettings
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

void ShapedEnvelopeInitialise(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms,
                              float attack_shape, float decay_shape, struct ShapedEnvelopeSettings* p,
                              struct ShapedEnvelopeState* s);
void ShapedEnvelopeInitialiseState(struct ShapedEnvelopeState* s);
float ShapedEnvelopeStep(const struct ShapedEnvelopeSettings* p, struct ShapedEnvelopeState* s);


struct NoiseState
{
	uint32_t x;
};

void NoiseInitialise(uint32_t seed, struct NoiseState* s);
float NoiseStep(struct NoiseState* s);


struct FilterSettings
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

void FilterInitialise(float sampling_frequency, enum Filter12dbType type, float cutoff, float resonance,
                      struct FilterSettings* p, struct FilterState* s);
void FilterInitialiseState(struct FilterState* s);
float FilterStep(float x, const struct FilterSettings* p, struct FilterState* s);


struct SquareX6Settings
{
	int32_t step[6];
	float amplitude;
};

struct SquareX6State
{
	int32_t phase[6];
};

void SquareX6Initialise(float sampling_frequency, float amplitude, float frequency1, float frequency2, float frequency3,
                        float frequency4, float frequency5, float frequency6, struct SquareX6Settings* p,
                        struct SquareX6State* s);
void SquareX6InitialiseState(struct SquareX6State* s);
float SquareX6Step(const struct SquareX6Settings* p, struct SquareX6State* s);


float CheapDistortion(float x, float f);
float FancyDistortion(float x, float f);


struct KickSettings
{
	struct ShapedEnvelopeSettings env;
	struct OscillatorSettings osc[2];
};

struct KickState
{
	struct ShapedEnvelopeState env;
	struct OscillatorState osc[2];
};

void KickInitialise(float sampling_frequency, struct KickSettings* p, struct KickState* s);
void KickInitialiseState(float sampling_frequency, struct KickState* s);
void RenderKick(const struct KickSettings* p, struct KickState* s, float* out, const float* out_end);


struct SnareSettings
{
	struct OscillatorSettings osc;
	struct EnvelopeSettings env;
	struct FilterSettings filter[2];
};

struct SnareState
{
	struct OscillatorState osc;
	struct EnvelopeState env;
	struct NoiseState noise;
	struct FilterState filter[2];
};

void SnareInitialise(float sampling_frequency, struct SnareSettings* p, struct SnareState* s);
void SnareInitialiseState(float sampling_frequency, struct SnareState* s);
void RenderSnare(const struct SnareSettings* p, struct SnareState* s, float* out, const float* out_end);


struct HatSettings
{
	struct SquareX6Settings sqr;

	struct FilterSettings bp[2]; // Bandpass is asymmetrical, 24 of highpass, 12 of lowpass,
	                             // shared between all hats and cymbal

	struct ShapedEnvelopeSettings env_long; // Envelope has a weird shape emulated here by using two of them,
	                                        // also, while both are exponential, one is more "lineal" than
	struct EnvelopeSettings env_short;      // the other, that's why the different types

	struct FilterSettings hp; // For noise added by distorting
	struct FilterSettings lp; // Final filter, mostly shapes white noise

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

void HatInitialise(float sampling_frequency, enum HatType type, struct HatSettings* p, struct HatState* s);
void HatInitialiseState(struct HatState* s);
void RenderHat(const struct HatSettings* p, struct HatState* s, float* out, const float* out_end);

#endif
