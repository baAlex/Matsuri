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


class VoiceRack : public yui::HBox
{
  public:
	VoiceRack(const std::string& name)
	{
		m_name = name;
		SetStretch(true, false);

		AddNewChild<yui::ButtonWithText>(name);

		AddNewChild<yui::Button>().SetStretch(true, true);
		AddNewChild<yui::ButtonWithText>("Vol -").SetId("#vol-");
		AddNewChild<yui::ButtonWithText>("Vol +").SetId("#vol+");

		AddNewChild<yui::Button>().SetStretch(true, true);
		AddNewChild<yui::ButtonWithText>("Pan -").SetId("#pan-");
		AddNewChild<yui::ButtonWithText>("Pan +").SetId("#pan+");
	}

	yui::EventPropagation OnMouse(yui::MouseGesture gesture, yui::Position, const yui::Widget& target) override
	{
		// Event delegation example, not sure if is right,
		// just now I learned about it:
		// https://developer.mozilla.org/en-US/docs/Learn_web_development/Core/Scripting/Event_bubbling

		if (gesture == yui::MouseGesture::Click)
			printf("%s | Click on target \"%s\"\n", m_name.c_str(), target.GetId());
		else if (gesture == yui::MouseGesture::Press)
			printf("%s | Press on target \"%s\"\n", m_name.c_str(), target.GetId());
		else if (gesture == yui::MouseGesture::Release)
			printf("%s | Release on target \"%s\"\n", m_name.c_str(), target.GetId());
		else if (gesture == yui::MouseGesture::Enters)
			printf("%s | Enters on target \"%s\"\n", m_name.c_str(), target.GetId());
		else if (gesture == yui::MouseGesture::Leaves)
			printf("%s | Leaves on target \"%s\"\n", m_name.c_str(), target.GetId());

		return yui::EventPropagation::KeepPassingIt;
	}

  private:
	std::string m_name;
};

class ClickableText : public yui::Text
{
  public:
	ClickableText(const std::string& text) : yui::Text(text) {}

	yui::EventPropagation OnMouse(yui::MouseGesture gesture, yui::Position, const yui::Widget& target) override
	{
		if (gesture == yui::MouseGesture::Click)
			printf("Click on target \"%s\"\n", target.GetId());
		else if (gesture == yui::MouseGesture::Press)
			printf("Press on target \"%s\"\n", target.GetId());
		else if (gesture == yui::MouseGesture::Release)
			printf("Release on target \"%s\"\n", target.GetId());

		return yui::EventPropagation::KeepPassingIt;
	}
};

class MenuButton : public yui::ButtonWithText
{
  public:
	MenuButton(const std::string& text) : yui::ButtonWithText(text) {};

	yui::EventPropagation OnMouse(yui::MouseGesture gesture, yui::Position, const yui::Widget& target) override
	{
		if (gesture == yui::MouseGesture::Enters || gesture == yui::MouseGesture::Leaves)
			SetDirty(true);
		return yui::EventPropagation::KeepPassingIt;
	}
};

static void sCreateUi(yui::Wrapper& root)
{
	auto& main_container = root.SetNewChild<yui::VBox>();
	main_container.SetStretch(true, true);

#if 1
	auto& titlebar = main_container.AddNewChild<yui::HBox>();
	titlebar.SetStretch(true, false);
	titlebar.AddNewChild<yui::ButtonWithText>("|");
	titlebar.AddNewChild<yui::ButtonWithText>("Microsoft (a) [b] Word!? - {Document 1}").SetStretch(true, false);
	titlebar.AddNewChild<yui::ButtonWithText>("_");
	titlebar.AddNewChild<yui::ButtonWithText>("[]");
	titlebar.AddNewChild<yui::ButtonWithText>("X");

	auto& menu = main_container.AddNewChild<yui::HBox>();
	menu.AddNewChild<MenuButton>("File");
	menu.AddNewChild<MenuButton>("Edit");
	menu.AddNewChild<MenuButton>("View");
	menu.AddNewChild<MenuButton>("Insert");
	menu.AddNewChild<MenuButton>("Format");
	menu.AddNewChild<MenuButton>("Tools");
	menu.AddNewChild<MenuButton>("Table");
	menu.AddNewChild<MenuButton>("Window");
	menu.AddNewChild<MenuButton>("Help");

	auto& top_toolbar = main_container.AddNewChild<yui::HBox>();
	top_toolbar.AddNewChild<yui::ButtonWithText>("0"); // New
	top_toolbar.AddNewChild<yui::ButtonWithText>("1"); // Open
	top_toolbar.AddNewChild<yui::ButtonWithText>("2"); // Save
	top_toolbar.AddNewChild<yui::ButtonWithText>("3"); // Print
	top_toolbar.AddNewChild<yui::ButtonWithText>("4"); // Search
	top_toolbar.AddNewChild<yui::ButtonWithText>("5"); // Spell
	top_toolbar.AddNewChild<yui::ButtonWithText>("6"); // Cut
	top_toolbar.AddNewChild<yui::ButtonWithText>("7"); // Copy
	top_toolbar.AddNewChild<yui::ButtonWithText>("8"); // Paste
	top_toolbar.AddNewChild<yui::ButtonWithText>("9"); // Format
	top_toolbar.AddNewChild<yui::ButtonWithText>("A"); // Undo
	top_toolbar.AddNewChild<yui::ButtonWithText>("B"); // Redo

	auto& bottom_toolbar = main_container.AddNewChild<yui::HBox>();
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("Normal");          // Style
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("Times New Roman"); // Font
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("10");              // Size
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("C");               // Bold
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("D");               // Italic
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("E");               // Underline
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("F");               // Left
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("?");               // Center
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("!");               // Right

	auto& content = main_container.AddNewChild<yui::VBox>();
	content.SetStretch(true, true);
#endif

	main_container.AddNewChild<VoiceRack>("Bass drum").SetId("#bd_rack");
	main_container.AddNewChild<VoiceRack>("Snare").SetId("#sd_rack");

	main_container.AddNewChild<ClickableText>("Test Text").SetId("#test_text");
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
		app->screen.MousePress({static_cast<int>(event->button.x), static_cast<int>(event->button.y)});
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		app->screen.MouseRelease({static_cast<int>(event->button.x), static_cast<int>(event->button.y)});
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		app->screen.MouseMoves({static_cast<int>(event->button.x), static_cast<int>(event->button.y)});
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
		app->screen.Draw(app->window_size, reinterpret_cast<uint32_t*>(pixels));
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
