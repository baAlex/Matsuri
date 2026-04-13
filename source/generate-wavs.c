/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matsuri-library.h"


static int sSave(const char* filename, uint32_t frequency, const float* in, const float* in_end)
{
	FILE* fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		fprintf(stderr, "fopen() error\n");
		return 1;
	}

	const uint16_t channels = 1;
	const uint16_t bits_per_sample = 32;
	const uint16_t format = 3; // IEEE float

	const uint32_t data_size = (uint32_t)(in_end - in) * sizeof(float);
	const uint32_t riff_size = 36 + data_size; // Entire file size, less 8
	const uint32_t byte_rate = frequency * channels * bits_per_sample / 8;
	const uint16_t block_align = channels * bits_per_sample / 8;
	const uint32_t fmt_size = 16;

	if (
	    // RIFF
	    fwrite("RIFF", 1, 4, fp) != 4 ||     //
	    fwrite(&riff_size, 4, 1, fp) != 1 || //
	    fwrite("WAVE", 1, 4, fp) != 4 ||     //

	    // Fmt
	    fwrite("fmt ", 1, 4, fp) != 4 ||           //
	    fwrite(&fmt_size, 4, 1, fp) != 1 ||        //
	    fwrite(&format, 2, 1, fp) != 1 ||          //
	    fwrite(&channels, 2, 1, fp) != 1 ||        //
	    fwrite(&frequency, 4, 1, fp) != 1 ||       //
	    fwrite(&byte_rate, 4, 1, fp) != 1 ||       //
	    fwrite(&block_align, 2, 1, fp) != 1 ||     //
	    fwrite(&bits_per_sample, 2, 1, fp) != 1 || //

	    // Data
	    fwrite("data", 1, 4, fp) != 4 ||     //
	    fwrite(&data_size, 4, 1, fp) != 1 || //
	    fwrite(in, sizeof(float), (size_t)(in_end - in), fp) != (size_t)(in_end - in))
	{
		fprintf(stderr, "fwrite() error\n");
		fclose(fp);
		return 1;
	}

	fclose(fp);
	return 0;
}


#define FREQUENCY 44100
#define BUFFER_LEN (FREQUENCY * 4)
static float s_buffer[BUFFER_LEN];


static size_t sMillisecondsToSamples(float sampling_frequency, float milliseconds)
{
	return (size_t)((milliseconds * sampling_frequency) / 1000.0f);
}


#define VEL_VOL_MOD 0.0f
#define VEL_TONE_MOD 1.0f
#define REFERENCE_VEL 0.5f

int main(void)
{
	if (1)
	{
		printf("Kick...\n");

		struct mtsr606_KickProgram p;
		struct mtsr606_KickState s;
		float duration;
		size_t samples;

		mtsr606_KickSetProgram((float)(FREQUENCY), &p);

		duration = mtsr606_KickSetState(MTSR_STATE_START, (float)(FREQUENCY), 0.5f, VEL_VOL_MOD, VEL_TONE_MOD,
		                                REFERENCE_VEL, &s); // Normal
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderKick(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-bass-drum.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration =
		    mtsr606_KickSetState(MTSR_STATE_START, (float)(FREQUENCY), 0.75f, VEL_VOL_MOD, VEL_TONE_MOD, REFERENCE_VEL,
		                         &s); // Mid velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderKick(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-bass-drum-mid.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration =
		    mtsr606_KickSetState(MTSR_STATE_START, (float)(FREQUENCY), 1.0f, VEL_VOL_MOD, VEL_TONE_MOD, REFERENCE_VEL,
		                         &s); // Max velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderKick(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-bass-drum-max.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		printf("Snare...\n");

		struct mtsr606_SnareProgram p;
		struct mtsr606_SnareState s;
		float duration;
		size_t samples;

		mtsr606_SnareSetProgram((float)(FREQUENCY), &p);

		duration = mtsr606_SnareSetState(MTSR_STATE_START, (float)(FREQUENCY), 666, 0.5f, VEL_VOL_MOD, VEL_TONE_MOD,
		                                 REFERENCE_VEL,
		                                 &s); // Normal
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderSnare(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-snare-drum.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_SnareSetState(MTSR_STATE_START, (float)(FREQUENCY), 666, 0.75f, VEL_VOL_MOD, VEL_TONE_MOD,
		                                 REFERENCE_VEL,
		                                 &s); // Mid velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderSnare(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-snare-drum-mid.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_SnareSetState(MTSR_STATE_START, (float)(FREQUENCY), 666, 1.0f, VEL_VOL_MOD, VEL_TONE_MOD,
		                                 REFERENCE_VEL,
		                                 &s); // Max velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderSnare(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-snare-drum-max.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		printf("Open Hat...\n");

		struct mtsr606_HatProgram p;
		struct mtsr606_HatState s;
		float duration;
		size_t samples;

		mtsr606_HatSetProgram((float)(FREQUENCY), MTSR_OPEN_HAT, &p);

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_OPEN_HAT, 666, 0.5f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Normal
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-hat-open.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_OPEN_HAT, 666, 0.75f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Mid velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-hat-open-mid.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_OPEN_HAT, 666, 1.0f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Max velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-hat-open-max.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		printf("Closed Hat...\n");

		struct mtsr606_HatProgram p;
		struct mtsr606_HatState s;
		float duration;
		size_t samples;

		mtsr606_HatSetProgram((float)(FREQUENCY), MTSR_CLOSED_HAT, &p);

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_CLOSED_HAT, 666, 0.5f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Normal
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-hat-closed.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_CLOSED_HAT, 666, 0.75f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Mid velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);
		if (sSave("606-hat-closed-mid.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_CLOSED_HAT, 666, 1.0f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Max velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);
		if (sSave("606-hat-closed-max.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		printf("Cymbal...\n");

		struct mtsr606_HatProgram p;
		struct mtsr606_HatState s;
		float duration;
		size_t samples;

		mtsr606_HatSetProgram((float)(FREQUENCY), MTSR_CYMBAL, &p);

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_CYMBAL, 666, 0.5f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Normal
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-cymbal.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_CYMBAL, 666, 0.75f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Mid velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);
		if (sSave("606-cymbal-mid.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_HatSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_CYMBAL, 666, 1.0f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Max velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderHat(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);
		if (sSave("606-cymbal-max.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		printf("Low Tom...\n");

		struct mtsr606_TomProgram p;
		struct mtsr606_TomState s;
		float duration;
		size_t samples;

		mtsr606_TomSetProgram((float)(FREQUENCY), MTSR_LOW_TOM, &p);

		duration = mtsr606_TomSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_LOW_TOM, 0.5f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL,
		                               &s); // Normal
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderTom(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-tom-low.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_TomSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_LOW_TOM, 0.75f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Mid velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderTom(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);
		if (sSave("606-tom-low-mid.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_TomSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_LOW_TOM, 1.0f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL,
		                               &s); // Max velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderTom(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);
		if (sSave("606-tom-low-max.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		printf("High Tom...\n");

		struct mtsr606_TomProgram p;
		struct mtsr606_TomState s;
		float duration;
		size_t samples;

		mtsr606_TomSetProgram((float)(FREQUENCY), MTSR_HIGH_TOM, &p);

		duration = mtsr606_TomSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_HIGH_TOM, 0.5f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Normal
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderTom(1.0f, &p, &s, s_buffer, s_buffer + samples);
		if (sSave("606-tom-high.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_TomSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_HIGH_TOM, 0.75f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Mid velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderTom(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);
		if (sSave("606-tom-high-mid.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;

		duration = mtsr606_TomSetState(MTSR_STATE_START, (float)(FREQUENCY), MTSR_HIGH_TOM, 1.0f, VEL_VOL_MOD,
		                               VEL_TONE_MOD, REFERENCE_VEL, &s); // Max velocity
		samples = sMillisecondsToSamples((float)(FREQUENCY), duration);
		mtsr606_RenderTom(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);
		if (sSave("606-tom-high-max.wav", FREQUENCY, s_buffer, s_buffer + samples) != 0)
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
