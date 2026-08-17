/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef MATSURI_UI_UI_HPP
#define MATSURI_UI_UI_HPP

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
	yuika::Screen m_yui;

#ifdef MATSURI_UI_X11
	Display* m_display;
	Window m_window;
	XImage* m_image;
#endif

  public:
	void Initialise(int width, int height);
	void Deinitialise() noexcept;

#ifdef MATSURI_UI_X11
	void SetParent(Window parent_window);
	void OnFd() noexcept;
#endif

	void Show();
	void Hide();
};

#endif
