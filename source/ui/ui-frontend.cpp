/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "ui.hpp"


class MainContainer : public yuika::VBox
{
  public:
	MainContainer() {}

	yuika::EventPropagation OnMouse(yuika::MouseGesture gesture, yuika::Position, const yuika::Widget& target) override
	{
		if (gesture == yuika::MouseGesture::Click)
			printf("Click on target \"%s\"\n", target.GetId());
		else if (gesture == yuika::MouseGesture::Press)
			printf("Press on target \"%s\"\n", target.GetId());
		else if (gesture == yuika::MouseGesture::Release)
			printf("Release on target \"%s\"\n", target.GetId());
		else if (gesture == yuika::MouseGesture::Enters)
			printf("Enters on target \"%s\"\n", target.GetId());
		else if (gesture == yuika::MouseGesture::Leaves)
			printf("Leaves on target \"%s\"\n", target.GetId());

		return yuika::EventPropagation::KeepPassingIt;
	}
};


static void sCreateUi(yuika::Wrapper& root)
{
	namespace yui = yuika;

	auto& main_container = root.SetNewChild<MainContainer>();
	main_container.SetStretch(true, true);
	main_container.SetId("#main_container");

	auto& titlebar = main_container.AddNewChild<yui::HBox>();
	titlebar.SetStretch(true, false);
	titlebar.AddNewChild<yui::ButtonWithText>("").SetId("#");
	titlebar.AddNewChild<yui::ButtonWithText>("Microsoft Word - Document 1").SetStretch(true, false).SetId("#msword");
	titlebar.AddNewChild<yui::ButtonWithText>("_").SetId("#_");
	titlebar.AddNewChild<yui::ButtonWithText>("[]").SetId("#[]");
	titlebar.AddNewChild<yui::ButtonWithText>("X").SetId("#X");
	titlebar.SetId("#titlebar");

	auto& menu = main_container.AddNewChild<yui::HBox>();
	menu.AddNewChild<yui::ButtonWithText>("File").SetId("#file");
	menu.AddNewChild<yui::ButtonWithText>("Edit").SetId("#edit");
	menu.AddNewChild<yui::ButtonWithText>("View").SetId("#view");
	menu.AddNewChild<yui::ButtonWithText>("Insert").SetId("#insert");
	menu.AddNewChild<yui::ButtonWithText>("Format").SetId("#format");
	menu.AddNewChild<yui::ButtonWithText>("Tools").SetId("#tools");
	menu.AddNewChild<yui::ButtonWithText>("Table").SetId("#table");
	menu.AddNewChild<yui::ButtonWithText>("Window").SetId("#window");
	menu.AddNewChild<yui::ButtonWithText>("Help").SetId("#help");
	menu.SetId("#menu");

	auto& top_toolbar = main_container.AddNewChild<yui::HBox>();
	top_toolbar.AddNewChild<yui::ButtonWithText>("0").SetId("#0"); // New
	top_toolbar.AddNewChild<yui::ButtonWithText>("1").SetId("#1"); // Open
	top_toolbar.AddNewChild<yui::ButtonWithText>("2").SetId("#2"); // Save
	top_toolbar.AddNewChild<yui::ButtonWithText>("3").SetId("#3"); // Print
	top_toolbar.AddNewChild<yui::ButtonWithText>("4").SetId("#4"); // Search
	top_toolbar.AddNewChild<yui::ButtonWithText>("5").SetId("#5"); // Spell
	top_toolbar.AddNewChild<yui::ButtonWithText>("6").SetId("#6"); // Cut
	top_toolbar.AddNewChild<yui::ButtonWithText>("7").SetId("#7"); // Copy
	top_toolbar.AddNewChild<yui::ButtonWithText>("8").SetId("#8"); // Paste
	top_toolbar.AddNewChild<yui::ButtonWithText>("9").SetId("#9"); // Format
	top_toolbar.AddNewChild<yui::ButtonWithText>("A").SetId("#A"); // Undo
	top_toolbar.AddNewChild<yui::ButtonWithText>("B").SetId("#B"); // Redo
	top_toolbar.SetId("#top_toolbar");

	auto& bottom_toolbar = main_container.AddNewChild<yui::HBox>();
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("Normal").SetId("#normal");         // Style
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("Times New Roman").SetId("#times"); // Font
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("10").SetId("#10");                 // Size
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("C").SetId("#c");                   // Bold
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("D").SetId("#d");                   // Italic
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("E").SetId("#e");                   // Underline
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("F").SetId("#f");                   // Left
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("?").SetId("#?");                   // Center
	bottom_toolbar.AddNewChild<yui::ButtonWithText>("!").SetId("#!");                   // Right
	bottom_toolbar.SetId("#bottom_toolbar");

	auto& content = main_container.AddNewChild<yui::VBox>();
	content.SetStretch(true, true);
	content.SetId("#content");

	auto& status_bar = main_container.AddNewChild<yui::HBox>();
	status_bar.SetStretch(true, false);
	status_bar.AddNewChild<yui::ButtonWithText>("Bass Drum").SetId("#bd");
	status_bar.AddNewChild<yui::ButtonWithText>("100%").SetStretch(true, true).SetId("#100%");
	status_bar.AddNewChild<yui::ButtonWithText>("Center").SetStretch(true, true).SetId("#center");
	status_bar.AddNewChild<yui::ButtonWithText>("606").SetId("#606");
	status_bar.AddNewChild<yui::ButtonWithText>("Snare").SetId("#snare");
	status_bar.SetId("#status_bar");
}


void UiFrontend::Initialise(int width, int height)
{
	UiBackend::Initialise(width, height);

	sCreateUi(m_yui.GetRoot());

	// Draw first frame,
	// TODO, ugly design, the backend should do this
	m_yui.Draw({width, height}, reinterpret_cast<uint32_t*>(m_buffer));
}
