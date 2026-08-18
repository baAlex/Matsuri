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


void yuika::Screen::Initialise()
{
	m_out = m_dummy;
	m_size = {1, 1};
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
			rect.size.w = (Clamp(rect.pos.x + rect.size.w, 0, fwend->m_size.w) - x1) * 4;
			rect.size.h = (Clamp(rect.pos.y + rect.size.h, 0, fwend->m_size.h) - y1) * fwend->m_size.w * 4;

			uint8_t* out = fwend->m_out + (x1 * 4) + (y1 * fwend->m_size.w * 4);
			for (uint8_t* row = out; row < out + rect.size.h; row += fwend->m_size.w * 4)
			{
				for (uint8_t* col = row; col < row + rect.size.w; col += 4)
				{
					col[0] = colour.a;
					col[1] = colour.b;
					col[2] = colour.g;
					col[3] = colour.r;
				}
			}
		}

		void Draw3dBevel(yuika::Rect rect, BevelStyle style) noexcept override
		{
			if (style == BevelStyle::Inset)
			{
				DrawRectangle(BEVEL_MID_COLOUR, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(BEVEL_MID_COLOUR, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(BEVEL_LIGHT_COLOUR, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(BEVEL_LIGHT_COLOUR, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});
			}
			else
			{
				DrawRectangle(BEVEL_LIGHT_COLOUR, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(BEVEL_LIGHT_COLOUR, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(BEVEL_SHADOW_COLOUR, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(BEVEL_SHADOW_COLOUR, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});

				DrawRectangle(BEVEL_MID_COLOUR, {{rect.pos.x + rect.size.w - 2, rect.pos.y + 1}, {1, rect.size.h - 2}});
				DrawRectangle(BEVEL_MID_COLOUR, {{rect.pos.x + 1, rect.pos.y + rect.size.h - 2}, {rect.size.w - 2, 1}});
			}
		}
	};
};

void yuika::Screen::Update(Size size, void* out_raw)
{
	m_out = reinterpret_cast<uint8_t*>(out_raw);

	if (m_size.w != size.w || m_size.h != size.h)
	{
		// Set stuff
		m_size = size;

		ScreenFriend::DrawApiImplementation api;
		api.fwend = this;

		api.DrawRectangle(BKG_COLOUR, {{0, 0}, m_size});
		api.Draw3dBevel({{0, 0}, m_size}, DrawApi::BevelStyle::Outset);
	}
}
