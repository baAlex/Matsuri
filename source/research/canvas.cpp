/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <assert.h>
#include <stdlib.h>

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


static void sDrawRectangle(int buffer_w, int buffer_h, Rect rect, Colour16 colour, Colour16* buffer)
{
	const int x1 = Clamp(static_cast<int>(rect.pos.x), 0, buffer_w);
	const int y1 = Clamp(static_cast<int>(rect.pos.y), 0, buffer_h);
	const int x2 = Clamp(static_cast<int>(rect.pos.x + rect.size.w), 0, buffer_w);
	const int y2 = Clamp(static_cast<int>(rect.pos.y + rect.size.h), 0, buffer_h);

	for (Colour16* row = (buffer + buffer_w * y1); row < (buffer + buffer_w * y2); row += buffer_w)
	{
		for (int col = x1; col < x2; col += 1)
			row[col] = colour;
	}
}

static void sDraw3dOutsideBevel(int buffer_w, int buffer_h, Rect rect, Colour16* buffer)
{
	// TODO, Code of this one is ugly, also pixel-perfect lines is going to
	// be painful on wacky DPI and zoom levels

	const int x1 = Clamp(static_cast<int>(rect.pos.x), 0, buffer_w);
	const int y1 = Clamp(static_cast<int>(rect.pos.y), 0, buffer_h);
	const int x2 = Clamp(static_cast<int>(rect.pos.x + rect.size.w), 0, buffer_w);
	const int y2 = Clamp(static_cast<int>(rect.pos.y + rect.size.h), 0, buffer_h);

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

static void sDraw3dInsideBevel(int buffer_w, int buffer_h, Rect rect, Colour16* buffer)
{
	rect.pos.x += 1.0f;
	rect.pos.y += 1.0f;
	rect.size.w -= 2.0f;
	rect.size.h -= 2.0f;

	const int x1 = Clamp(static_cast<int>(rect.pos.x), 0, buffer_w);
	const int y1 = Clamp(static_cast<int>(rect.pos.y), 0, buffer_h);
	const int x2 = Clamp(static_cast<int>(rect.pos.x + rect.size.w), 0, buffer_w);
	const int y2 = Clamp(static_cast<int>(rect.pos.y + rect.size.h), 0, buffer_h);

	// Bottom
	if (static_cast<int>(rect.pos.y + rect.size.h) <= buffer_h)
	{
		for (Colour16* p = (buffer + buffer_w * (y2 - 1) + x1); p < (buffer + buffer_w * (y2 - 1) + x2); p += 1)
			*p = COLOUR16_DARK_GREY;
	}

	// Right
	if (static_cast<int>(rect.pos.x + rect.size.w) <= buffer_w)
	{
		for (Colour16* p = (buffer + buffer_w * y1 + (x2 - 1)); p < (buffer + buffer_w * y2 + (x2 - 1)); p += buffer_w)
			*p = COLOUR16_DARK_GREY;
	}
}

void Canvas::DrawRectangle(Rect rect, Colour16 colour)
{
	rect.pos.x *= m_em_scale;
	rect.pos.y *= m_em_scale;
	rect.size.w *= m_em_scale;
	rect.size.h *= m_em_scale;
	sDrawRectangle(m_width, m_height, rect, colour, m_buffer);
}

void Canvas::Draw3dBevel(Rect rect)
{
	rect.pos.x *= m_em_scale;
	rect.pos.y *= m_em_scale;
	rect.size.w *= m_em_scale;
	rect.size.h *= m_em_scale;
	sDraw3dOutsideBevel(m_width, m_height, rect, m_buffer);
	sDraw3dInsideBevel(m_width, m_height, rect, m_buffer);
}
