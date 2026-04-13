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


enum mtsr_StateState
{
	MTSR_STATE_START,
	MTSR_STATE_DEAD // Completely dead, an hard stop, not a release
};


struct mtsr_OscillatorProgram
{
	float zeta;
	float sweep_step;
	float sweep_target;
};

struct mtsr_OscillatorState
{
	int delay;
	float omega;
	float v;
	float x;
	float sweep;
};

void mtsr_OscillatorSetProgram(float sampling_frequency, float decay_ms, float sweep_factor, float sweep_decay_ms,
                               struct mtsr_OscillatorProgram* p);
void mtsr_OscillatorSetState(enum mtsr_StateState, float sampling_frequency, float frequency, float delay_ms,
                             float volume, struct mtsr_OscillatorState* s);
float mtsr_OscillatorStep(const struct mtsr_OscillatorProgram* p, struct mtsr_OscillatorState* s);


struct mtsr_EnvelopeProgram
{
	float durations[4];
	float levels[4];
};

struct mtsr_EnvelopeState
{
	uint32_t x;
	float y;
	uint8_t stage;
};

void mtsr_EnvelopeSetProgram(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms, float sustain,
                             float decay2_ms, float volume, struct mtsr_EnvelopeProgram* p);
void mtsr_EnvelopeSetState(enum mtsr_StateState, struct mtsr_EnvelopeState* s);
float mtsr_EnvelopeStep(const struct mtsr_EnvelopeProgram* p, struct mtsr_EnvelopeState* s);


struct mtsr_ShapedEnvelopeProgram
{
	uint32_t durations[4];
	float steps[4];
	float shapes[2];
};

struct mtsr_ShapedEnvelopeState
{
	uint32_t x;
	float y;
	uint8_t stage;
};

void mtsr_ShapedEnvelopeSetProgram(float sampling_frequency, float delay_ms, float attack_ms, float decay_ms,
                                   float attack_shape, float decay_shape, struct mtsr_ShapedEnvelopeProgram* p);
void mtsr_ShapedEnvelopeSetState(enum mtsr_StateState, struct mtsr_ShapedEnvelopeState* s);
float mtsr_ShapedEnvelopeStep(const struct mtsr_ShapedEnvelopeProgram* p, struct mtsr_ShapedEnvelopeState* s);


struct mtsr_NoiseState
{
	uint32_t x;
};

void mtsr_NoiseSet(uint32_t seed, struct mtsr_NoiseState* s);
float mtsr_NoiseStep(struct mtsr_NoiseState* s);


struct mtsr_FilterProgram
{
	float c[4];
	float c_b0;
};

struct mtsr_FilterState
{
	float s[4];
};

enum mtsr_FilterType
{
	MTSR_LOWPASS_12DB,
	MTSR_HIGHPASS_12DB,
	MTSR_BANDPASS_12DB,
	MTSR_RC_LOWPASS_6DB,
	MTSR_RC_HIGHPASS_6DB,
	MTSR_RC_LOWPASS_12DB
};

void mtsr_FilterSetProgram(float sampling_frequency, enum mtsr_FilterType type, float cutoff, float resonance,
                           float volume, struct mtsr_FilterProgram* p);
void mtsr_FilterSetState(struct mtsr_FilterState* s);
float mtsr_FilterStep(float x, const struct mtsr_FilterProgram* p, struct mtsr_FilterState* s);


struct mtsr_SquareX6Program
{
	int32_t step[6];
	float volume;
};

struct mtsr_SquareX6State
{
	int32_t phase[6];
};

void mtsr_SquareX6SetProgram(float sampling_frequency, float volume, float frequency1, float frequency2,
                             float frequency3, float frequency4, float frequency5, float frequency6,
                             struct mtsr_SquareX6Program* p);
void mtsr_SquareX6SetState(uint32_t seed, struct mtsr_SquareX6State* s);
float mtsr_SquareX6Step(const struct mtsr_SquareX6Program* p, struct mtsr_SquareX6State* s);


float mtsr_CheapDistortion(float x, float f);
float mtsr_FancyDistortion(float x, float f);


struct mtsr_TailProgram
{
	float c;
};

struct mtsr_TailState
{
	float x;
};

void mtsr_TailSetProgram(float sampling_frequency, float decay_ms, struct mtsr_TailProgram* p);
void mtsr_TailSetState(struct mtsr_TailState* s);
void mtsr_TailAccumulate(struct mtsr_TailState* s, float signal);
float mtsr_TailStep(struct mtsr_TailProgram* p, struct mtsr_TailState* s);


struct mtsr606_KickProgram
{
	struct mtsr_ShapedEnvelopeProgram env;
	struct mtsr_OscillatorProgram osc[2];
};

struct mtsr606_KickState
{
	struct mtsr_ShapedEnvelopeState env;
	struct mtsr_OscillatorState osc[2];
	float distortion;
	float click_volume;
};

void mtsr606_KickSetProgram(float sampling_frequency, struct mtsr606_KickProgram* p);
float mtsr606_KickSetState(enum mtsr_StateState, float sampling_frequency, float velocity, float vel_vol_mod,
                           float vel_tone_mod, float reference_vel, struct mtsr606_KickState* s);

float mtsr606_RenderKick(float volume, const struct mtsr606_KickProgram* p, struct mtsr606_KickState* s, float* out,
                         const float* out_end);
float mtsr606_RenderAdditiveKick(float volume, const struct mtsr606_KickProgram* p, struct mtsr606_KickState* s,
                                 float* out, const float* out_end);


struct mtsr606_SnareProgram
{
	struct mtsr_OscillatorProgram osc;
	struct mtsr_EnvelopeProgram env;
	struct mtsr_FilterProgram filter[2];
};

struct mtsr606_SnareState
{
	struct mtsr_OscillatorState osc;
	struct mtsr_EnvelopeState env;
	struct mtsr_NoiseState noise;
	struct mtsr_FilterState filter[2];
	float distortion;
	float noise_volume;
};

void mtsr606_SnareSetProgram(float sampling_frequency, struct mtsr606_SnareProgram* p);
float mtsr606_SnareSetState(enum mtsr_StateState, float sampling_frequency, uint32_t seed, float velocity,
                            float vel_vol_mod, float vel_tone_mod, float reference_vel, struct mtsr606_SnareState* s);

float mtsr606_RenderSnare(float volume, const struct mtsr606_SnareProgram* p, struct mtsr606_SnareState* s, float* out,
                          const float* out_end);
float mtsr606_RenderAdditiveSnare(float volume, const struct mtsr606_SnareProgram* p, struct mtsr606_SnareState* s,
                                  float* out, const float* out_end);


enum mtsr_HatType
{
	MTSR_OPEN_HAT,
	MTSR_CLOSED_HAT,
	MTSR_CYMBAL
};

struct mtsr606_HatProgram
{
	struct mtsr_SquareX6Program sqr;

	struct mtsr_FilterProgram bp[3]; // Bandpass is asymmetrical, 24 of highpass, 12 of lowpass,
	                                 // shared between all hats and cymbal

	struct mtsr_FilterProgram hp; // For noise added by distorting
	struct mtsr_FilterProgram lp; // Final filter, mostly shapes white noise
};

struct mtsr606_HatState
{
	struct mtsr_SquareX6State sqr;
	struct mtsr_FilterState bp[3];

	struct mtsr_ShapedEnvelopeProgram env_long_p; // Envelope has a weird shape emulated here by using two of them,
	                                              // also, while both are exponential, one is more "lineal" than
	struct mtsr_EnvelopeProgram env_short_p;      // the other, that's why the different types
	struct mtsr_ShapedEnvelopeState env_long;
	struct mtsr_EnvelopeState env_short;

	struct mtsr_FilterState hp;
	struct mtsr_FilterState lp;

	struct mtsr_NoiseState noise;

	float noise_volume;
	float long_volume;
	float final_volume;
	float fade_out_in;
	float fade_out_in_c;
};

void mtsr606_HatSetProgram(float sampling_frequency, enum mtsr_HatType type, struct mtsr606_HatProgram* p);
float mtsr606_HatSetState(enum mtsr_StateState, float sampling_frequency, enum mtsr_HatType type, uint32_t seed,
                          float velocity, float vel_vol_mod, float vel_tone_mod, float reference_vel,
                          struct mtsr606_HatState* s);

float mtsr606_RenderHat(float volume, const struct mtsr606_HatProgram* p, struct mtsr606_HatState* s, float* out,
                        const float* out_end);
float mtsr606_RenderAdditiveHat(float volume, const struct mtsr606_HatProgram* p, struct mtsr606_HatState* s,
                                float* out, const float* out_end);


enum mtsr_TomType
{
	MTSR_LOW_TOM,
	MTSR_HIGH_TOM
};

struct mtsr606_TomProgram
{
	struct mtsr_ShapedEnvelopeProgram env;
	struct mtsr_OscillatorProgram osc;
};

struct mtsr606_TomState
{
	float click_volume;
	struct mtsr_ShapedEnvelopeState env;
	struct mtsr_OscillatorState osc;
};

void mtsr606_TomSetProgram(float sampling_frequency, enum mtsr_TomType type, struct mtsr606_TomProgram* p);
float mtsr606_TomSetState(enum mtsr_StateState, float sampling_frequency, enum mtsr_TomType type, float velocity,
                          float vel_vol_mod, float vel_tone_mod, float reference_vel, struct mtsr606_TomState* s);

float mtsr606_RenderTom(float volume, const struct mtsr606_TomProgram* p, struct mtsr606_TomState* s, float* out,
                        const float* out_end);
float mtsr606_RenderAdditiveTom(float volume, const struct mtsr606_TomProgram* p, struct mtsr606_TomState* s,
                                float* out, const float* out_end);
#endif
