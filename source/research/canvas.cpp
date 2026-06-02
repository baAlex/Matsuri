/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "canvas.hpp"


Canvas* Canvas::Create(int width, int height, float em_scale)
{
	return new Canvas(width, height, em_scale);
}

Canvas::Canvas(int width, int height, float em_scale)
{
	m_em_scale = em_scale;
	m_width = width;
	m_height = height;
	m_buffer = reinterpret_cast<Colour16*>(
	    malloc(sizeof(Colour16) * static_cast<size_t>(width) * static_cast<size_t>(height)));
}

const uint16_t* Canvas::GetBuffer() const
{
	return reinterpret_cast<const uint16_t*>(m_buffer);
}


static float sScale(float x, float em_scale)
{
	return floorf(x * em_scale + 0.5f); // round() is expensive
}

static void sDrawRectangle(float em_scale, int buffer_w, int buffer_h, Rect rect, Colour16 colour, Colour16* buffer)
{
	// PROTIP, is better to do the scaling at the last instance possible, near int conversion.
	// Otherwise round errors accumulate, very quickly everywhere

	const int x1 = Clamp(static_cast<int>(sScale(rect.pos.x, em_scale)), 0, buffer_w);
	const int y1 = Clamp(static_cast<int>(sScale(rect.pos.y, em_scale)), 0, buffer_h);
	const int x2 = Clamp(static_cast<int>(sScale(rect.pos.x + rect.size.w, em_scale)), 0, buffer_w);
	const int y2 = Clamp(static_cast<int>(sScale(rect.pos.y + rect.size.h, em_scale)), 0, buffer_h);

	for (Colour16* row = (buffer + buffer_w * y1); row < (buffer + buffer_w * y2); row += buffer_w)
	{
		for (int col = x1; col < x2; col += 1)
			row[col] = colour;
	}
}


static constexpr float SPACING = 1.0f / 20.0f; // TODO, hardcoded for now
#include "metrics.inc"

Size Canvas::GetTextSize(const char* text) const
{
	Size ret = {};

	for (const char* c = text; *c != '\0'; c += 1)
	{
		const Metrics metrics = METRICS[(*c) & 127];
		ret.w += metrics.advance + SPACING;
	}

	ret.w -= SPACING;
	// ret.h = HEIGHT; // TODO, looks ugly, also margins aren't helping
	ret.h = 1.0f;
	return ret;
}

static void sDrawText(float em_scale, int buffer_w, int buffer_h, const char* text, Position pos, Colour16 colour,
                      Colour16* buffer)
{
	for (const char* c = text; *c != '\0'; c += 1)
	{
		const Metrics metrics = METRICS[(*c) & 127];

		if ((static_cast<char>(*c) > '!' && static_cast<char>(*c) < '~') || static_cast<char>(*c) == ' ')
		{
			// clang-format off
			const int x1 = Clamp(static_cast<int>(sScale(pos.x + metrics.x_offset, em_scale)), 0, buffer_w);
			const int y1 = Clamp(static_cast<int>(sScale(pos.y + metrics.y_offset, em_scale)), 0, buffer_h);
			const int x2 = Clamp(static_cast<int>(sScale(pos.x + metrics.x_offset + metrics.w, em_scale)), 0, buffer_w);
			const int y2 = Clamp(static_cast<int>(sScale(pos.y + metrics.y_offset + metrics.h, em_scale)), 0, buffer_h);
			// clang-format on

			int row_no = 0;
			for (Colour16* row = (buffer + buffer_w * y1); row < (buffer + buffer_w * y2); row += buffer_w, row_no += 1)
			{
				for (int col = x1; col < x2; col += 1)
				{
					row[col] = (((col ^ row_no) & 1) != 0) ? colour : row[col];
				}
			}
		}

		pos.x += metrics.advance + SPACING;
	}
}


static void sDraw3dOutsideBevel(float em_scale, int buffer_w, int buffer_h, Rect rect, Colour16* buffer)
{
	// TODO, Code of this one is ugly, also pixel-perfect lines is going to
	// be painful on wacky DPI and zoom levels

	// clang-format off
	const int x1 = Clamp(static_cast<int>(sScale(rect.pos.x, em_scale)), 0, buffer_w);
	const int y1 = Clamp(static_cast<int>(sScale(rect.pos.y, em_scale)), 0, buffer_h);
	const int x2 = Clamp(static_cast<int>(sScale(rect.pos.x + rect.size.w, em_scale)), 0, buffer_w);
	const int y2 = Clamp(static_cast<int>(sScale(rect.pos.y + rect.size.h, em_scale)), 0, buffer_h);
	// clang-format on

	// Top
	if (static_cast<int>(rect.pos.y) >= 0)
	{
		for (Colour16* p = (buffer + buffer_w * y1 + x1); p < (buffer + buffer_w * y1 + x2); p += 1)
			*p = COLOUR16_WHITE;
	}

	// Left
	if (static_cast<int>(rect.pos.x) >= 0)
	{
		for (Colour16* p = (buffer + buffer_w * y1 + x1); p < (buffer + buffer_w * y2 + x1); p += buffer_w)
			*p = COLOUR16_WHITE;
	}

	// Bottom
	if (static_cast<int>(rect.pos.y + rect.size.h) <= buffer_h)
	{
		for (Colour16* p = (buffer + buffer_w * (y2 - 1) + x1); p < (buffer + buffer_w * (y2 - 1) + x2); p += 1)
			*p = COLOUR16_BLACK;
	}

	// Right
	if (static_cast<int>(rect.pos.x + rect.size.w) <= buffer_w)
	{
		for (Colour16* p = (buffer + buffer_w * y1 + (x2 - 1)); p < (buffer + buffer_w * y2 + (x2 - 1)); p += buffer_w)
			*p = COLOUR16_BLACK;
	}
}


static void sDraw3dInnerBevel(float em_scale, int buffer_w, int buffer_h, Rect rect, Colour16* buffer)
{
	// This inner bevel always bite one column and row, making its content look misaligned.
	// But it's the correct way, we don't want to have all other metrics misaligned.

	if (1)
	{
		rect.pos.x += 1.0f / em_scale;
		rect.pos.y += 1.0f / em_scale;
		rect.size.w -= 2.0f / em_scale;
		rect.size.h -= 2.0f / em_scale;

		// clang-format off
		const int x1 = Clamp(static_cast<int>(sScale(rect.pos.x, em_scale)), 0, buffer_w);
		const int y1 = Clamp(static_cast<int>(sScale(rect.pos.y, em_scale)), 0, buffer_h);
		const int x2 = Clamp(static_cast<int>(sScale(rect.pos.x + rect.size.w, em_scale)), 0, buffer_w);
		const int y2 = Clamp(static_cast<int>(sScale(rect.pos.y + rect.size.h, em_scale)), 0, buffer_h);
		// clang-format on

		// Bottom
		if (static_cast<int>(rect.pos.y + rect.size.h) <= buffer_h)
		{
			for (Colour16* p = (buffer + buffer_w * (y2 - 1) + x1); p < (buffer + buffer_w * (y2 - 1) + x2); p += 1)
				*p = COLOUR16_DARK_GREY;
		}

		// Right
		if (static_cast<int>(rect.pos.x + rect.size.w) <= buffer_w)
		{
			for (Colour16* p = (buffer + buffer_w * y1 + (x2 - 1)); p < (buffer + buffer_w * y2 + (x2 - 1));
			     p += buffer_w)
				*p = COLOUR16_DARK_GREY;
		}
	}
}


void Canvas::DrawRectangle(Rect rect, Colour16 colour)
{
	sDrawRectangle(m_em_scale, m_width, m_height, rect, colour, m_buffer);
}

void Canvas::DrawText(Position pos, const char* text, Colour16 colour)
{
	sDrawText(m_em_scale, m_width, m_height, text, pos, colour, m_buffer);
}

void Canvas::Draw3dBevel(Rect rect)
{
	sDraw3dOutsideBevel(m_em_scale, m_width, m_height, rect, m_buffer);
	sDraw3dInnerBevel(m_em_scale, m_width, m_height, rect, m_buffer);
}
