/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "yuika.hpp"

#if 1
#define DEBUGPRINT(...) __builtin_printf(__VA_ARGS__)
#else
#define DEBUGPRINT(...) // Empty
#endif

template <typename T> static T Min(T a, T b) noexcept
{
	return (a < b) ? a : b;
}
template <typename T> static T Max(T a, T b) noexcept
{
	return (a > b) ? a : b;
}
template <typename T> static T Clamp(T v, T min, T max) noexcept
{
	return Min(Max(v, min), max);
}


#if defined(__clang__) || defined(__GNUC__)
static int sFindLastSet(uint32_t v) noexcept
{
	return v == 0 ? 0 : 32 - __builtin_clz(v);
}
#elif defined(_MSC_VER)
#include <intrin.h>
static int sFindLastSet(uint32_t v) noexcept
{
	unsigned long index;
	return _BitScanReverse(&index, v) ? static_cast<int>(index + 1) : 0;
}
#else
static int sClz(uint32_t x) noexcept
{
	// https://blog.stephencleary.com/2010/10/implementing-gccs-builtin-functions.html
	int n = 32;
	uint32_t y;

	// clang-format off
	y = x >>16; if (y != 0) {n = n -16; x = y;}
	y = x >> 8; if (y != 0) {n = n - 8; x = y;}
	y = x >> 4; if (y != 0) {n = n - 4; x = y;}
	y = x >> 2; if (y != 0) {n = n - 2; x = y;}
	y = x >> 1; if (y != 0) return n - 2;
	// clang-format on

	return n - static_cast<int>(x);
}
static int sFindLastSet(uint32_t v) noexcept
{
	return v == 0 ? 0 : 32 - sClz(v);
}
#endif


void yuika::Screen::Initialise(uint32_t r_mask, uint32_t g_mask, uint32_t b_mask)
{
	m_size = {1, 1};
	m_out = &m_dummy;

	auto set_colour = [&](DrawApi::Colour colour, uint8_t r, uint8_t g, uint8_t b)
	{
		const int rl = sFindLastSet(r_mask);
		const int gl = sFindLastSet(g_mask);
		const int bl = sFindLastSet(b_mask);

		uint32_t* c = m_palette + static_cast<int>(colour);
		*c = 0;
		*c = *c | (static_cast<uint32_t>((rl >= 8) ? (r << (rl - 8)) : (r >> (8 - rl))) & r_mask);
		*c = *c | (static_cast<uint32_t>((gl >= 8) ? (g << (gl - 8)) : (g >> (8 - gl))) & g_mask);
		*c = *c | (static_cast<uint32_t>((bl >= 8) ? (b << (bl - 8)) : (b >> (8 - bl))) & b_mask);
	};

	set_colour(DrawApi::Colour::Black, 0x00, 0x00, 0x00);
	set_colour(DrawApi::Colour::White, 0xFF, 0xFF, 0xFF);
	set_colour(DrawApi::Colour::Red, 0xFF, 0x00, 0x00);
	set_colour(DrawApi::Colour::Green, 0x00, 0xFF, 0x00);
	set_colour(DrawApi::Colour::Blue, 0x00, 0x00, 0xFF);

	set_colour(DrawApi::Colour::Background, 0xE6, 0x28, 0x28);
	set_colour(DrawApi::Colour::BevelMid, 0xE6 / 2, 0x28 / 2, 0x28 / 2);
}

void yuika::Screen::Deinitialise() noexcept {}


class yuika::ScreenFriend
{
  public:
	class DrawApiImplementation final : public yuika::DrawApi
	{
	  public:
		yuika::Screen* fwend;

		void SetClickableArea(yuika::Rect) noexcept override {}

		void DrawRectangle(Colour colour, yuika::Rect rect) noexcept override
		{
			const int x1 = Clamp(rect.pos.x, 0, fwend->m_size.w);
			const int y1 = Clamp(rect.pos.y, 0, fwend->m_size.h);
			rect.size.w = (Clamp(rect.pos.x + rect.size.w, 0, fwend->m_size.w) - x1);
			rect.size.h = (Clamp(rect.pos.y + rect.size.h, 0, fwend->m_size.h) - y1) * fwend->m_size.w;

			uint32_t* out = fwend->m_out + static_cast<size_t>(x1 + y1 * fwend->m_size.w);
			for (uint32_t* row = out; row < out + rect.size.h; row += static_cast<size_t>(fwend->m_size.w))
			{
				for (uint32_t* col = row; col < row + rect.size.w; col += 1)
				{
					*col = fwend->m_palette[static_cast<int>(colour)];
				}
			}
		}

		void Draw3dBevel(yuika::Rect rect, BevelStyle style) noexcept override
		{
			if (style == BevelStyle::Inset)
			{
				DrawRectangle(Colour::BevelMid, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(Colour::BevelMid, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(Colour::BevelLight, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(Colour::BevelLight, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});
			}
			else
			{
				DrawRectangle(Colour::BevelLight, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(Colour::BevelLight, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(Colour::BevelShadow, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(Colour::BevelShadow, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});

				DrawRectangle(Colour::BevelMid, {{rect.pos.x + rect.size.w - 2, rect.pos.y + 1}, {1, rect.size.h - 2}});
				DrawRectangle(Colour::BevelMid, {{rect.pos.x + 1, rect.pos.y + rect.size.h - 2}, {rect.size.w - 2, 1}});
			}
		}
	};
};

void yuika::Screen::Update(Size size, uint32_t* out)
{
	m_out = out;

	if (m_size.w != size.w || m_size.h != size.h)
	{
		// Set stuff
		m_size = size;

		ScreenFriend::DrawApiImplementation api;
		api.fwend = this;

		api.DrawRectangle(DrawApi::Colour::Background, {{0, 0}, m_size});
		api.Draw3dBevel({{0, 0}, m_size}, DrawApi::BevelStyle::Outset);
	}
}
