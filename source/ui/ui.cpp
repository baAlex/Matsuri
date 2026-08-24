/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.hpp"

extern "C"
{
#include "../version.h"
}


// ############################


static thread_local int s_scary_shining_red_button = 0; // 'thread_local' is pretty much what I can do,
                                                        // other solutions depends on how DAWs handle
                                                        // plugins.


#if (MATSURI_UI == MATSURI_UI_X11)
// Error handling robbed from GLFW (if I'm going to copy, I'm going to copy from the best):
// https://github.com/glfw/glfw/blob/92dcf4ce74f2e2554a98fea09be7c705c17daa5a/src/x11_init.c#L1098

// And not just the functions, the style as well.

/*
Copyright (c) 2002-2006 Marcus Geelnard

Copyright (c) 2006-2019 Camilla Löwy

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would
   be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.
*/

static thread_local XErrorHandler s_X11_previous_error_handler;

static int sX11ErrorHandler(Display* display, XErrorEvent* event)
{
	s_scary_shining_red_button = 1;

	{
		char text[256];
		strcpy(text, "X11 error: ");
		XGetErrorText(display, event->error_code, text + strlen("X11 error: "),
		              static_cast<int>(sizeof(text) - strlen("X11 error: ")));

		fprintf(stderr, "X11 error: \"%s\"\n", text);
		throw std::runtime_error(text);
	}

	return 0;
}

void sGrabX11ErrorHandler()
{
	s_X11_previous_error_handler = XSetErrorHandler(sX11ErrorHandler);
}

void sReleaseX11ErrorHandler(Display* display)
{
	XSync(display, False); // TODO, shouldn't this be after XSetErrorHandler?
	                       // EDIT, observation shows that's fine
	XSetErrorHandler(s_X11_previous_error_handler);
	s_X11_previous_error_handler = nullptr;
}
#endif


// ############################


#if (MATSURI_UI == MATSURI_UI_WIN32)
static int globalOpenGUICount = 0;
LRESULT CALLBACK GUIWindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
#endif

void Ui::Initialise(int width, int height)
{
	// We are a plugin, many instances can be initialised... but that
	// doesn't mean that a broken API will somehow work on a second try
	if (s_scary_shining_red_button != 0)
		throw BrokenState();

	// Allocate buffer
	m_width = width;
	m_height = height;
	m_buffer_size = static_cast<size_t>(width * height) * sizeof(uint32_t);
	if ((m_buffer = realloc(nullptr, m_buffer_size)) == nullptr)
		throw std::bad_alloc();

	// Specific API things
#if (MATSURI_UI == MATSURI_UI_X11)
	{
		// Create display,
		// GLFW checks nullity
		// https://github.com/glfw/glfw/blob/92dcf4ce74f2e2554a98fea09be7c705c17daa5a/src/x11_init.c#L1292
		if ((m_x11_display = XOpenDisplay(nullptr)) == nullptr)
			throw std::runtime_error("Cannot open default X11 display");

		// Create window,
		// GLFW uses error handler here
		// https://github.com/glfw/glfw/blob/92dcf4ce74f2e2554a98fea09be7c705c17daa5a/src/x11_window.c#L609
		sGrabX11ErrorHandler();
		{
			XSetWindowAttributes attributes = {};
			m_x11_window = XCreateWindow(m_x11_display, DefaultRootWindow(m_x11_display), 0, 0,
			                             static_cast<unsigned int>(m_width), static_cast<unsigned int>(m_height), 0, 0,
			                             InputOutput, CopyFromParent, CWOverrideRedirect, &attributes);
		}
		sReleaseX11ErrorHandler(m_x11_display);

		if (m_x11_display == nullptr)
			throw std::runtime_error("Cannot create X11 window");

		// Embeddable property, name, and inputs received
		// GLFW is not checking for properties nor hints errors
		// https://github.com/glfw/glfw/blob/92dcf4ce74f2e2554a98fea09be7c705c17daa5a/src/x11_window.c#L692
		sGrabX11ErrorHandler(); // ... but differently to GLFW, we don't own a window, so we better catch
		{                       // error before the default error handler does it
			const Atom embed_info_atom = XInternAtom(m_x11_display, "_XEMBED_INFO", False);

			uint32_t embed_info_data[2] = {0 /* version */, 0 /* not mapped */};
			XChangeProperty(m_x11_display, m_x11_window, embed_info_atom, embed_info_atom, 32, PropModeReplace,
			                reinterpret_cast<uint8_t*>(embed_info_data), 2);

			XStoreName(m_x11_display, m_x11_window, MATSURI_NAME);

			XSelectInput(m_x11_display, m_x11_window,
			             SubstructureNotifyMask | ExposureMask | PointerMotionMask | ButtonPressMask |
			                 ButtonReleaseMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | EnterWindowMask |
			                 LeaveWindowMask | ButtonMotionMask | KeymapStateMask | FocusChangeMask |
			                 PropertyChangeMask);
		}
		sReleaseX11ErrorHandler(m_x11_display);

		// Create image
		sGrabX11ErrorHandler();
		{
			m_x11_image = XCreateImage(m_x11_display, DefaultVisual(m_x11_display, 0),
			                           24,      // depth
			                           ZPixmap, // format (XYBitmap, XYPixmap, or ZPixmap)
			                           0,       // offset
			                           nullptr, // data
			                           1,       // width
			                           1,       // height
			                           32,      // bitmap_pad
			                           0        // bytes_per_line
			);

			// """ The XCreateImage function allocates the memory needed for an XImage structure for the specified
			// display but does not allocate space for the image itself. """
			// (https://xorg.freedesktop.org/releases/current/doc/libX11/libX11/libX11.html#XCreateImage)

			// """ The red, green, and blue mask values are defined for Z format images only and are derived from the
			// Visual structure passed in. """ (ditto)

			// """ XDestroyImage function calls frees both the image structure and the data pointed to by the image
			// structure. """ (ditto)

			// """ The red, green, and blue mask values are defined for Z format images only and are derived from the
			// Visual structure passed in. """ (ditto)

			m_x11_image->width = m_width;
			m_x11_image->height = m_height;
			m_x11_image->bytes_per_line = m_width * static_cast<int>(sizeof(uint32_t));

			m_x11_image->data = reinterpret_cast<char*>(m_buffer);
		}
		sReleaseX11ErrorHandler(m_x11_display);

		if (m_x11_image == nullptr)
			throw std::runtime_error("Cannot create X11 image");

		// Initialise Yui and draw first frame
		{
			const auto v = DefaultVisual(m_x11_display, 0);

			m_yui.Initialise(static_cast<uint32_t>(v->red_mask), static_cast<uint32_t>(v->green_mask),
			                 static_cast<uint32_t>(v->blue_mask));

			m_yui.Update({m_width, m_height}, reinterpret_cast<uint32_t*>(m_buffer));
		}
	}
#elif (MATSURI_UI == MATSURI_UI_WIN32)
	{
		// MessageBoxW(nullptr, L"Ui::Initialise", L"Matsuri", MB_OK);

		if (globalOpenGUICount == 0)
		{
			WNDCLASS temp = {};
			temp.lpfnWndProc = GUIWindowProcedure;
			temp.cbWndExtra = sizeof(Ui*); // TODO, read reference to confirm what I think it is
			temp.lpszClassName = MATSURI_URI;
			temp.hCursor = LoadCursor(nullptr, IDC_ARROW);
			temp.style = CS_DBLCLKS;

			RegisterClass(&temp);
			globalOpenGUICount++;
		}

		m_win32_window = CreateWindow(MATSURI_URI, MATSURI_NAME, WS_CHILDWINDOW | WS_CLIPSIBLINGS, CW_USEDEFAULT, 0,
		                              m_width, m_height, GetDesktopWindow(), nullptr, nullptr, nullptr);

		SetWindowLongPtr(m_win32_window, 0, (LONG_PTR)(this)); // TODO, read reference to confirm what I think it is

		// Initialise Yui and draw first frame
		{
			m_yui.Initialise(0x00FF0000, 0x0000FF00, 0x000000FF); // BI_RGB below
			m_yui.Update({m_width, m_height}, reinterpret_cast<uint32_t*>(m_buffer));
		}
	}
#endif
}


void Ui::Deinitialise() noexcept
{
	// Free our stuff
	free(m_buffer);

	// Go back?, specific API objects can stay where they are,
	// a broken X11 is horrible, it can crash the entire DAW
	if (s_scary_shining_red_button != 0)
		return;

	// Specific API things
#if (MATSURI_UI == MATSURI_UI_X11)
	{
		m_x11_image->data = nullptr; // XDestroyImage() also free data
		XDestroyImage(m_x11_image);

		XDestroyWindow(m_x11_display, m_x11_window);
		XCloseDisplay(m_x11_display);
	}
#endif
}


#if (MATSURI_UI == MATSURI_UI_X11)
void Ui::SetParent(Window parent_window)
{
	if (s_scary_shining_red_button != 0)
		throw BrokenState();

	// Running "QT_QPA_PLATFORM=xcb qtractor" makes XReparentWindow() succeed,
	// on the other hand "QT_QPA_PLATFORM=wayland qtractor" succeeds then it crashes
	// everything.

	// And it's not Qtractor fault, but Qt, Wayland has a X11 compatibility layer so
	// it's doing fine on pretending being compatible, it's Qt doing the glue wrong.

	// Here, what it seems to be an invalid 'parent_window' is being pass:
	// https://github.com/rncbc/qtractor/blob/04b568651c70cc8e6bd135b79e47d270314c913c/src/qtractorClapPlugin.cpp#L3128

	sGrabX11ErrorHandler();
	{
		XReparentWindow(m_x11_display, m_x11_window, parent_window, 0, 0);
		XFlush(m_x11_display);
	}
	sReleaseX11ErrorHandler(m_x11_display);
}
#elif (MATSURI_UI == MATSURI_UI_WIN32)
void Ui::SetParent_(HWND parent_window)
{
	SetParent(m_win32_window, parent_window); // TODO, check error
}
#endif


void Ui::Show()
{
	if (s_scary_shining_red_button != 0)
		throw BrokenState();

#if (MATSURI_UI == MATSURI_UI_X11)
	sGrabX11ErrorHandler();
	{
		XMapRaised(m_x11_display, m_x11_window);
		XFlush(m_x11_display);
	}
	sReleaseX11ErrorHandler(m_x11_display);

#elif (MATSURI_UI == MATSURI_UI_WIN32)
	ShowWindow(m_win32_window, SW_SHOW);
#endif
}

void Ui::Hide()
{
	if (s_scary_shining_red_button != 0)
		throw BrokenState();

#if (MATSURI_UI == MATSURI_UI_X11)
	sGrabX11ErrorHandler();
	{
		XUnmapWindow(m_x11_display, m_x11_window);
		XFlush(m_x11_display);
	}
	sReleaseX11ErrorHandler(m_x11_display);

#elif (MATSURI_UI == MATSURI_UI_WIN32)
	ShowWindow(m_win32_window, SW_HIDE);
#endif
}


void Ui::Resize(int width, int height)
{
	if (s_scary_shining_red_button != 0)
		throw BrokenState();

	if (width == m_width && height == m_height)
		return;

	// Reallocate buffer?
	if (static_cast<size_t>(width * height) * sizeof(uint32_t) > m_buffer_size)
	{
		m_buffer_size = static_cast<size_t>(width * height) * sizeof(uint32_t);
		m_buffer_size = (m_buffer_size * 3) / 2;

		// printf("%zu, %f MB\n", m_buffer_size, static_cast<double>(m_buffer_size) / 1000.0 / 1000.0);

		void* prev = m_buffer;
		if ((m_buffer = realloc(m_buffer, m_buffer_size)) == nullptr)
		{
			m_buffer = prev;
			throw std::bad_alloc();
		}
	}

	// Update Yui
	m_width = width;
	m_height = height;
	m_yui.Update({m_width, m_height}, reinterpret_cast<uint32_t*>(m_buffer));

	// Update specific API
#if (MATSURI_UI == MATSURI_UI_X11)
	sGrabX11ErrorHandler();
	{
		m_x11_image->width = m_width;
		m_x11_image->height = m_height;
		m_x11_image->bytes_per_line = m_width * static_cast<int>(sizeof(uint32_t));
		m_x11_image->data = reinterpret_cast<char*>(m_buffer);

		XResizeWindow(m_x11_display, m_x11_window, static_cast<unsigned int>(m_x11_image->width),
		              static_cast<unsigned int>(m_x11_image->height));
		XPutImage(m_x11_display, m_x11_window, DefaultGC(m_x11_display, 0), m_x11_image, 0, 0, 0, 0,
		          static_cast<unsigned int>(m_x11_image->width), static_cast<unsigned int>(m_x11_image->height));
	}
	sReleaseX11ErrorHandler(m_x11_display);
#endif
}


#if (MATSURI_UI == MATSURI_UI_X11)
void Ui::OnFd()
{
	if (s_scary_shining_red_button != 0)
		throw BrokenState();

	sGrabX11ErrorHandler();
	{
		XFlush(m_x11_display);

		while (XPending(m_x11_display))
		{
			XEvent event;
			XNextEvent(m_x11_display, &event);

			if (event.type == Expose)
			{
				if (event.xexpose.window == m_x11_window)
				{
					// TODO, Am I doing this right?
					XPutImage(m_x11_display, m_x11_window, DefaultGC(m_x11_display, 0), m_x11_image, event.xexpose.x,
					          event.xexpose.y, event.xexpose.x, event.xexpose.y,
					          static_cast<unsigned int>(event.xexpose.width),
					          static_cast<unsigned int>(event.xexpose.height));
				}
			}

			XFlush(m_x11_display);
		}
	}
	sReleaseX11ErrorHandler(m_x11_display);
}
#elif (MATSURI_UI == MATSURI_UI_WIN32)
LRESULT CALLBACK GUIWindowProcedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	Ui* self = (Ui*)(GetWindowLongPtr(window, 0)); // TODO, read reference to confirm what I think it is

	if (self == nullptr) // TODO What does this means?, is not a message for us?
		return DefWindowProc(window, message, w_param, l_param);

	if (message == WM_PAINT)
	{
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(window, &paint);
		BITMAPINFO info = {{sizeof(BITMAPINFOHEADER), self->m_width, -self->m_height, 1, 32, BI_RGB}};
		StretchDIBits(dc, 0, 0, self->m_width, self->m_height, 0, 0, self->m_width, self->m_height, self->m_buffer,
		              &info, DIB_RGB_COLORS, SRCCOPY); // TODO, check error
		EndPaint(window, &paint); // TODO, same
	}
	/*else if (message == WM_MOUSEMOVE)
	{
	    PluginProcessMouseDrag(self, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
	    GUIPaint(self, true);
	}
	else if (message == WM_LBUTTONDOWN)
	{
	    SetCapture(window);
	    PluginProcessMousePress(self, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
	    GUIPaint(self, true);
	}
	else if (message == WM_LBUTTONUP)
	{
	    ReleaseCapture();
	    PluginProcessMouseRelease(self);
	    GUIPaint(self, true);
	}*/
	else
	{
		return DefWindowProc(window, message, w_param, l_param);
	}

	return 0;
}
#endif
