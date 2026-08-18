/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef MATSURI_UI_YUIKA_HPP
#define MATSURI_UI_YUIKA_HPP

#include <stddef.h>
#include <stdint.h>

namespace yuika
{

struct Position
{
	int x, y;
};

struct Delta
{
	int x, y;
};

struct Size
{
	int w, h;
};

struct Rect
{
	Position pos;
	Size size;
};

struct Colour
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

static constexpr Colour BLACK = {0xFF, 0x00, 0x00, 0x00};
static constexpr Colour WHITE = {0xFF, 0xFF, 0xFF, 0xFF};
static constexpr Colour RED = {0xFF, 0xFF, 0x00, 0x00};
static constexpr Colour GREEN = {0xFF, 0x00, 0xFF, 0x00};
static constexpr Colour BLUE = {0xFF, 0x00, 0x00, 0xFF};

static constexpr Colour BKG_COLOUR = {0xFF, 0xE6, 0x28, 0x28};

static constexpr Colour BEVEL_LIGHT_COLOUR = WHITE;
static constexpr Colour BEVEL_SHADOW_COLOUR = BLACK;
static constexpr Colour BEVEL_MID_COLOUR = {0xFF, 0x73, 0x14, 0x14}; // BKG_COLOUR / 2

class DrawApi
{
  public:
	virtual void SetClickableArea(Rect rect) = 0;
	virtual void DrawRectangle(Colour colour, Rect rect) = 0;

	enum class BevelStyle
	{
		Inset,
		Outset
	};

	virtual void Draw3dBevel(Rect rect, BevelStyle style) = 0;
};


class ScreenFriend;
class Screen
{
  public:
	void Initialise();
	void Deinitialise() noexcept;
	void Update(Size size, void* out);

  private:
	friend ScreenFriend; // :)

	uint8_t* m_out;
	Size m_size;

	uint8_t m_dummy[4];
};

} // namespace yuika

#endif
