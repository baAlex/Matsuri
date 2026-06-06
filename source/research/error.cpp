/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "error.hpp"


// The idea is to be as freestanding as possible


static char* sStrncpy(char* dest, const char* src, size_t dest_length) noexcept
{
	for (; *src != '\0' && dest_length > 1; dest_length -= 1)
		*dest++ = *src++;
	*dest = '\0';

	return dest;
}

static char* sStrecpy(char* dest, const char* dest_end, const char* str) noexcept
{
	if (dest >= dest_end)
		return dest;

	while (*str != '\0' && dest < dest_end - 1)
		*dest++ = *str++;
	*dest = '\0'; // Is far better to overwrite '\0's that not having them

	return dest;
}

static char* sFastFormat(char* dest, const char* dest_end, const char* format, const char* value) noexcept
{
	while (*format != '\0' && dest < dest_end - 1)
	{
		if (*format != '%')
			*dest++ = *format++;
		else
		{
			dest = sStrecpy(dest, dest_end, value);
			format += 1;
		}
	}
	*dest = '\0';

	return dest;
}

static char* sFastFormat(char* dest, const char* dest_end, const char* format, char value) noexcept
{
	while (*format != '\0' && dest < dest_end - 1)
	{
		if (*format != '%')
			*dest++ = *format++;
		else
		{
			*dest++ = value;
			format += 1;
		}
	}
	*dest = '\0';

	return dest;
}


Error::Error(const char* message)
{
	sStrncpy(this->message, message, BUFFERS_LEN);
	this->item[0] = '\0';
}

Error::Error(const char* format, const char* item, const char* fallback)
{
	if (*item != '\0')
	{
		sFastFormat(this->message, this->message + BUFFERS_LEN, format, item);
		sStrecpy(this->item, this->item + BUFFERS_LEN, "");
	}
	else
	{
		sStrecpy(this->message, this->message + BUFFERS_LEN, fallback);
		this->item[0] = '\0';
	}
}

Error::Error(const char* format, char item, const char* fallback)
{
	if (item != '\0')
	{
		sFastFormat(this->message, this->message + BUFFERS_LEN, format, item);
		this->item[0] = item;
		this->item[1] = '\0';
	}
	else
	{
		sStrecpy(this->message, this->message + BUFFERS_LEN, fallback);
		this->item[0] = '\0';
	}
}
