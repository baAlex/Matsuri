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

static int sErrorHandler(Display* display, XErrorEvent* error)
{
	char text[256];

	XGetErrorText(display, error->error_code, text, sizeof(text));
	fprintf(stderr, "X11 error: %s\n", text); // TODO?

	// """ it is acceptable for your error handler to return; the returned value is ignored. However, the error handler
	// should not call any functions """
	// (https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetErrorHandler.html)
	return 0;
}

void Ui::Initialise(int width, int height)
{
	// Set error handler,
	// X11 has a server-client model, so most operations will not
	// execute immediately, and so errors are caught reaaallllyy late
	XSetErrorHandler(sErrorHandler);

	// Create display
	if ((m_display = XOpenDisplay(NULL)) == nullptr)
	{
		// FIXME, it seems that most X11 returns random stuff, even if the reference say otherwise,
		// the error handler seems to be the only one with a proper final word
		// throw 666;
	}

	// Create window
	XSetWindowAttributes attributes = {};
	if ((m_window = XCreateWindow(m_display, DefaultRootWindow(m_display), 0, 0, static_cast<unsigned int>(width),
	                              static_cast<unsigned int>(height), 0, 0, InputOutput, CopyFromParent,
	                              CWOverrideRedirect, &attributes)) == None)
	{
		// throw 666;
	}

	// Embeddable property
	const Atom embed_info_atom = XInternAtom(m_display, "_XEMBED_INFO", False);
	// if (embed_info_atom == None)
	// throw 666;

	uint32_t embed_info_data[2] = {0 /* version */, 0 /* not mapped */};
	if (XChangeProperty(m_display, m_window, embed_info_atom, embed_info_atom, 32, PropModeReplace,
	                    reinterpret_cast<uint8_t*>(embed_info_data), 2) != Success)
	{
		// XChangeProperty always fails!!!:
		// https://stackoverflow.com/a/11354589

		// throw 666;
	}

	// Some more properties, I mean, hints
	{
		XSizeHints size_hints = {};
		size_hints.flags = PMinSize;
		size_hints.min_width = 320;
		size_hints.min_height = 240;

		XSetWMNormalHints(m_display, m_window, &size_hints);
		XStoreName(m_display, m_window, MATSURI_NAME);

		XSelectInput(m_display, m_window,
		             SubstructureNotifyMask | ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
		                 KeyPressMask | KeyReleaseMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask |
		                 ButtonMotionMask | KeymapStateMask | FocusChangeMask | PropertyChangeMask);
	}

	// Create image
	if ((m_image = XCreateImage(m_display, DefaultVisual(m_display, 0), 24, ZPixmap, 0, NULL, 10, 10, 32, 0)) ==
	    nullptr)
	{
		// throw 666;
	}

	m_image->width = width;
	m_image->height = height;
	m_image->bytes_per_line = width * 4;

	// Create image buffer [a]
	if ((m_image->data = reinterpret_cast<char*>(calloc(1, static_cast<size_t>(width * height) * sizeof(uint32_t)))) ==
	    nullptr)
	{
		// throw 666;
	}

	// [a]: """ Note that when the image is created using XCreateImage(), XGetImage(), or XSubImage(), the destroy
	// procedure that the XDestroyImage() function calls frees both the image structure and the data pointed to
	// by the image structure """ (https://tronche.com/gui/x/xlib/utilities/XCreateImage.html)

	// Bye!
	XSync(m_display, False); // I need to crash here!, this blocks until X11 server process all instructions
}

void Ui::Deinitialise() noexcept
{
	free(m_image->data);     // [a] we need to free it, the c-library X11 is linked against may be different to ours
	m_image->data = nullptr; // Also [a]

	XDestroyImage(m_image);

	XDestroyWindow(m_display, m_window);
	XCloseDisplay(m_display);
}

void Ui::SetParent(Window parent_window)
{
	// Running "QT_QPA_PLATFORM=xcb qtractor" makes XReparentWindow() succeed,
	// on the other hand "QT_QPA_PLATFORM=wayland qtractor" succeeds then it crashes
	// everything.

	// And it's not Qtractor fault, but Qt, Wayland has a X11 compatibility layer so
	// it's doing fine on pretending being compatible, it's Qt doing the glue wrong.

	// Here, what it seems to be an invalid 'parent_window' is being pass:
	// https://github.com/rncbc/qtractor/blob/04b568651c70cc8e6bd135b79e47d270314c913c/src/qtractorClapPlugin.cpp#L3128

	XReparentWindow(m_display, m_window, parent_window, 0, 0); // This one is failing on success
	XFlush(m_display);
	XSync(m_display, False); // I need XReparentWindow() to crash here!
}

void Ui::OnFd() noexcept
{
	XFlush(m_display);

	while (XPending(m_display))
	{
		XEvent event;
		XNextEvent(m_display, &event);
		XFlush(m_display);
	}
}

void Ui::Show()
{
	XMapRaised(m_display, m_window);
	XFlush(m_display);
}

void Ui::Hide()
{
	XUnmapWindow(m_display, m_window);
	XFlush(m_display);
}
#endif
