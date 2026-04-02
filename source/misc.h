/*

Copyright (c) 2026 Alexander Brandt

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

This Source Code Form is "Incompatible With Secondary Licenses", as
defined by the Mozilla Public License, v. 2.0.
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

#endif
