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
#include <stdexcept>


#define MATSURI_UI_X11 1
#define MATSURI_UI_WIN32 2


class BrokenState : public std::runtime_error
{
  public:
	BrokenState() : std::runtime_error("Broken state, another error may happened before") {};
};


extern "C"
{
#if (MATSURI_UI == MATSURI_UI_X11)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#elif (MATSURI_UI == MATSURI_UI_WIN32)
#include <windows.h>
#endif
}


class Ui
{
  public:
	int m_width;
	int m_height;
	size_t m_buffer_size;
	void* m_buffer;

	yuika::Screen m_yui;

#if (MATSURI_UI == MATSURI_UI_X11)
	Display* m_x11_display;
	Window m_x11_window;
	XImage* m_x11_image;
#elif (MATSURI_UI == MATSURI_UI_WIN32)
	HWND m_win32_window;
#endif

	void Initialise(int width, int height);
	void Deinitialise() noexcept;

#if (MATSURI_UI == MATSURI_UI_X11)
	void SetParent(Window parent_window);
	void OnFd();
#elif (MATSURI_UI == MATSURI_UI_WIN32)
	void SetParent_(HWND parent_window);
#endif

	void Show();
	void Hide();

	void Resize(int width, int height);
};

#endif
