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

	Ui ui;
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


static void sCreateUi(Ui* ui)
{
	auto vbox = Ui::VBox::Create(ui->GetRoot())->SetStretch(true, true);
	{
		auto titlebar = Ui::HBox::Create(vbox)->SetStretch(true, false);
		{
			Ui::Button::Create(titlebar, "");
			Ui::Button::Create(titlebar, "Microsoft Word - Document 1")->SetStretch(true, false);
			Ui::Button::Create(titlebar, "_");
			Ui::Button::Create(titlebar, "[]");
			Ui::Button::Create(titlebar, "X");
		}

		auto menu = Ui::HBox::Create(vbox);
		{
			Ui::Button::Create(menu, "File");
			Ui::Button::Create(menu, "Edit");
			Ui::Button::Create(menu, "View");
			Ui::Button::Create(menu, "Insert");
			Ui::Button::Create(menu, "Format");
			Ui::Button::Create(menu, "Tools");
			Ui::Button::Create(menu, "Table");
			Ui::Button::Create(menu, "Window");
			Ui::Button::Create(menu, "Help");
		}

		auto top_toolbar = Ui::HBox::Create(vbox);
		{
			Ui::Button::Create(top_toolbar, "0"); // New
			Ui::Button::Create(top_toolbar, "1"); // Open
			Ui::Button::Create(top_toolbar, "2"); // Save
			// Ui::Text::Create(top_toolbar, "|");
			Ui::Button::Create(top_toolbar, "3"); // Print
			Ui::Button::Create(top_toolbar, "4"); // Search
			Ui::Button::Create(top_toolbar, "5"); // Spell
			// Ui::Text::Create(top_toolbar, "|");
			Ui::Button::Create(top_toolbar, "6"); // Cut
			Ui::Button::Create(top_toolbar, "7"); // Copy
			Ui::Button::Create(top_toolbar, "8"); // Paste
			Ui::Button::Create(top_toolbar, "9"); // Format
			// Ui::Text::Create(top_toolbar, "|");
			Ui::Button::Create(top_toolbar, "A"); // Undo
			Ui::Button::Create(top_toolbar, "B"); // Redo
		}

		auto bottom_toolbar = Ui::HBox::Create(vbox);
		{
			Ui::Button::Create(bottom_toolbar, "Normal");          // Style
			Ui::Button::Create(bottom_toolbar, "Times New Roman"); // Font
			Ui::Button::Create(bottom_toolbar, "10");              // Size
			// Ui::Text::Create(bottom_toolbar, "|");
			Ui::Button::Create(bottom_toolbar, "C"); // Bold
			Ui::Button::Create(bottom_toolbar, "D"); // Italic
			Ui::Button::Create(bottom_toolbar, "E"); // Underline
			// Ui::Text::Create(bottom_toolbar, "|");
			Ui::Button::Create(bottom_toolbar, "F"); // Left
			Ui::Button::Create(bottom_toolbar, "?"); // Center
			Ui::Button::Create(bottom_toolbar, "!"); // Right
		}

		Ui::HBox::Create(vbox)->SetStretch(false, true); // Content, it stretches vertically

		auto status_bar = Ui::HBox::Create(vbox)->SetStretch(true, false);
		{
			Ui::Button::Create(status_bar, "Bass Drum");
			Ui::Button::Create(status_bar, "100%")->SetStretch(true, true);
			Ui::Button::Create(status_bar, "Center")->SetStretch(true, true);
			Ui::Button::Create(status_bar, "606");
			Ui::Button::Create(status_bar, "Snare");
		}
	}
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

	app->ui.Initialise();
	sCreateUi(&app->ui);

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
		app->ui.Update(app->mouse_pos, app->window_size, stride, pixels);
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

	app->ui.Deinitialise();

	free(app);
}


#ifndef NDEBUG
extern "C" const char* __lsan_default_suppressions(void)
{
	// It seems to be client-side decorations on Wayland:
	return "leak:libgtk\nleak:libglib\nleak:libpango\nleak:libdecor\n";
}
#endif
