/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef ERROR_HPP
#define ERROR_HPP

#include <stddef.h>

struct Error
{
	static const size_t BUFFERS_LEN = 128;

	char message[BUFFERS_LEN];
	char item[BUFFERS_LEN];

	Error(const char* message = "");
	Error(const char* format, const char* item, const char* fallback);
	Error(const char* format, char item, const char* fallback);
};

#endif
