/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef CANVAS_HPP
#define CANVAS_HPP

#include "misc.hpp"

struct Colour16
{
	uint16_t m_v;

	static constexpr Colour16 Set(float r, float g, float b, float a)
	{
		return {static_cast<uint16_t>((static_cast<uint16_t>(Clamp(r, 0.0f, 1.0f) * 31.0f) << 11) | //
		                              (static_cast<uint16_t>(Clamp(g, 0.0f, 1.0f) * 31.0f) << 6) |  //
		                              (static_cast<uint16_t>(Clamp(b, 0.0f, 1.0f) * 31.0f) << 1) |  //
		                              (static_cast<uint16_t>((a > 0.5f) ? 1.0f : 0.0f) << 0))};
	}

	static Colour16 MixToBlack(Colour16 c, uint8_t t)
	{
		// const int r = ((((c.m_v >> 11) & 0b11111) << 3) * t) >> 11;
		// const int g = ((((c.m_v >> 6) & 0b11111) << 3) * t) >> 11;
		// const int b = ((((c.m_v >> 1) & 0b11111) << 3) * t) >> 11;
		// const int a = (c.m_v >> 0) & 1;

		const int r = (((c.m_v >> 8) & 255) * t) >> 11;
		const int g = (((c.m_v >> 3) & 255) * t) >> 11;
		const int b = (((c.m_v << 2) & 255) * t) >> 11;
		const int a = c.m_v & 1;

		return {static_cast<uint16_t>((static_cast<uint16_t>(r) << 11) | //
		                              (static_cast<uint16_t>(g) << 6) |  //
		                              (static_cast<uint16_t>(b) << 1) |  //
		                              (static_cast<uint16_t>(a)))};
	}
};

static constexpr auto COLOUR16_WHITE = Colour16::Set(1.0f, 1.0f, 1.0f, 1.0f);
static constexpr auto COLOUR16_BLACK = Colour16::Set(0.0f, 0.0f, 0.0f, 1.0f);
static constexpr auto COLOUR16_GREY = Colour16::Set(0.75f, 0.75f, 0.75f, 1.0f);
static constexpr auto COLOUR16_DARK_GREY = Colour16::Set(0.5f, 0.5f, 0.5f, 1.0f);
static constexpr auto COLOUR16_RED = Colour16::Set(1.0f, 0.0f, 0.0f, 1.0f);
static constexpr auto COLOUR16_GREEN = Colour16::Set(0.0f, 1.0f, 0.0f, 1.0f);
static constexpr auto COLOUR16_BLUE = Colour16::Set(0.0f, 0.0f, 1.0f, 1.0f);

class Canvas
{
  public:
	static Canvas* Create(int width, int height, float em_scale);
	const uint16_t* GetBuffer() const;

	void DrawRectangle(Rect rect, Colour16 colour);
	void DrawText(Position pos, const char* text, Colour16 colour);
	void Draw3dBevel(Rect rect);

	Size GetTextSize(const char* text) const;

  protected:
	Canvas(int width, int height, float em_scale);
	Colour16* m_buffer;
	float m_em_scale;
	int m_width;
	int m_height;
};

#endif
