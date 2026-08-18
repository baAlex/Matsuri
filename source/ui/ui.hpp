/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef UI_HPP
#define UI_HPP

#include "yuika.hpp"

// #define MATSURI_UI_X11 // CMake should define this

#ifdef MATSURI_UI_X11
extern "C"
{
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}
#endif

class Ui
{
	int m_width;
	int m_height;
	size_t m_buffer_size;
	void* m_buffer;

	yuika::Screen m_yui;

  public: // Yes, some members are used from outside, thing is that,
	      // I don't want to put CLAP procedures here
#ifdef MATSURI_UI_X11
	Display* m_x11_display;
	Window m_x11_window;
	XImage* m_x11_image;
#endif

	void Initialise(int width, int height);
	void Deinitialise() noexcept;

#ifdef MATSURI_UI_X11
	void SetParent(Window parent_window);
	void OnFd() noexcept;
#endif

	void Show();
	void Hide();

	void Resize(int width, int height);
};

#endif
