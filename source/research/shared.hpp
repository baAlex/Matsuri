/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef SHARED_HPP
#define SHARED_HPP

#include <stddef.h>
#include <stdint.h>


// clang-format off
template <typename T> constexpr T Min(T a, T b) { return (a < b) ? a : b; }
template <typename T> constexpr T Max(T a, T b) { return (a > b) ? a : b; }
template <typename T> constexpr T Clamp(T x, T min, T max) { return Min(Max(x, min), max); }
// clang-format on


// My first time differentiating vectors based on their use:
// (I remember a Naughty Dog's developer article on this)
// (so far I'm not buying the idea)

struct Size
{
	float w; // In EM
	float h; // In EM
};

struct Position
{
	float x; // In EM
	float y; // In EM
};

struct Delta
{
	float x; // In EM
	float y; // In EM
};

struct Rect
{
	// I want to tell the compiler that these are contiguous:
	// (that's why Rect exists)(... and it's for SIMD purposes)
	Position pos;
	Size size;
};

static const int FONTS_NO = 2;
enum Font
{
	Normal,
	Small
};

struct Colour
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	static constexpr Colour Set(float r, float g, float b, float a) noexcept
	{
		return {static_cast<uint8_t>(Clamp(r, 0.0f, 1.0f) * 255.0f), //
		        static_cast<uint8_t>(Clamp(g, 0.0f, 1.0f) * 255.0f), //
		        static_cast<uint8_t>(Clamp(b, 0.0f, 1.0f) * 255.0f), //
		        static_cast<uint8_t>(Clamp(a, 0.0f, 1.0f) * 255.0f)};
	}

	static Colour Mix(Colour c1, Colour c2, uint8_t t) noexcept
	{
		// clang-format off
		const int16_t r = static_cast<int16_t>(c1.r) + ((static_cast<int16_t>(c2.r) - static_cast<int16_t>(c1.r)) * static_cast<int16_t>(t) >> 8);
		const int16_t g = static_cast<int16_t>(c1.g) + ((static_cast<int16_t>(c2.g) - static_cast<int16_t>(c1.g)) * static_cast<int16_t>(t) >> 8);
		const int16_t b = static_cast<int16_t>(c1.b) + ((static_cast<int16_t>(c2.b) - static_cast<int16_t>(c1.b)) * static_cast<int16_t>(t) >> 8);
		const int16_t a = static_cast<int16_t>(c1.a) + ((static_cast<int16_t>(c2.a) - static_cast<int16_t>(c1.a)) * static_cast<int16_t>(t) >> 8);
		// clang-format on

		return {static_cast<uint8_t>(r), //
		        static_cast<uint8_t>(g), //
		        static_cast<uint8_t>(b), //
		        static_cast<uint8_t>(a)};
	}
};

static constexpr auto COLOUR_WHITE = Colour::Set(1.0f, 1.0f, 1.0f, 1.0f);
static constexpr auto COLOUR_BLACK = Colour::Set(0.0f, 0.0f, 0.0f, 1.0f);
static constexpr auto COLOUR_GREY = Colour::Set(0.75f, 0.75f, 0.75f, 1.0f);
static constexpr auto COLOUR_DARK_GREY = Colour::Set(0.5f, 0.5f, 0.5f, 1.0f);
static constexpr auto COLOUR_RED = Colour::Set(1.0f, 0.0f, 0.0f, 1.0f);
static constexpr auto COLOUR_GREEN = Colour::Set(0.0f, 1.0f, 0.0f, 1.0f);
static constexpr auto COLOUR_BLUE = Colour::Set(0.0f, 0.0f, 1.0f, 1.0f);


class DrawAPI
{
  public:
	virtual void Draw3dBevel(Rect rect) = 0;
	virtual void DrawText(Font font, Position pos, const char* text, Colour colour) = 0;
	virtual Size GetTextSize(Font font, const char* text) const = 0;
};


int SaveBMP(const char* filename, int width, int height, const Colour* buffer);

#endif
