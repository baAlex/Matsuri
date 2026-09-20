/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <algorithm>
#include <assert.h>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H


// ############################


template <typename T> class [[nodiscard]] Defer
{
	// Andrei Alexandrescu & Petru Marginean (2000).
	// "Generic: Change the Way You Write Exception-Safe Code - Forever"
	// https://web.archive.org/web/20190114213811/http://www.drdobbs.com/cpp/generic-change-the-way-you-write-excepti/184403758?pgno=2

  public:
	T callback;
	Defer(T c) noexcept : callback(c) {}
	~Defer() noexcept
	{
		callback();
	}
};

#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)
#define DEFER(code) const Defer CONCAT(_defer_l, __LINE__)([&]() -> void { code });

// ----

template <typename T, typename Y> T UnionCast(Y v)
{
	static_assert(sizeof(T) == sizeof(Y), "OI M8!");
	T ret;
	memcpy(&ret, &v, sizeof(Y));
	return ret;
};

uint16_t EndiannessReverse(uint16_t v) noexcept
{
	return static_cast<uint16_t>((v << 8) | (v >> 8));
}

uint32_t EndiannessReverse(uint32_t v) noexcept
{
	return static_cast<uint32_t>(((v & 0x000000FF) << 24) | //
	                             ((v & 0x0000FF00) << 8) |  //
	                             ((v & 0x00FF0000) >> 8) |  //
	                             ((v & 0xFF000000) >> 24));
}

int16_t EndiannessReverse(int16_t v) noexcept
{
	return UnionCast<int16_t>(EndiannessReverse(UnionCast<uint16_t>(v)));
}

int32_t EndiannessReverse(int32_t v) noexcept
{
	return UnionCast<int32_t>(EndiannessReverse(UnionCast<uint32_t>(v)));
}

float EndiannessReverse(float v) noexcept
{
	return UnionCast<float>(EndiannessReverse(UnionCast<uint32_t>(v)));
}

// ----

template <typename T> static void sWrite(T value, FILE* fp)
{
	if (fwrite(&value, sizeof(T), 1, fp) != 1)
		throw std::runtime_error("Cannot write to file");
}

template <typename T> static void sWriteReverse(T value, FILE* fp)
{
	value = EndiannessReverse(value);
	if (fwrite(&value, sizeof(T), 1, fp) != 1)
		throw std::runtime_error("Cannot write to file");
}

class FileWriter
{
	FILE* m_fp;

  public:
	FileWriter(const char* filename)
	{
		if ((m_fp = fopen(filename, "wb")) == nullptr)
			throw std::runtime_error(std::string("Cannot open '") + filename + "'");
	}

	~FileWriter() noexcept
	{
		fclose(m_fp);
	}

	void WriteString(const char* string, bool nul_terminate = false)
	{
		const size_t len = strlen(string);
		if (fwrite(string, sizeof(char), len + static_cast<size_t>(nul_terminate), m_fp) != len)
			throw std::runtime_error("Cannot write to file");
	}

	size_t Fwrite(const void* ptr, size_t size, size_t no) // Old reliable
	{
		const size_t ret = fwrite(ptr, size, no, m_fp);
		if (ret != no)
			throw std::runtime_error("Cannot write to file");
		return ret;
	}

	// clang-format off
	void WriteU8(uint8_t value)   { sWrite<uint8_t>(value, m_fp); }
	void WriteS8(int8_t value)    { sWrite<int8_t>(value, m_fp); }
	void WriteU16(uint16_t value) { sWrite<uint16_t>(value, m_fp); }
	void WriteS16(int16_t value)  { sWrite<int16_t>(value, m_fp); }
	void WriteU32(uint32_t value) { sWrite<uint32_t>(value, m_fp); }
	void WriteS32(int32_t value)  { sWrite<int32_t>(value, m_fp); }
	void WriteF32(float value)    { sWrite<float>(value, m_fp); }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	void WriteU16Le(uint16_t value) { WriteU16(value); }
	void WriteS16Le(int16_t value)  { WriteS16(value); }
	void WriteU32Le(uint32_t value) { WriteU32(value); }
	void WriteS32Le(int32_t value)  { WriteS32(value); }
	void WriteF32Le(float value)    { WriteF32(value); }

	void WriteU16Be(uint16_t value) { sWriteReverse<uint16_t>(value, m_fp); }
	void WriteS16Be(int16_t value)  { sWriteReverse<int16_t>(value, m_fp); }
	void WriteU32Be(uint32_t value) { sWriteReverse<uint32_t>(value, m_fp); }
	void WriteS32Be(int32_t value)  { sWriteReverse<int32_t>(value, m_fp); }
	void WriteF32Be(float value)    { sWriteReverse<float>(value, m_fp); }
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	void WriteU16Le(uint16_t value) { sWriteReverse<uint16_t>(value, m_fp); }
	void WriteS16Le(int16_t value)  { sWriteReverse<int16_t>(value, m_fp); }
	void WriteU32Le(uint32_t value) { sWriteReverse<uint32_t>(value, m_fp); }
	void WriteS32Le(int32_t value)  { sWriteReverse<int32_t>(value, m_fp); }
	void WriteF32Le(float value)    { sWriteReverse<float>(value, m_fp); }

	void WriteU16Be(uint16_t value) { WriteU16(value); }
	void WriteS16Be(int16_t value)  { WriteS16(value); }
	void WriteU32Be(uint32_t value) { WriteU32(value); }
	void WriteS32Be(int32_t value)  { WriteS32(value); }
	void WriteF32Be(float value)    { WriteF32(value); }
#endif
	// clang-format on
};


// ############################


constexpr uint32_t DDS_FLAG_CAPS = 0x1;            // Always required
constexpr uint32_t DDS_FLAG_HEIGHT = 0x2;          // Always required
constexpr uint32_t DDS_FLAG_WIDTH = 0x4;           // Always required
constexpr uint32_t DDS_FLAG_PITCH = 0x8;           // When pitch is provided on a uncompressed texture
constexpr uint32_t DDS_FLAG_PIXELFORMAT = 0x1000;  // Always required
constexpr uint32_t DDS_FLAG_MIPMAPCOUNT = 0x20000; // When texture is mipmapped
constexpr uint32_t DDS_FLAG_LINEARSIZE = 0x80000;  // When pitch is provided on a compressed texture
constexpr uint32_t DDS_FLAG_DEPTH = 0x800000;      // If is a 3d texture

constexpr uint32_t DDS_FORMAT_FLAG_ALPHAPIXELS = 0x1;
constexpr uint32_t DDS_FORMAT_FLAG_ALPHA = 0x2;
constexpr uint32_t DDS_FORMAT_FLAG_FOURCC = 0x4;
constexpr uint32_t DDS_FORMAT_FLAG_RGB = 0x40;
constexpr uint32_t DDS_FORMAT_FLAG_YUV = 0x200;
constexpr uint32_t DDS_FORMAT_FLAG_LUMINANCE = 0x20000;

constexpr uint32_t DDS_CAPS_COMPLEX = 0x8; // "Optional; must be used on any file that contains more than one surface (a
                                           // mipmap, a cubic environment map, or mipmapped volume texture)" [b]
constexpr uint32_t DDS_CAPS_MIPMAP = 0x400000; // "Optional; should be used for a mipmap" [b]
constexpr uint32_t DDS_CAPS_TEXTURE = 0x1000;  // Always required

constexpr uint32_t DDS_CAPS2_CUBEMAP = 0x200;             // Required for cube maps
constexpr uint32_t DDS_CAPS2_CUBEMAP_POSITIVE_X = 0x400;  // Required if side of cube map is present
constexpr uint32_t DDS_CAPS2_CUBEMAP_NEGATIVE_X = 0x800;  // Required if side of cube map is present
constexpr uint32_t DDS_CAPS2_CUBEMAP_POSITIVE_Y = 0x1000; // Required if side of cube map is present
constexpr uint32_t DDS_CAPS2_CUBEMAP_NEGATIVE_Y = 0x2000; // Required if side of cube map is present
constexpr uint32_t DDS_CAPS2_CUBEMAP_POSITIVE_Z = 0x4000; // Required if side of cube map is present
constexpr uint32_t DDS_CAPS2_CUBEMAP_NEGATIVE_Z = 0x8000; // Required if side of cube map is present
constexpr uint32_t DDS_CAPS2_VOLUME = 0x200000;           // For 3d textures

static void sWriteDdsAlpha8(const uint8_t* data, uint32_t width, uint32_t height, uint32_t input_pitch,
                            const char* filename)
{
	// [a] https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide#dds-file-layout
	// [b] https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header#syntax

	auto f = FileWriter(filename);

	const uint8_t bits_per_pixel = 8;
	const uint32_t output_pitch = (width * bits_per_pixel + 7) / 8; // Using provided formula [a]
	                                                                // "(width * bits-per-pixel + 7) / 8"

	f.WriteU32Le(0x20534444); // Magic

	f.WriteU32Le(124); // Size (of header)
	f.WriteU32Le(DDS_FLAG_CAPS | DDS_FLAG_HEIGHT | DDS_FLAG_WIDTH | DDS_FLAG_PITCH | DDS_FLAG_PIXELFORMAT);
	f.WriteU32Le(height);
	f.WriteU32Le(width);
	f.WriteU32Le(output_pitch); // "number of bytes per scan line in an uncompressed texture" [b]
	f.WriteU32Le(0);            // Depth for 3d textures
	f.WriteU32Le(0);            // Mipmap levels

	for (uint32_t i = 0; i < 11; i += 1)
		f.WriteU32(0);

	// PixelFormat struct
	f.WriteU32Le(32);                          // Size (of this struct)
	f.WriteU32Le(DDS_FORMAT_FLAG_ALPHAPIXELS); // Format flags
	f.WriteU32Le(0);                           // FourCC (for compressed formats)
	f.WriteU32Le(bits_per_pixel);              // RGB bit count
	f.WriteU32Le(0x00);                        // R bit mask (requires DDS_FORMAT_FLAG_RGB)
	f.WriteU32Le(0x00);                        // G bit mask (requires DDS_FORMAT_FLAG_RGB)
	f.WriteU32Le(0x00);                        // B bit mask (requires DDS_FORMAT_FLAG_RGB)
	f.WriteU32Le(0xFF);                        // A bit mask (requires DDS_FORMAT_FLAG_ALPHAPIXELS)
	// end of PixelFormat struct

	f.WriteU32Le(DDS_CAPS_TEXTURE);
	f.WriteU32Le(0); // Caps2 flags
	f.WriteU32Le(0); // Caps3 flags
	f.WriteU32Le(0); // Caps4 flags
	f.WriteU32Le(0); // Reserved

	const uint8_t* pixel = data;
	for (uint32_t row = 0; row < height; row += 1)
	{
		for (uint32_t col = 0; col < width; col += 1)
			f.WriteU8(pixel[col]);
		for (uint32_t p = width; p < output_pitch; p += 1)
			f.WriteU8(0);
		pixel += input_pitch;
	}
}


// ############################


struct Character
{
	uint32_t char_no;

	// Bitmap
	uint32_t width;
	uint32_t height;
	size_t data_at;

	uint32_t atlas_x;
	uint32_t atlas_y;

	// Metrics
	float x_offset;
	float y_offset;
	float advance;
};

static void sWriteHeader(const uint8_t* memory, uint32_t width, uint32_t height, float font_height,
                         const std::vector<Character>& characters, const char* out_filename)
{
	FILE* fp = fopen(out_filename, "w");
	if (fp == nullptr)
		throw std::runtime_error(std::string("Cannot open '") + out_filename + "'");

	DEFER({ fclose(fp); });

	fprintf(fp, "\n");
	fprintf(fp, "#include <stddef.h>\n");
	fprintf(fp, "\n");
	fprintf(fp, "static const size_t ATLAS_WIDTH = %u;\n", width);
	fprintf(fp, "static const size_t ATLAS_HEIGHT = %u;\n", height);
	fprintf(fp, "static const unsigned char ATLAS_DATA[] = \"");

	for (const uint8_t* p = memory; p < memory + static_cast<size_t>(width) * static_cast<size_t>(height); p += 1)
		fprintf(fp, "\\x%X", *p);

	fprintf(fp, "\";\n");

	if (true)
	{
		fprintf(fp, "\n");
		fprintf(fp, "#ifndef CHARACTER_METRIC_STRUCT\n");
		fprintf(fp, "#define CHARACTER_METRIC_STRUCT\n");
		fprintf(fp, "struct CharacterMetric\n");
		fprintf(fp, "{\n");
		fprintf(fp, "\tint width;\n");
		fprintf(fp, "\tint height;\n");
		fprintf(fp, "\tsize_t atlas_x;\n");
		fprintf(fp, "\tsize_t atlas_y;\n");
		fprintf(fp, "\tfloat x_offset;\n");
		fprintf(fp, "\tfloat y_offset;\n");
		fprintf(fp, "\tfloat advance;\n");
		fprintf(fp, "};\n");
		fprintf(fp, "#endif\n");

		fprintf(fp, "\n");
		fprintf(fp, "static const float FONT_HEIGHT = %.2ff;\n", font_height);
		fprintf(fp, "static const size_t CHARACTERS_NO = %zu;\n", characters.size());
		fprintf(fp, "static const size_t FIRST_CHARACTER_CODE = %u; // '%c'\n", characters.at(0).char_no,
		        static_cast<char>(characters.at(0).char_no));
		fprintf(fp, "\n");

		fprintf(fp, "static const struct CharacterMetric CHARACTERS_METRICS[CHARACTERS_NO] = {\n");
		uint32_t prev_no = characters.at(0).char_no - 1;
		for (const Character& ch : characters)
		{
			for (; ch.char_no > prev_no + 1; prev_no += 1)
				fprintf(fp, "\t{0, 0},\n");

			fprintf(fp, "\t{%u, %u, %u, %u, %.2ff, %.2ff, %.2ff}, // '%c'\n", ch.width, ch.height, ch.atlas_x,
			        ch.atlas_y, ch.x_offset, ch.y_offset, ch.advance, static_cast<char>(ch.char_no));
			prev_no = ch.char_no;
		}
		fprintf(fp, "};\n");
	}
}

static void sRenderAtlas(const char* in_filename, uint32_t pixel_size, uint32_t margin, const char* out_filename)
{
	assert((margin % 2) == 0);

	float font_height;

	auto characters = std::vector<Character>();
	auto data = std::vector<uint8_t>();

	// Use Freetype to get all what we need
	{
		auto get_freetype_error = [](FT_Error err) -> const char*
		{
			const char* str = FT_Error_String(err); // It may be null
			return (str != nullptr) ? str : "Unknown error";
		};

		FT_Error err;

		// Fixed formats used by Freetype,
		// (26.6 and 16.16 being more specific)
		const auto fixed_to_float = [](FT_Pos x) -> float { return static_cast<float>(x) / 64.0f; };
		const auto fixed_to_float2 = [](FT_Pos x) -> float { return static_cast<float>(x) / 65536.0f; };

		// Initialise library and face
		FT_Library library;
		if ((err = FT_Init_FreeType(&library)) != 0)
			throw std::runtime_error(std::string("FT_Init_FreeType() error: '") + get_freetype_error(err) + "'");

		DEFER({ FT_Done_FreeType(library); });

		{
			int major;
			int minor;
			int path;
			FT_Library_Version(library, &major, &minor, &path);

			printf("Freetype v%i.%i.%i\n", major, minor, path);
			printf("\n");
		}

		// Initialise
		FT_Face face;
		if ((err = FT_New_Face(library, in_filename, 0, &face)) != 0)
			throw std::runtime_error(std::string("FT_New_Face() error: '") + get_freetype_error(err) + "'");

		DEFER({ FT_Done_Face(face); });

		if ((err = FT_Set_Pixel_Sizes(face, 0, pixel_size)) != 0)
			throw std::runtime_error(std::string("FT_Set_Pixel_Sizes() error: '") + get_freetype_error(err) + "'");

		// const FT_Int spread = 8; // The default
		// FT_Property_Set(library, "sdf", "spread", &spread);
		// https://freetype.org/freetype2/docs/reference/ft2-properties.html#spread

		// Print metrics
		printf("units_per_EM: %u\n", face->units_per_EM);
		printf("x-ppem: %u\n", face->size->metrics.x_ppem);
		printf("y-ppem: %u\n", face->size->metrics.y_ppem);
		printf("x-scale: %.2f\n", fixed_to_float2(face->size->metrics.x_scale)); // ???
		printf("y-scale: %.2f\n", fixed_to_float2(face->size->metrics.y_scale));
		printf("height: %.2f (%ld)\n", fixed_to_float(face->size->metrics.height), face->size->metrics.height);

		{
			// "Height" is a postmodernist construction, what we really want is
			// the literal height of the 'x' character. But truly, font designers
			// seems to put whatever in the 'height' field... that or I'm miss
			// interpreting it, or miss usign Freetype (or both!)
			// (https://en.wikipedia.org/wiki/X-height)

			if (FT_Load_Char(face, 'x', FT_LOAD_DEFAULT) != 0)
				throw std::runtime_error(std::string("FT_Load_Char() error: '") + get_freetype_error(err) + "'");

			printf("x-height: %.2f (%ld)\n", fixed_to_float(face->glyph->metrics.height), face->glyph->metrics.height);
			printf("\n");

			font_height = fixed_to_float(face->glyph->metrics.height);
		}

		// Render ASCII characters
		FT_GlyphSlot g = face->glyph;

		for (int c = static_cast<int>(' '); c <= static_cast<int>('~'); c += 1)
		{
			// Render
			// https://stackoverflow.com/a/72072959

			// FT_LOAD_TARGET_LIGHT // Light hinting
			// FT_LOAD_TARGET_NORMAL // Stronger, default hinting
			// FT_LOAD_FORCE_AUTO // Ignore font's hinting

			if ((err = FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_DEFAULT)) != 0)
			{
				throw std::runtime_error(std::string("FT_Load_Char() error: '") + get_freetype_error(err) + "'");
			}

			if ((err = FT_Render_Glyph(g, FT_RENDER_MODE_SDF)) != 0)
				throw std::runtime_error(std::string("FT_Render_Glyph() error: '") + get_freetype_error(err) + "'");

			// Save metrics
			Character ch = {};
			ch.char_no = static_cast<uint32_t>(c);

			ch.width = g->bitmap.width;
			ch.height = g->bitmap.rows;
			ch.data_at = SIZE_MAX;

			ch.x_offset = fixed_to_float(g->metrics.horiBearingX) - 8.0f; // 8 is the SDF spread
			ch.y_offset = -(fixed_to_float(g->metrics.horiBearingY));
			ch.advance = fixed_to_float(g->metrics.horiAdvance);

			// Save bitmap data
			if (g->bitmap.buffer == nullptr || (ch.width == 0 && ch.height == 0)) // This is normal Freetype behaviour
			{
				if (static_cast<char>(c) != ' ')
					printf("Warning, character '%c' doesn't have data\n", static_cast<char>(c));

				goto done; // Nothing more to do
			}

			if (g->bitmap.pitch <= 0) // More normal Freetype behaviour
			{
				// Weird Freetype quirk, pitch can be negative
				// (an old BMP design from the 90s???)
				throw std::runtime_error("'g->bitmap.pitch' error");
			}

			{
				ch.data_at = data.size();

				const uint8_t* pixel = g->bitmap.buffer;
				for (uint32_t row = 0; row < ch.height; row += 1)
				{
					for (uint32_t col = 0; col < ch.width; col += 1)
						data.push_back(pixel[col]);
					pixel += g->bitmap.pitch;
				}
			}

		done:
			// Done
			characters.push_back(ch);
		}
	}

	// Pack characters into an atlas
	if (true)
	{
		std::sort(characters.begin(), characters.end(),
		          [](const Character& a, const Character& b) { return a.height > b.height; });

		// Find a square size by making tentative packings
		// (we are a offline tool, we have all the time in the world)
		uint32_t dimension = 32;
		while (true)
		{
			uint32_t max_height = 0;
			uint32_t acc_x = 0;
			uint32_t acc_y = 0;
			for (const Character& c : characters)
			{
				if (c.data_at == SIZE_MAX)
					continue;

				if (acc_x + (c.width + margin) >= dimension)
				{
					acc_x = 0;
					acc_y += max_height;
					max_height = 0;
					if (acc_y > 8192)
						throw std::runtime_error("Atlas grew too big");
				}

				if ((acc_y + c.height) >= dimension)
				{
					// dimension <<= 1; // Faster and power of two, but wastes space
					dimension += (max_height + margin); // Slower and non power of two, but precise
					goto outside_continue;
				}

				acc_x += (c.width + margin);
				max_height = std::max(max_height, (c.height + margin));
			}

			if (acc_y < dimension)
				break;
		outside_continue:;
		}

		// Allocate memory
		printf("Atlas size: %u x %u px\n", dimension, dimension);

		auto memory = reinterpret_cast<uint8_t*>(malloc(sizeof(uint8_t) * dimension * dimension));
		DEFER({ free(memory); });

		memset(memory, 0, sizeof(uint8_t) * dimension * dimension);

		// Pack for real this time
		{
			auto blit =
			    [](const uint8_t* in, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t dest_w, uint8_t* dest)
			{
				dest += static_cast<size_t>(x + y * dest_w);
				for (uint8_t* row = dest; row < dest + static_cast<size_t>(h * dest_w); row += dest_w)
				{
					for (uint8_t* col = row; col < row + w; col += 1)
					{
						*col = *in++;
					}
				}
			};

			uint32_t max_height = 0;
			uint32_t acc_x = 0;
			uint32_t acc_y = 0;
			for (Character& ch : characters)
			{
				if (ch.data_at == SIZE_MAX)
					continue;

				if (acc_x + (ch.width + margin) > dimension)
				{
					acc_x = 0;
					acc_y += max_height;
					max_height = 0;
				}

				ch.atlas_x = (acc_x + margin / 2);
				ch.atlas_y = (acc_y + margin / 2);
				blit(data.data() + ch.data_at, (acc_x + margin / 2), (acc_y + margin / 2), ch.width, ch.height,
				     dimension, memory);

				acc_x += (ch.width + margin);
				max_height = std::max(max_height, (ch.height + margin));
			}
		}

		// Bye!
		std::sort(characters.begin(), characters.end(),
		          [](const Character& a, const Character& b) { return a.char_no < b.char_no; });

		sWriteDdsAlpha8(memory, dimension, dimension, dimension, "test.dds");
		sWriteHeader(memory, dimension, dimension, font_height, characters, out_filename);
	}
}


int main(int argc, const char* argv[])
{
	if (argc <= 2)
		return 0;

	try
	{
		sRenderAtlas(argv[1], 15, 2, argv[2]);
	}
	catch (const std::exception& err)
	{
		fprintf(stderr, "%s\n", err.what());
		return 1;
	}

	return 0;
}
