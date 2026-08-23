/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef YUIKA_HPP
#define YUIKA_HPP

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

class DrawApi
{
  public:
	enum class Colour
	{
		Black = 0,
		White = 1,
		Red = 2,
		Green = 3,
		Blue = 4,

		Background = 5,
		BevelMid = 6,
		BevelLight = White,
		BevelShadow = Black,
	};

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
	void Initialise(uint32_t r_mask, uint32_t g_mask, uint32_t b_mask);
	void Deinitialise() noexcept;
	void Update(Size size, uint32_t* out);

  private:
	friend ScreenFriend; // :)

	Size m_size;
	uint32_t* m_out;
	uint32_t m_dummy;
	uint32_t m_palette[7];
};

} // namespace yuika
#endif
