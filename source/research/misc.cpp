/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <assert.h>
#include <stdio.h>

#include "misc.hpp"


int SaveBMP16(const char* filename, int width, int height, const uint16_t* buffer)
{
	FILE* fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		fprintf(stderr, "fopen() error\n");
		return 1;
	}

	const uint32_t row_size = static_cast<uint32_t>(width) * sizeof(uint16_t);

	// We want align to 4 bytes:
	const uint32_t stride = (row_size + (sizeof(uint32_t) - 1)) & ~(sizeof(uint32_t) - 1);

	const uint32_t image_size = stride * static_cast<uint32_t>(height);
	const uint32_t data_offset = 14 + 40 + 12;
	const uint32_t file_size = data_offset + image_size;

	// Header (14 bytes)
	{
		const uint16_t reserved = 0;

		if (fwrite("BM", 2, 1, fp) != 1 ||       //
		    fwrite(&file_size, 4, 1, fp) != 1 || //
		    fwrite(&reserved, 2, 1, fp) != 1 ||  //
		    fwrite(&reserved, 2, 1, fp) != 1 ||  //
		    fwrite(&data_offset, 4, 1, fp) != 1)
		{
			fprintf(stderr, "fwrite() error\n");
			fclose(fp);
			return 1;
		}
	}

	// InfoHeader (40 bytes)
	{
		const uint32_t info_header_size = 40;
		const int32_t flipped_height = -height; // top-down
		const uint16_t planes = 1;
		const uint16_t bits_per_pixel = 16;
		const uint32_t compression = 3;       // BI_BITFIELDS
		const int32_t x_pixels_per_meter = 0; // ???
		const int32_t y_pixels_per_meter = 0; // ???
		const uint32_t colours_used = 0;      // For indexed only
		const uint32_t important_colours = 0; // Same

		if (fwrite(&info_header_size, 4, 1, fp) != 1 ||   //
		    fwrite(&width, 4, 1, fp) != 1 ||              //
		    fwrite(&flipped_height, 4, 1, fp) != 1 ||     //
		    fwrite(&planes, 2, 1, fp) != 1 ||             //
		    fwrite(&bits_per_pixel, 2, 1, fp) != 1 ||     //
		    fwrite(&compression, 4, 1, fp) != 1 ||        //
		    fwrite(&image_size, 4, 1, fp) != 1 ||         //
		    fwrite(&x_pixels_per_meter, 4, 1, fp) != 1 || //
		    fwrite(&y_pixels_per_meter, 4, 1, fp) != 1 || //
		    fwrite(&colours_used, 4, 1, fp) != 1 ||       //
		    fwrite(&important_colours, 4, 1, fp) != 1)
		{
			fprintf(stderr, "fwrite() error\n");
			fclose(fp);
			return 1;
		}
	}

	// Bitfields (12 bytes)
	{
		// https://www.virtualdub.org/blog2/entry_177.html
		const uint32_t r_mask = 0xF800; // 0b1111100000000000;
		const uint32_t g_mask = 0x7C0;  // 0b0000011111000000;
		const uint32_t b_mask = 0x3E;   // 0b0000000000111110;

		if (fwrite(&r_mask, 4, 1, fp) != 1 || //
		    fwrite(&g_mask, 4, 1, fp) != 1 || //
		    fwrite(&b_mask, 4, 1, fp) != 1)
		{
			fprintf(stderr, "fwrite() error\n");
			fclose(fp);
			return 1;
		}
	}

	// Data
	{
		const uint8_t zero[4] = {0, 0, 0, 0};
		const uint32_t padding = stride - row_size;

		for (int y = 0; y < height; y += 1)
		{
			const uint16_t* row = buffer + y * width;

			if (fwrite(row, row_size, 1, fp) != 1)
			{
				fprintf(stderr, "fwrite() error\n");
				fclose(fp);
				return 1;
			}

			if (padding == 0)
				continue;

			if (fwrite(zero, padding, 1, fp) != 1)
			{
				fprintf(stderr, "fwrite() error\n");
				fclose(fp);
				return 1;
			}
		}
	}

	fclose(fp);
	return 0;
}
