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

#include "shared.hpp"

class Canvas
{
  public:
	Canvas(int width, int height, float em_scale);
	~Canvas() noexcept;

	const Colour* GetBuffer() const noexcept;

	void DrawRectangle(Rect rect, Colour colour) noexcept;
	void Draw3dBevel(Rect rect) noexcept;

	void DrawText(Font font, Position pos, const char* text, Colour colour) noexcept;
	Size GetTextSize(Font font, const char* text) const noexcept;

  private:
	float m_em_scale;

	int m_width;
	int m_height;
	Colour* m_buffer;

	struct Glyph
	{
		int w;          // In PX
		int h;          // In PX
		float x_offset; // In PX
		float y_offset; // In PX

		float advance; // In EM

		size_t data_offset; // Offset/index to data soup, in bytes
	};

	struct FontItem
	{
		float font_height; // In EM
		Glyph glyph[128];
	};

	FontItem m_fonts[FONTS_NO];

	uint8_t* m_data_soup;
	size_t m_data_soup_size; // In bytes

	const FontItem* GetFont(Font font) const noexcept;
	void LoadAndRenderFont(int px_size, const uint8_t* data, size_t data_size, FontItem* out);
};

#endif
