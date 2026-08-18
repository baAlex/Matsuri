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

#include "ui.hpp"

extern "C"
{
#include "../version.h"
}


#ifdef MATSURI_UI_X11
static int sX11ErrorHandler(Display* display, XErrorEvent* error)
{
	char text[256];

	XGetErrorText(display, error->error_code, text, sizeof(text));
	fprintf(stderr, "X11 error: %s\n", text); // TODO?

	// """ it is acceptable for your error handler to return; the returned value is ignored. However, the error handler
	// should not call any functions """
	// (https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetErrorHandler.html)
	return 0;
}
#endif


void Ui::Initialise(int width, int height)
{
	// Allocate buffer
	m_width = width;
	m_height = height;
	m_buffer_size = static_cast<size_t>(width * height) * sizeof(uint32_t);
	if ((m_buffer = malloc(m_buffer_size)) == nullptr)
		throw 666;

	// Initialise Yui and draw first frame
	m_yui.Initialise();
	m_yui.Update({m_width, m_height}, m_buffer);

	// X11
#ifdef MATSURI_UI_X11
	{
		// Set error handler,
		// X11 has a server-client model, so most operations will not execute immediately, and
		// so errors are caught reaaallllyy late
		XSetErrorHandler(sX11ErrorHandler);

		// Create display
		if ((m_x11_display = XOpenDisplay(NULL)) == nullptr)
		{
			// It seems that most X11 returns random stuff, even if the reference say otherwise,
			// the error handler seems to be the only one with a proper final word

			// For example, XChangeProperty always fails:
			// https://stackoverflow.com/a/11354589

			// Same happens with XReparentWindow
		}

		// Create window
		XSetWindowAttributes attributes = {};
		m_x11_window = XCreateWindow(m_x11_display, DefaultRootWindow(m_x11_display), 0, 0,
		                             static_cast<unsigned int>(m_width), static_cast<unsigned int>(m_height), 0, 0,
		                             InputOutput, CopyFromParent, CWOverrideRedirect, &attributes);

		// Embeddable property
		const Atom embed_info_atom = XInternAtom(m_x11_display, "_XEMBED_INFO", False);

		uint32_t embed_info_data[2] = {0 /* version */, 0 /* not mapped */};
		XChangeProperty(m_x11_display, m_x11_window, embed_info_atom, embed_info_atom, 32, PropModeReplace,
		                reinterpret_cast<uint8_t*>(embed_info_data), 2);

		// Some more properties
		XStoreName(m_x11_display, m_x11_window, MATSURI_NAME);

		XSelectInput(m_x11_display, m_x11_window,
		             SubstructureNotifyMask | ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
		                 KeyPressMask | KeyReleaseMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask |
		                 ButtonMotionMask | KeymapStateMask | FocusChangeMask | PropertyChangeMask);

		// Create image
		m_x11_image =
		    XCreateImage(m_x11_display, DefaultVisual(m_x11_display, 0), 24, ZPixmap, 0, nullptr, 1, 1, 32, 0);

		m_x11_image->width = m_width;
		m_x11_image->height = m_height;
		m_x11_image->bytes_per_line = m_width * static_cast<int>(sizeof(uint32_t));

		m_x11_image->data = reinterpret_cast<char*>(m_buffer);

		// All done!
		XSync(m_x11_display, False); // This blocks until X11 server process all instructions, triggering
		                             // error handler if it's the case
	}
#endif
}


void Ui::Deinitialise() noexcept
{
	free(m_buffer);

#ifdef MATSURI_UI_X11
	{
		m_x11_image->data = nullptr; // XDestroyImage() also free data
		XDestroyImage(m_x11_image);

		XDestroyWindow(m_x11_display, m_x11_window);
		XCloseDisplay(m_x11_display);
	}
#endif
}


#ifdef MATSURI_UI_X11
void Ui::SetParent(Window parent_window)
{
	// Running "QT_QPA_PLATFORM=xcb qtractor" makes XReparentWindow() succeed,
	// on the other hand "QT_QPA_PLATFORM=wayland qtractor" succeeds then it crashes
	// everything.

	// And it's not Qtractor fault, but Qt, Wayland has a X11 compatibility layer so
	// it's doing fine on pretending being compatible, it's Qt doing the glue wrong.

	// Here, what it seems to be an invalid 'parent_window' is being pass:
	// https://github.com/rncbc/qtractor/blob/04b568651c70cc8e6bd135b79e47d270314c913c/src/qtractorClapPlugin.cpp#L3128

	XReparentWindow(m_x11_display, m_x11_window, parent_window, 0, 0);
	XFlush(m_x11_display);
	XSync(m_x11_display, False); // I need to crash here
}
#endif


void Ui::Show()
{
#ifdef MATSURI_UI_X11
	XMapRaised(m_x11_display, m_x11_window);
	XFlush(m_x11_display);
#endif
}

void Ui::Hide()
{
#ifdef MATSURI_UI_X11
	XUnmapWindow(m_x11_display, m_x11_window);
	XFlush(m_x11_display);
#endif
}


void Ui::Resize(int width, int height)
{
	if (width == m_width && height == m_height)
		return;

	// Reallocate buffer?
	if (static_cast<size_t>(width * height) * sizeof(uint32_t) > m_buffer_size)
	{
		m_buffer_size = static_cast<size_t>(width * height) * sizeof(uint32_t);

		void* prev = m_buffer;
		if ((m_buffer = realloc(m_buffer, m_buffer_size)) == nullptr)
		{
			m_buffer = prev;
			// throw 666; // TODO?
			return;
		}
	}

	// Update Yui
	m_width = width;
	m_height = height;
	m_yui.Update({m_width, m_height}, m_buffer);

	// Update window
#ifdef MATSURI_UI_X11
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
#endif
}


#ifdef MATSURI_UI_X11
void Ui::OnFd() noexcept
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
				// Am I doing this right?
				XPutImage(m_x11_display, m_x11_window, DefaultGC(m_x11_display, 0), m_x11_image, event.xexpose.x,
				          event.xexpose.y, event.xexpose.x, event.xexpose.y,
				          static_cast<unsigned int>(event.xexpose.width),
				          static_cast<unsigned int>(event.xexpose.height));
			}
		}

		XFlush(m_x11_display);
	}
}
#endif
