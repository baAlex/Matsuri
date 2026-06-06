/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <math.h>
#include <stdlib.h>

#include "canvas.hpp"
#include "error.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H


static const float EM_BASE = 16.0f;


#include "texgyreheros-regular.inc"


void Canvas::LoadAndRenderFont(int px_size, const uint8_t* data, size_t data_size, FontItem* out)
{
	// Fixed format used by Freetype
	const auto fixed_to_float = [](FT_Pos x) -> float { return (float)(x) / 64.0f; };

	FT_Error err;
	FT_Library library = nullptr;
	FT_Face face = nullptr;

	const auto clean_stuff = [&]()
	{
		if (face != nullptr)
			FT_Done_Face(face);
		if (library != nullptr)
			FT_Done_FreeType(library);
	};

	// Initialise library and face
	if ((err = FT_Init_FreeType(&library)) != 0)
		throw Error("Cannot initialise Freetype library");

	if ((err = FT_New_Memory_Face(library, data, static_cast<FT_Long>(data_size), 0, &face)) != 0)
	{
		clean_stuff();
		throw Error("Cannot initialise font");
	}

	if ((err = FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(px_size))) != 0)
	{
		clean_stuff();
		throw Error("Cannot set font size");
	}

	// Get font height, which isn't that trivial
	if (1)
	{
		// https://en.wikipedia.org/wiki/X-height

		if (FT_Load_Char(face, 'X', FT_LOAD_DEFAULT) != 0) // Using uppercase one
		{
			clean_stuff();
			throw Error("Cannot load font character");
		}

		out->font_height = fixed_to_float(face->glyph->metrics.height);
	}
	else
	{
		out->font_height = fixed_to_float(face->size->metrics.height);
	}

	// Take ASCII characters metrics and render them
	for (int c = 0; c < 128; c += 1)
	{
		// Notice the lighter hint flag and auto-hint (it's good),
		// this Helvetica clone (Helvetica itself actually) is bad at numbers
		if ((err = FT_Load_Char(face, static_cast<FT_ULong>(c),
		                        FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT)) != 0)
		{
			clean_stuff();
			throw Error("Cannot render font character");
		}

		// Save metrics
		const FT_GlyphSlot g = face->glyph;

		if (0)
		{
			if ((static_cast<char>(c) >= '!' && static_cast<char>(c) <= '~') || static_cast<char>(c) == ' ')
				printf("'%c', Width: %i, Height: %i, X: %.2f, Y: %.2f, Advance: %.2f\n", //
				       static_cast<char>(c),
				       g->bitmap.width,                         //
				       g->bitmap.rows,                          //
				       fixed_to_float(g->metrics.horiBearingX), //
				       fixed_to_float(g->metrics.horiBearingY), //
				       fixed_to_float(g->metrics.horiAdvance));
		}

		Glyph* item = out->glyph + c;
		item->w = static_cast<int>(g->bitmap.width);
		item->h = static_cast<int>(g->bitmap.rows);
		item->x_offset = fixed_to_float(g->metrics.horiBearingX);
		item->y_offset = out->font_height - fixed_to_float(g->metrics.horiBearingY);
		item->advance = fixed_to_float(g->metrics.horiAdvance) / EM_BASE;

		// Nothing else to do with non-printable characters
		if (static_cast<char>(c) < '!' || static_cast<char>(c) > '~')
			continue;

		// Save rendered glyph
		if (g->bitmap.buffer == nullptr) // This is Freetype behaviour
		{
			// However is not like we are rendering empty glyphs
			clean_stuff();
			throw Error("No glyph rendered");
		}

		if (g->bitmap.pitch <= 0) // Same
		{
			// Weird Freetype quirk, pitch can be negative
			// (an old BMP design from the 90s???)

			// This time we known our hardcoded font doesn't
			// behave like this, it may trigger if I change it
			clean_stuff();
			throw Error("Cannot load font character");
		}

		item->data_offset = m_data_soup_size;
		m_data_soup_size += g->bitmap.width * g->bitmap.rows;

		{
			auto new_memory = reinterpret_cast<uint8_t*>(realloc(m_data_soup, m_data_soup_size));
			if (new_memory == nullptr)
			{
				free(m_data_soup);
				clean_stuff();
				throw Error("No enough memory");
			}

			m_data_soup = new_memory;
		}

		uint8_t* out = m_data_soup + item->data_offset;
		for (unsigned r = 0; r < g->bitmap.rows; r += 1)
		{
			const uint8_t* in = g->bitmap.buffer + static_cast<unsigned>(g->bitmap.pitch) * r;
			for (unsigned c = 0; c < g->bitmap.width; c += 1)
				*out++ = *in++;
		}
	}

	// One last thing, used for EM/PX conversion
	out->font_height /= EM_BASE;

	// Bye!
	clean_stuff();
	return;
}


Canvas::Canvas(int width, int height, float em_scale)
{
	m_em_scale = em_scale * EM_BASE;

	m_width = width;
	m_height = height;
	m_buffer = nullptr;

	for (int i = 0; i < FONTS_NO; i += 1)
		m_fonts[i] = {};

	m_data_soup = nullptr;
	m_data_soup_size = 0;

	// Now the things that can fail
	if ((m_buffer = reinterpret_cast<Colour*>(
	         malloc(sizeof(Colour) * static_cast<size_t>(width) * static_cast<size_t>(height)))) == nullptr)
	{
		throw Error("No enough memory");
	}

	try
	{
		// They follow GetFont()
		LoadAndRenderFont(static_cast<int>(m_em_scale * 0.85f), reinterpret_cast<const uint8_t*>(HEROS_REGULAR_DATA),
		                  HEROS_REGULAR_SIZE, m_fonts + 0);

		LoadAndRenderFont(static_cast<int>(m_em_scale * 0.85f * 0.75f),
		                  reinterpret_cast<const uint8_t*>(HEROS_REGULAR_DATA), HEROS_REGULAR_SIZE, m_fonts + 1);
	}
	catch (const Error& e)
	{
		throw e;
	}
}

Canvas::~Canvas() noexcept
{
	if (m_buffer != nullptr)
		free(m_buffer);

	if (m_data_soup != nullptr)
		free(m_data_soup);
}


const Canvas::FontItem* Canvas::GetFont(Font font_style) const noexcept
{
	switch (font_style)
	{
	case Font::Normal: return m_fonts + 0;
	case Font::Small: return m_fonts + 1;
	}
}


const Colour* Canvas::GetBuffer() const noexcept
{
	return m_buffer;
}


static float sScale(float x, float em_scale)
{
	return floorf(x * em_scale + 0.5f); // round() is expensive
}

void Canvas::DrawRectangle(Rect rect, Colour colour) noexcept
{
	// PROTIP, is better to do the scaling at the last instance possible, near int conversion.
	// Otherwise round errors accumulate, very quickly everywhere

	const int x1 = Clamp(static_cast<int>(sScale(rect.pos.x, m_em_scale)), 0, m_width);
	const int y1 = Clamp(static_cast<int>(sScale(rect.pos.y, m_em_scale)), 0, m_height);
	const int x2 = Clamp(static_cast<int>(sScale(rect.pos.x + rect.size.w, m_em_scale)), 0, m_width);
	const int y2 = Clamp(static_cast<int>(sScale(rect.pos.y + rect.size.h, m_em_scale)), 0, m_height);

	for (Colour* row = (m_buffer + m_width * y1); row < (m_buffer + m_width * y2); row += m_width)
	{
		for (int col = x1; col < x2; col += 1)
			row[col] = colour;
	}
}


Size Canvas::GetTextSize(Font font_style, const char* text) const noexcept
{
	const float SPACING = 0.0f; // Just in case for the future
	const FontItem* font = GetFont(font_style);

	Size ret = {};

	for (const char* c = text; *c != '\0'; c += 1)
	{
		const Glyph glyph = font->glyph[(*c) & 127];
		ret.w += glyph.advance + SPACING;
	}

	ret.w -= SPACING;
	ret.h = font->font_height; // TODO, looks ugly, also margins aren't helping
	                           // EDIT, nah, text work like this
	// ret.h = 1.0f;
	return ret;
}

void Canvas::DrawText(Font font_style, Position pos, const char* text, Colour colour) noexcept
{
	const float SPACING = 0.0f; // Just in case for the future
	const FontItem* font = GetFont(font_style);

	for (const char* c = text; *c != '\0'; c += 1)
	{
		const Glyph glyph = font->glyph[(*c) & 127];

		if ((static_cast<char>(*c) >= '!' && static_cast<char>(*c) <= '~'))
		{
			// clang-format off
			const int x1 = Clamp(static_cast<int>(sScale(pos.x, m_em_scale) + glyph.x_offset), 0, m_width);
			const int y1 = Clamp(static_cast<int>(sScale(pos.y, m_em_scale) + glyph.y_offset), 0, m_height);
			const int x2 = Clamp(static_cast<int>(sScale(pos.x, m_em_scale) + glyph.x_offset) + glyph.w, 0, m_width);
			const int y2 = Clamp(static_cast<int>(sScale(pos.y, m_em_scale) + glyph.y_offset) + glyph.h, 0, m_height);
			// clang-format on

			if (0)
			{
				// Developers, developers, developers
				int row_no = 0;

				for (Colour* row = (m_buffer + m_width * y1); row < (m_buffer + m_width * y2);
				     row += m_width, row_no += 1)
				{
					for (int col = x1; col < x2; col += 1)
					{
						row[col] = (((col ^ row_no) & 1) != 0) ? COLOUR_RED : row[col];
					}
				}
			}

			if (1)
			{
				const uint8_t* in = m_data_soup + glyph.data_offset;

				for (Colour* row = (m_buffer + m_width * y1); row < (m_buffer + m_width * y2); row += m_width)
				{
					for (int col = x1; col < x2; col += 1)
					{
						row[col] = Colour::Mix(row[col], colour, *in++);
					}
				}
			}
		}

		pos.x += glyph.advance + SPACING;
	}
}


static void sDraw3dOutsideBevel(float em_scale, int buffer_w, int buffer_h, Rect rect, Colour* buffer)
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
		for (Colour* p = (buffer + buffer_w * y1 + x1); p < (buffer + buffer_w * y1 + x2); p += 1)
			*p = COLOUR_WHITE;
	}

	// Left
	if (static_cast<int>(rect.pos.x) >= 0)
	{
		for (Colour* p = (buffer + buffer_w * y1 + x1); p < (buffer + buffer_w * y2 + x1); p += buffer_w)
			*p = COLOUR_WHITE;
	}

	// Bottom
	if (static_cast<int>(rect.pos.y + rect.size.h) <= buffer_h)
	{
		for (Colour* p = (buffer + buffer_w * (y2 - 1) + x1); p < (buffer + buffer_w * (y2 - 1) + x2); p += 1)
			*p = COLOUR_BLACK;
	}

	// Right
	if (static_cast<int>(rect.pos.x + rect.size.w) <= buffer_w)
	{
		for (Colour* p = (buffer + buffer_w * y1 + (x2 - 1)); p < (buffer + buffer_w * y2 + (x2 - 1)); p += buffer_w)
			*p = COLOUR_BLACK;
	}
}

static void sDraw3dInnerBevel(float em_scale, int buffer_w, int buffer_h, Rect rect, Colour* buffer)
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
			for (Colour* p = (buffer + buffer_w * (y2 - 1) + x1); p < (buffer + buffer_w * (y2 - 1) + x2); p += 1)
				*p = COLOUR_DARK_GREY;
		}

		// Right
		if (static_cast<int>(rect.pos.x + rect.size.w) <= buffer_w)
		{
			for (Colour* p = (buffer + buffer_w * y1 + (x2 - 1)); p < (buffer + buffer_w * y2 + (x2 - 1));
			     p += buffer_w)
				*p = COLOUR_DARK_GREY;
		}
	}
}

void Canvas::Draw3dBevel(Rect rect) noexcept
{
	sDraw3dOutsideBevel(m_em_scale, m_width, m_height, rect, m_buffer);
	sDraw3dInnerBevel(m_em_scale, m_width, m_height, rect, m_buffer);
}
