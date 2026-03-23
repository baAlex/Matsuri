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
#define BUFFER_LEN (FREQUENCY * 2)
static float s_buffer[BUFFER_LEN];


int main(void)
{
	if (1)
	{
		struct KickProgram p;
		struct KickState s;

		KickSet(STATE_START, (float)(FREQUENCY), 0.5f, &p, &s); // Normal
		RenderKick(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN / 4);
		if (sSave("new-kick.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 4) != 0)
			return EXIT_FAILURE;

		KickSetState(STATE_START, (float)(FREQUENCY), 0.75f, &s); // Mid velocity
		RenderKick(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN / 4);
		if (sSave("new-kick-mid.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 4) != 0)
			return EXIT_FAILURE;

		KickSetState(STATE_START, (float)(FREQUENCY), 1.0f, &s); // Max velocity
		RenderKick(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN / 4);
		if (sSave("new-kick-max.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 4) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		struct SnareProgram p;
		struct SnareState s;

		SnareSet(STATE_START, (float)(FREQUENCY), 0.5f, &p, &s); // Normal
		RenderSnare(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN / 8);
		if (sSave("new-snare.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 8) != 0)
			return EXIT_FAILURE;

		SnareSetState(STATE_START, (float)(FREQUENCY), 0.75f, &s); // Mid velocity
		RenderSnare(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN / 8);
		if (sSave("new-snare-mid.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 8) != 0)
			return EXIT_FAILURE;

		SnareSetState(STATE_START, (float)(FREQUENCY), 1.0f, &s); // Max velocity
		RenderSnare(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN / 8);
		if (sSave("new-snare-max.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 8) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		struct HatProgram p;
		struct HatState s;

		HatSet(STATE_START, (float)(FREQUENCY), 0.5f, OPEN_HAT, &p, &s);
		RenderHat(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN);

		if (sSave("new-hat-open.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		struct HatProgram p;
		struct HatState s;

		HatSet(STATE_START, (float)(FREQUENCY), 0.5f, CLOSED_HAT, &p, &s);
		RenderHat(1.0f, &p, &s, s_buffer, s_buffer + BUFFER_LEN / 8);

		if (sSave("new-hat-closed.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 8) != 0)
			return EXIT_FAILURE;
	}

#ifndef NDEBUG
	if (1)
	{
		struct KickProgram p;
		struct KickState s;
		struct TailProgram tail_p;
		struct TailState tail_s;

		KickSet(STATE_START, (float)(FREQUENCY), 0.5f, &p, &s);
		TailSetProgram((float)(FREQUENCY), 10.0f, &tail_p);
		TailSetState(&tail_s);

		memset(s_buffer, 0, sizeof(float) * BUFFER_LEN);
		const float last_signal =
		    RenderKick(1.0f, &p, &s, s_buffer, s_buffer + 2460); // Fewer samples, cut in the middle

		TailAccumulate(&tail_s, last_signal);

		if (1)
		{
			for (size_t i = 2460; i < BUFFER_LEN / 4; i += 1)
				s_buffer[i] = TailStep(&tail_p, &tail_s);
		}

		if (sSave("tail-test1.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 4) != 0)
			return EXIT_FAILURE;
	}

	if (1)
	{
		struct SnareProgram p;
		struct SnareState s;
		struct TailProgram tail_p;
		struct TailState tail_s;

		SnareSet(STATE_START, (float)(FREQUENCY), 0.5f, &p, &s);
		TailSetProgram((float)(FREQUENCY), 10.0f, &tail_p);
		TailSetState(&tail_s);

		memset(s_buffer, 0, sizeof(float) * BUFFER_LEN);
		const float last_signal =
		    RenderSnare(1.0f, &p, &s, s_buffer, s_buffer + 910); // Fewer samples, cut in the middle

		TailAccumulate(&tail_s, last_signal);

		if (1)
		{
			for (size_t i = 910; i < BUFFER_LEN / 8; i += 1)
				s_buffer[i] = TailStep(&tail_p, &tail_s);
		}

		if (sSave("tail-test2.wav", FREQUENCY, s_buffer, s_buffer + BUFFER_LEN / 8) != 0)
			return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}
