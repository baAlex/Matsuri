
#include <ft2build.h>
#include <stdio.h>
#include FT_FREETYPE_H


int main(int argc, const char* argv[])
{
	const auto fixed_to_float = [](int x) -> float { return (float)(x) / 64.0f; };

	if (argc == 1)
		return 0;

	FT_Library library;
	FT_Face face;

	FT_Init_FreeType(&library);
	FT_New_Face(library, argv[1], 0, &face);

	FT_Set_Pixel_Sizes(face, 0, 16);

	printf("\n// %s %s\n", face->family_name, face->style_name);

	printf("\n");
	printf("struct Metrics\n{\n\tfloat w;\n\tfloat h;\n\tfloat x_offset;\n\tfloat y_offset;\n\tfloat advance;\n};\n");
	printf("\n");

	const float font_height =
	    fixed_to_float(face->size->metrics.height) / 16.0f; // 'face->size' only available after Set_Pixel_Sizes()
	printf("static constexpr float HEIGHT = %ff;\n", font_height);

	printf("\n");

	printf("static constexpr Metrics METRICS[128] = {\n");
	for (int c = 0; c < 128; c += 1)
	{
		if (FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_DEFAULT) != 0)
			continue;

		const FT_GlyphSlot g = face->glyph;

		/*
		...to render text, a virtual point, located on the baseline, called the pen position or origin, is used to
		locate glyphs.

		(...)
		Left side bearing
		The horizontal distance from the current pen position to the glyph's left bbox edge. It is positive for
		horizontal layouts, and in most cases negative for vertical ones. In the FreeType API, this is also called
		bearingX. Another shorthand is ‘lsb’.

		(...)
		Top side bearing
		The vertical distance from the baseline to the top of the glyph's bbox. It is usually positive for horizontal
		layouts, and negative for vertical ones. In the FreeType API, this is also called bearingY.

		(...)
		Advance width
		The horizontal distance to increment (for left-to-right writing) or decrement (for right-to-left writing) the
		pen position after a glyph has been rendered when processing text. It is always positive for horizontal layouts,
		and zero for vertical ones. In the FreeType API, this is also called advanceX.

		(...)
		Because hinting aligns the glyph's control points to the pixel grid, this process slightly modifies the
		dimensions of character images in ways that differ from simple scaling.

		(...)
		As seen before, the ‘origin’ of a given glyph corresponds to the position of the pen on the baseline. It is not
		necessarily located on one of the glyph's bounding box corners.

		(https://freetype.org/freetype2/docs/glyphs/glyphs-3.html)
		*/

		// Funny 'font_height - height' is because our render ('canvas.cpp') renders upside-down

		if ((static_cast<char>(c) > '!' && static_cast<char>(c) < '~') || static_cast<char>(c) == ' ')
			printf("\t{%.2ff, %.2ff, %.2ff, %.2ff, %.2ff}, // '%c'\n",            //
			       fixed_to_float(g->metrics.width) / 16.0f,                      //
			       fixed_to_float(g->metrics.height) / 16.0f,                     //
			       fixed_to_float(g->metrics.horiBearingX) / 16.0f,               //
			       font_height - fixed_to_float(g->metrics.horiBearingY) / 16.0f, //
			       fixed_to_float(g->metrics.horiAdvance) / 16.0f,                //
			       static_cast<char>(c));
		else
			printf("\t{%.2ff, %.2ff, %.2ff, %.2ff, %.2ff}, //\n",                 //
			       fixed_to_float(g->metrics.width) / 16.0f,                      //
			       fixed_to_float(g->metrics.height) / 16.0f,                     //
			       fixed_to_float(g->metrics.horiBearingX) / 16.0f,               //
			       font_height - fixed_to_float(g->metrics.horiBearingY) / 16.0f, //
			       fixed_to_float(g->metrics.horiAdvance) / 16.0f);
	}

	printf("};\n");

	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return 0;
}
