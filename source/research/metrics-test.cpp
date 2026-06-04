
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H


// Use "ldd ./metrics-test", "ldd ./metrics-system-test" after compiling,
// errors of 1 or 2 pixels are expected


static int sLoadFont(const char* filename)
{
	const auto fixed_to_float = [](FT_Pos x) -> float { return (float)(x) / 64.0f; };

	printf("sizeof(FT_Pos) = %zu\n", sizeof(FT_Pos));
	printf("sizeof(long)   = %zu\n", sizeof(long));

	FT_Error err;
	FT_Library library;
	FT_Face face;

	if ((err = FT_Init_FreeType(&library)) != 0)
	{
		printf("Error: '%s'\n", FT_Error_String(err)); // Use exceptions on the real thing
		return 1;
	}

	if ((err = FT_New_Face(library, filename, 0, &face)) != 0)
	{
		printf("Error: '%s'\n", FT_Error_String(err));
		return 1;
	}

	if ((err = FT_Set_Pixel_Sizes(face, 0, 32)) != 0)
	{
		printf("Error: '%s'\n", FT_Error_String(err));
		return 1;
	}

	printf("units_per_EM = %u\n", face->units_per_EM);

	printf("x_ppem = %u\n", face->size->metrics.x_ppem);
	printf("y_ppem = %u\n", face->size->metrics.y_ppem);

	printf("x_scale = %ld\n", face->size->metrics.x_scale);
	printf("y_scale = %ld\n", face->size->metrics.y_scale);
	printf("height raw = %ld\n", face->size->metrics.height);

	printf("height = %.2f\n", fixed_to_float(face->size->metrics.height));

	const float font_height =
	    fixed_to_float(face->size->metrics.height); // 'face->size' only available after Set_Pixel_Sizes()

	for (int c = 0; c < 128; c += 1)
	{
		if ((err = FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_DEFAULT)) != 0)
		{
			printf("Error: '%s'\n", FT_Error_String(err));
			return 1;
		}

		const FT_GlyphSlot g = face->glyph;

		if ((static_cast<char>(c) > '!' && static_cast<char>(c) < '~') || static_cast<char>(c) == ' ')
			printf("'%c', Width: %.2f, Height: %.2f, X: %.2f, Y: %.2f, Advance: %.2f\n", //
			       static_cast<char>(c),
			       fixed_to_float(g->metrics.width),        //
			       fixed_to_float(g->metrics.height),       //
			       fixed_to_float(g->metrics.horiBearingX), //
			       fixed_to_float(g->metrics.horiBearingY), //
			       fixed_to_float(g->metrics.horiAdvance));
	}

	printf("Font Height = %.4f\n", font_height);

	int major;
	int minor;
	int path;
	FT_Library_Version(library, &major, &minor, &path);

	printf("Freetype v%i.%i.%i\n", major, minor, path);

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	return 0;
}


int main(int argc, const char* argv[])
{
	if (argc == 1)
		return 0;

	sLoadFont(argv[1]);

	return 0;
}
