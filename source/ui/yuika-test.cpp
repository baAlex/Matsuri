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

#include <utility>

#include "yuika.hpp"
namespace yui = yuika;


struct App
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;

	bool needs_redraw;

	yui::Screen screen;
	yui::Size window_size;
};


static int sUpdateTexture(App* app)
{
	if (app->texture != nullptr)
		SDL_DestroyTexture(app->texture);

	if ((app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING,
	                                      app->window_size.w, app->window_size.h)) == nullptr)
	{
		printf("SDL_CreateTexture(), %s\n", SDL_GetError());
		return 1;
	}

	return 0;
}


class DisabledHBox : public yui::HBox
{
  public:
	DisabledHBox() : yui::HBox() {}

	yui::EventPropagation OnMouseCapturing(yui::MouseGesture, yui::Position) override
	{
		return yui::EventPropagation::StopIt;
	}
};

class NoisyButton : public yui::ButtonWithLabel
{
  public:
	NoisyButton(const std::string& text) : yui::ButtonWithLabel(text) {}

	yui::EventPropagation OnMouseCapturing(yui::MouseGesture gesture, yui::Position) override
	{
		/*switch (gesture)
		{
		case yui::MouseGesture::Press: printf("Capture, mouse press, \"%s\"\n", m_text.c_str()); break;
		case yui::MouseGesture::Release: printf("Capture, mouse release, \"%s\"\n", m_text.c_str()); break;
		case yui::MouseGesture::Click: printf("Capture, mouse click, \"%s\"\n", m_text.c_str()); break;
		}*/
		return yui::EventPropagation::KeepPassingIt;
	}

	yui::EventPropagation OnMouseBubbling(yui::MouseGesture gesture, yui::Position, yui::Widget& target) override
	{
		if (&target != this)
			return yui::EventPropagation::KeepPassingIt;

		/*switch (gesture)
		{
		case yui::MouseGesture::Press: printf("Bubble, mouse press, \"%s\"\n", m_text.c_str()); break;
		case yui::MouseGesture::Release: printf("Bubble, mouse release, \"%s\"\n", m_text.c_str()); break;
		case yui::MouseGesture::Click: printf("Bubble, mouse click, \"%s\"\n", m_text.c_str()); break;
		}*/
		return yui::EventPropagation::KeepPassingIt;
	}
};

class VoiceRack : public yui::HBox
{
  public:
	VoiceRack(const std::string& name)
	{
		m_name = name;
		SetStretch(true, false);

		AddNewChild<yui::ButtonWithLabel>(name);

		AddNewChild<yui::Button>().SetStretch(true, true);
		AddNewChild<yui::ButtonWithLabel>("Vol -").SetId("#vol-");
		AddNewChild<yui::ButtonWithLabel>("Vol +").SetId("#vol+");

		AddNewChild<yui::Button>().SetStretch(true, true);
		AddNewChild<yui::ButtonWithLabel>("Pan -").SetId("#pan-");
		AddNewChild<yui::ButtonWithLabel>("Pan +").SetId("#pan+");
	}

	yui::EventPropagation OnMouseBubbling(yui::MouseGesture gesture, yui::Position, yui::Widget& target) override
	{
		// Event delegation example, not sure if is right,
		// just now I learned about it:
		// https://developer.mozilla.org/en-US/docs/Learn_web_development/Core/Scripting/Event_bubbling

		if (gesture == yui::MouseGesture::Click)
		{
			if (strcmp(target.GetId(), "#vol-") == 0)
				printf("%s | Click on Vol-\n", m_name.c_str());
			else if (strcmp(target.GetId(), "#vol+") == 0)
				printf("%s | Click on Vol+\n", m_name.c_str());
			else if (strcmp(target.GetId(), "#pan-") == 0)
				printf("%s | Click on Pan-\n", m_name.c_str());
			else if (strcmp(target.GetId(), "#pan+") == 0)
				printf("%s | Click on Pan+\n", m_name.c_str());
		}

		return yui::EventPropagation::KeepPassingIt;
	}

  private:
	std::string m_name;
};

static void sCreateUi(yui::Wrapper& root)
{
	auto& main_container = root.SetNewChild<yui::VBox>();

	auto& titlebar = main_container.AddNewChild<yui::HBox>();
	titlebar.SetStretch(true, false);
	titlebar.AddNewChild<NoisyButton>("");
	titlebar.AddNewChild<yui::ButtonWithLabel>("Microsoft Word - Document 1").SetStretch(true, false);
	titlebar.AddNewChild<yui::ButtonWithLabel>("_");
	titlebar.AddNewChild<yui::ButtonWithLabel>("[]");
	titlebar.AddNewChild<NoisyButton>("X");

	auto& menu = main_container.AddNewChild<yui::HBox>();
	menu.AddNewChild<NoisyButton>("File");
	menu.AddNewChild<NoisyButton>("Edit");
	menu.AddNewChild<NoisyButton>("View");
	menu.AddNewChild<NoisyButton>("Insert");
	menu.AddNewChild<NoisyButton>("Format");
	menu.AddNewChild<NoisyButton>("Tools");
	menu.AddNewChild<NoisyButton>("Table");
	menu.AddNewChild<NoisyButton>("Window");
	menu.AddNewChild<NoisyButton>("Help");

	auto& top_toolbar = main_container.AddNewChild<DisabledHBox>(); // Intercepts events
	// auto& top_toolbar = main_container.AddNewChild<yui::HBox>();
	top_toolbar.AddNewChild<NoisyButton>("0"); // New
	top_toolbar.AddNewChild<NoisyButton>("1"); // Open
	top_toolbar.AddNewChild<NoisyButton>("2"); // Save
	top_toolbar.AddNewChild<NoisyButton>("3"); // Print
	top_toolbar.AddNewChild<NoisyButton>("4"); // Search
	top_toolbar.AddNewChild<NoisyButton>("5"); // Spell
	top_toolbar.AddNewChild<NoisyButton>("6"); // Cut
	top_toolbar.AddNewChild<NoisyButton>("7"); // Copy
	top_toolbar.AddNewChild<NoisyButton>("8"); // Paste
	top_toolbar.AddNewChild<NoisyButton>("9"); // Format
	top_toolbar.AddNewChild<NoisyButton>("A"); // Undo
	top_toolbar.AddNewChild<NoisyButton>("B"); // Redo

	auto& bottom_toolbar = main_container.AddNewChild<yui::HBox>();
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("Normal");          // Style
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("Times New Roman"); // Font
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("10");              // Size
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("C");               // Bold
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("D");               // Italic
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("E");               // Underline
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("F");               // Left
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("?");               // Center
	bottom_toolbar.AddNewChild<yui::ButtonWithLabel>("!");               // Right

	auto& content = main_container.AddNewChild<yui::VBox>();
	content.SetStretch(true, true);

	main_container.AddNewChild<VoiceRack>("Bass drum");
	main_container.AddNewChild<VoiceRack>("Snare");
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

	app->window_size.w = 1280 / 2;
	app->window_size.h = 720 / 2;

	if (SDL_CreateWindowAndRenderer("Test test test", app->window_size.w, app->window_size.h, SDL_WINDOW_RESIZABLE,
	                                &app->window, &app->renderer) == false)
	{
		printf("SDL_CreateWindowAndRenderer(), %s\n", SDL_GetError());
		goto return_failure;
	}

	SDL_SetRenderVSync(app->renderer, 1);

	if (sUpdateTexture(app) != 0)
		goto return_failure;

	app->screen.Initialise(0x00FF0000, 0x0000FF00, 0x000000FF);
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
	{
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		app->screen.MouseEvent(yui::MouseGesture::Press,
		                       {static_cast<int>(event->button.x), static_cast<int>(event->button.y)});
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		app->screen.MouseEvent(yui::MouseGesture::Release,
		                       {static_cast<int>(event->button.x), static_cast<int>(event->button.y)});
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
		app->screen.Update(app->window_size, reinterpret_cast<uint32_t*>(pixels));
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
