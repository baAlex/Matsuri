/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef MISC_HPP
#define MISC_HPP

#include <stdint.h>

// My first time differentiating vectors based on their use:
// (I remember a Naughty Dog's developer article on this)
// (so far I'm not buying the idea)

struct Size
{
	float w; // In EM
	float h; // In EM
};

struct Position
{
	float x; // In EM
	float y; // In EM
};

struct Delta
{
	float x; // In EM
	float y; // In EM
};

struct Rect
{
	// I want to tell the compiler that these are contiguous:
	// (that's why Rect exists)(... and it's for SIMD purposes)
	Position pos;
	Size size;
};

// clang-format off
template <typename T> constexpr T Min(T a, T b) { return (a < b) ? a : b; }
template <typename T> constexpr T Max(T a, T b) { return (a > b) ? a : b; }
template <typename T> constexpr T Clamp(T x, T min, T max) { return Min(Max(x, min), max); }
// clang-format on

int SaveBMP16(const char* filename, int width, int height, const uint16_t* buffer);

#endif
