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

#include <ft2build.h>
#include FT_FREETYPE_H


static constexpr float EM_BASE = 16.0f;


struct Glyph
{
	int w;          // In PX
	int h;          // In PX
	float x_offset; // In PX
	float y_offset; // In PX

	float advance; // In EM

	size_t data_offset; // Offset/index to data soup, in bytes
};

// TODO, all this should be part of the Canvas
static uint8_t* s_data_soup;
static size_t s_data_soup_size; // In bytes

static float s_font_height; // In EM
static Glyph s_glyph[128];


static int sLoadAndRenderFont(int px_size, const char* filename)
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
	{
		printf("Error: '%s'\n", FT_Error_String(err)); // TODO, use error codes!
		goto return_failure;
	}

	if ((err = FT_New_Face(library, filename, 0, &face)) != 0)
	{
		printf("Error: '%s'\n", FT_Error_String(err));
		goto return_failure;
	}

	if ((err = FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(px_size))) != 0)
	{
		printf("Error: '%s'\n", FT_Error_String(err));
		goto return_failure;
	}

	s_font_height = fixed_to_float(face->size->metrics.height); // 'face->size' only available after Set_Pixel_Sizes()

	// Take ASCII characters metrics and render them
	for (int c = 0; c < 128; c += 1)
	{
		if ((err = FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)) != 0)
		{
			printf("Error: '%s'\n", FT_Error_String(err));
			goto return_failure;
		}

		// Save metrics
		const FT_GlyphSlot g = face->glyph;

		if ((static_cast<char>(c) >= '!' && static_cast<char>(c) <= '~') || static_cast<char>(c) == ' ')
			printf("'%c', Width: %i, Height: %i, X: %.2f, Y: %.2f, Advance: %.2f\n", //
			       static_cast<char>(c),
			       g->bitmap.width,                         //
			       g->bitmap.rows,                          //
			       fixed_to_float(g->metrics.horiBearingX), //
			       fixed_to_float(g->metrics.horiBearingY), //
			       fixed_to_float(g->metrics.horiAdvance));

		Glyph* item = s_glyph + c;
		item->w = static_cast<int>(g->bitmap.width);
		item->h = static_cast<int>(g->bitmap.rows);
		item->x_offset = fixed_to_float(g->metrics.horiBearingX);
		item->y_offset = s_font_height - fixed_to_float(g->metrics.horiBearingY);
		item->advance = fixed_to_float(g->metrics.horiAdvance) / EM_BASE;

		// Nothing else to do with non-printable characters
		if (static_cast<char>(c) < '!' || static_cast<char>(c) > '~')
			continue;

		// Save rendered glyph
		if (g->bitmap.buffer == nullptr)
		{
			printf("Error: not glyph rendered\n");
			goto return_failure;
		}

		if (g->bitmap.pitch <= 0)
		{
			// Weird design, pitch can be negative
			// (an old BMP design from the 90s???)
			printf("Error: funny value\n");
			goto return_failure;
		}

		item->data_offset = s_data_soup_size;
		s_data_soup_size += g->bitmap.width * g->bitmap.rows;

		s_data_soup = reinterpret_cast<uint8_t*>(
		    realloc(s_data_soup, s_data_soup_size)); // TODO, check error, and realloc() quirks

		uint8_t* out = s_data_soup + item->data_offset;
		for (unsigned r = 0; r < g->bitmap.rows; r += 1)
		{
			const uint8_t* in = g->bitmap.buffer + static_cast<unsigned>(g->bitmap.pitch) * r;
			for (unsigned c = 0; c < g->bitmap.width; c += 1)
				*out++ = 255 - (*in++);
		}
	}

	// One last thing, used for EM/PX conversion
	s_font_height /= EM_BASE;

	// Bye!
	clean_stuff();
	return 0;

return_failure:
	clean_stuff();
	return 1;
}


Canvas* Canvas::Create(int width, int height, float em_scale)
{
	// TODO, obvious hardcoded size and font
	sLoadAndRenderFont(static_cast<int>(em_scale * 14.0f), "texgyreheros-regular.otf");
	return new Canvas(width, height, em_scale * EM_BASE);
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


Size Canvas::GetTextSize(const char* text) const
{
	const int SPACING = 0; // Just in case for the future

	Size ret = {};

	for (const char* c = text; *c != '\0'; c += 1)
	{
		const Glyph glyph = s_glyph[(*c) & 127];
		ret.w += glyph.advance + SPACING;
	}

	ret.w -= SPACING;
	ret.h = s_font_height; // TODO, looks ugly, also margins aren't helping
	                       // EDIT, nah, text work like this
	// ret.h = 1.0f;
	return ret;
}

static void sDrawText(float em_scale, int buffer_w, int buffer_h, const char* text, Position pos, Colour16 colour,
                      Colour16* buffer)
{
	const int SPACING = 0; // Just in case for the future

	for (const char* c = text; *c != '\0'; c += 1)
	{
		const Glyph glyph = s_glyph[(*c) & 127];

		if ((static_cast<char>(*c) >= '!' && static_cast<char>(*c) <= '~') || static_cast<char>(*c) == ' ')
		{
			// clang-format off
			const int x1 = Clamp(static_cast<int>(sScale(pos.x, em_scale) + glyph.x_offset), 0, buffer_w);
			const int y1 = Clamp(static_cast<int>(sScale(pos.y, em_scale) + glyph.y_offset), 0, buffer_h);
			const int x2 = Clamp(static_cast<int>(sScale(pos.x, em_scale) + glyph.x_offset) + glyph.w, 0, buffer_w);
			const int y2 = Clamp(static_cast<int>(sScale(pos.y, em_scale) + glyph.y_offset) + glyph.h, 0, buffer_h);
			// clang-format on

			if (0)
			{
				// Developers, developers, developers
				int row_no = 0;

				for (Colour16* row = (buffer + buffer_w * y1); row < (buffer + buffer_w * y2);
				     row += buffer_w, row_no += 1)
				{
					for (int col = x1; col < x2; col += 1)
					{
						row[col] = (((col ^ row_no) & 1) != 0) ? colour : row[col];
					}
				}
			}

			if (1)
			{
				const uint8_t* in = s_data_soup + glyph.data_offset;

				for (Colour16* row = (buffer + buffer_w * y1); row < (buffer + buffer_w * y2); row += buffer_w)
				{
					for (int col = x1; col < x2; col += 1)
					{
						row[col] = Colour16::MixToBlack(row[col], *in++);
					}
				}
			}
		}

		pos.x += glyph.advance + SPACING;
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
