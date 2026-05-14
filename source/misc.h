/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef MISC_H
#define MISC_H

#include <stddef.h>
#include <stdint.h>

// https://stackoverflow.com/a/2164853
#if defined(_MSC_VER)
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
#define EXPORT __attribute__((visibility("default")))
#define IMPORT
#else
#define EXPORT //
#define IMPORT //
#pragma warning Unknown dynamic link import / export semantics.
#endif

void* Memset(void* output, int c, size_t len);
uint32_t Xorshift(uint32_t x);

int MinI(int a, int b);
uint32_t MinU(uint32_t a, uint32_t b);
float MinF(float a, float b);
float MaxF(float a, float b);

float ExponentialVolumeEasing(float x);

#endif
