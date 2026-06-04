/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <assert.h>

#include "canvas.hpp"
#include "misc.hpp"


int main()
{
	auto canvas = Canvas::Create(640, 480, 1.0f / 16.0f);

	// Grey background, to check that it is 640 and not 639, same with 480
	// (valgrind will scream that memory isn't initialised if not the case)
	canvas->DrawRectangle({{0.0f, 0.0f}, {640.0f, 480.0f}}, COLOUR16_GREY);

	// Three of same size, they should have, in theory, the same size
	canvas->Draw3dBevel({{16.0f, 16.0f}, {32.0f, 32.0f}});
	canvas->Draw3dBevel({{48.0f, 16.0f}, {32.0f, 32.0f}});

	canvas->DrawRectangle({{16.0f, 64.0f}, {32.0f, 32.0f}}, COLOUR16_RED);

	// Against edge and corner (latter shouldn't make valgrind angry by writing outside memory)
	canvas->DrawRectangle({{640.0f - 16.0f, 16.0f}, {32.0f, 32.0f}}, COLOUR16_BLUE);
	canvas->DrawRectangle({{640.0f - 16.0f, 480.0f - 16.0f}, {32.0f, 32.0f}}, COLOUR16_GREEN);

	// Outline against edges
	{
		// Top
		canvas->Draw3dBevel({{320.0f - 16.0f, 0.0f}, {24.0f, 24.0f}});
		canvas->Draw3dBevel({{320.0f + 16.0f, -1.0f}, {24.0f, 24.0f}});

		// Left
		canvas->Draw3dBevel({{0.0f, 240.0f - 16.0f}, {24.0f, 24.0f}});
		canvas->Draw3dBevel({{-1.0f, 240.0f + 16.0f}, {24.0f, 24.0f}});

		// Bottom
		canvas->Draw3dBevel({{320.0f - 16.0f, 480.0f - 24.0f}, {24.0f, 24.0f}});
		canvas->Draw3dBevel({{320.0f + 16.0f, 480.0f - 23.0f}, {24.0f, 24.0f}});

		// Right
		canvas->Draw3dBevel({{640.0f - 24.0f, 240.0f - 16.0f}, {24.0f, 24.0f}});
		canvas->Draw3dBevel({{640.0f - 23.0f, 240.0f + 16.0f}, {24.0f, 24.0f}});
	}

	// Bye!
	SaveBMP16("test.bmp", 640, 480, canvas->GetBuffer());

	return 0;
}
