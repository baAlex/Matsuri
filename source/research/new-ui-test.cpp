/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <stdlib.h>

#include "new-ui.hpp"


struct App
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;

	bool needs_redraw;

	Ui::Screen screen;
	Ui::Position mouse_pos;
	Ui::Size window_size;
};


static int sUpdateTexture(App* app)
{
	if (app->texture != nullptr)
		SDL_DestroyTexture(app->texture);

	if ((app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
	                                      app->window_size.w, app->window_size.h)) == nullptr)
	{
		printf("SDL_CreateTexture(), %s\n", SDL_GetError());
		return 1;
	}

	return 0;
}


static void sCreateUi(Ui::Wrapper& root)
{
	using namespace Ui;

	auto titlebar = new HBox();
	titlebar->SetStretch(true, false);
	titlebar->AddChild(new Button(""));
	titlebar->AddChild(new Button("Microsoft Word - Document 1")).SetStretch(true, false);
	titlebar->AddChild(new Button("_"));
	titlebar->AddChild(new Button("[]"));
	titlebar->AddChild(new Button("X"));

	auto menu = new HBox();
	menu->AddChild(new Button("File"));
	menu->AddChild(new Button("Edit"));
	menu->AddChild(new Button("View"));
	menu->AddChild(new Button("Insert"));
	menu->AddChild(new Button("Format"));
	menu->AddChild(new Button("Tools"));
	menu->AddChild(new Button("Table"));
	menu->AddChild(new Button("Window"));
	menu->AddChild(new Button("Help"));

	auto top_toolbar = new HBox();
	top_toolbar->AddChild(new Button("0")); // New
	top_toolbar->AddChild(new Button("1")); // Open
	top_toolbar->AddChild(new Button("2")); // Save
	top_toolbar->AddChild(new Button("3")); // Print
	top_toolbar->AddChild(new Button("4")); // Search
	top_toolbar->AddChild(new Button("5")); // Spell
	top_toolbar->AddChild(new Button("6")); // Cut
	top_toolbar->AddChild(new Button("7")); // Copy
	top_toolbar->AddChild(new Button("8")); // Paste
	top_toolbar->AddChild(new Button("9")); // Format
	top_toolbar->AddChild(new Button("A")); // Undo
	top_toolbar->AddChild(new Button("B")); // Redo

	auto bottom_toolbar = new HBox();
	bottom_toolbar->AddChild(new Button("Normal"));          // Style
	bottom_toolbar->AddChild(new Button("Times New Roman")); // Font
	bottom_toolbar->AddChild(new Button("10"));              // Size
	bottom_toolbar->AddChild(new Button("C"));               // Bold
	bottom_toolbar->AddChild(new Button("D"));               // Italic
	bottom_toolbar->AddChild(new Button("E"));               // Underline
	bottom_toolbar->AddChild(new Button("F"));               // Left
	bottom_toolbar->AddChild(new Button("?"));               // Center
	bottom_toolbar->AddChild(new Button("!"));               // Right

	auto content = new VBox();
	content->SetStretch(true, true);

	auto status_bar = new HBox();
	status_bar->SetStretch(true, false);
	status_bar->AddChild(new Button("Bass Drum"));
	status_bar->AddChild(new Button("100%")).SetStretch(true, true);
	status_bar->AddChild(new Button("Center")).SetStretch(true, true);
	status_bar->AddChild(new Button("606"));
	status_bar->AddChild(new Button("Snare"));

	// ----

	auto vbox = new VBox();
	vbox->SetStretch(true, true);
	vbox->AddChild(titlebar);
	vbox->AddChild(menu);
	vbox->AddChild(top_toolbar);
	vbox->AddChild(bottom_toolbar);
	vbox->AddChild(content);
	vbox->AddChild(status_bar);

	root.SetChild(vbox);
}


SDL_AppResult SDL_AppInit(void** app_raw, int, char**)
{
	App* app;

	if ((*app_raw = malloc(sizeof(App))) == nullptr)
	{
		printf("No enough memory\n");
		goto return_failure;
	}

	app = reinterpret_cast<App*>(*app_raw);
	memset(app, 0, sizeof(App));

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != true)
	{
		printf("SDL_InitSubSystem(), %s\n", SDL_GetError());
		goto return_failure;
	}

	app->window_size.w = 640;
	app->window_size.h = 480;

	if (SDL_CreateWindowAndRenderer("Test test test", app->window_size.w, app->window_size.h, SDL_WINDOW_RESIZABLE,
	                                &app->window, &app->renderer) == false)
	{
		printf("SDL_CreateWindowAndRenderer(), %s\n", SDL_GetError());
		goto return_failure;
	}

	if (sUpdateTexture(app) != 0)
		goto return_failure;

	app->screen.Initialise();
	sCreateUi(app->screen.GetRoot());

	// Bye!
	return SDL_APP_CONTINUE;
return_failure:
	return SDL_APP_FAILURE;
}


SDL_AppResult SDL_AppEvent(void* app_raw, SDL_Event* event)
{
	App* app = reinterpret_cast<App*>(app_raw);

	if (event->type == SDL_EVENT_QUIT)
		return SDL_APP_SUCCESS;
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		app->mouse_pos.x = static_cast<int>(event->motion.x);
		app->mouse_pos.y = static_cast<int>(event->motion.y);
	}
	else if (event->type == SDL_EVENT_WINDOW_RESIZED)
	{
		app->window_size.w = event->window.data1;
		app->window_size.h = event->window.data2;
		sUpdateTexture(app);
	}

	return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void* app_raw)
{
	App* app = reinterpret_cast<App*>(app_raw);

	void* pixels = nullptr;
	int stride = 0;
	if (SDL_LockTexture(app->texture, nullptr, &pixels, &stride) == true)
	{
		app->screen.Update(app->mouse_pos, app->window_size, stride, pixels);
		SDL_UnlockTexture(app->texture);
	}
	else
	{
		printf("SDL_LockTexture(), %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_RenderClear(app->renderer);
	SDL_RenderTexture(app->renderer, app->texture, nullptr, nullptr);
	SDL_RenderPresent(app->renderer);

	return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void* app_raw, SDL_AppResult)
{
	if (app_raw == nullptr)
		return;

	App* app = reinterpret_cast<App*>(app_raw);

	if (app->texture)
		SDL_DestroyTexture(app->texture);
	if (app->renderer)
		SDL_DestroyRenderer(app->renderer);
	if (app->window)
		SDL_DestroyWindow(app->window);

	app->screen.Deinitialise();

	free(app);
}


#ifndef NDEBUG
extern "C" const char* __lsan_default_suppressions(void)
{
	// It seems to be client-side decorations on Wayland:
	return "leak:libgtk\nleak:libglib\nleak:libpango\nleak:libdecor\n";
}
#endif
