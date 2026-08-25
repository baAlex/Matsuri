/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "ui.hpp"


void UiFrontend::Initialise(int width, int height)
{
	UiBackend::Initialise(width, height);

	// Create UI
	namespace yui = yuika;

	auto& main_container = m_yui.GetRoot().SetChild<yui::VBox>();

	auto& titlebar = main_container.AddChild<yui::HBox>();
	titlebar.SetStretch(true, false);
	titlebar.AddChild<yui::Button>("");
	titlebar.AddChild<yui::Button>("Microsoft Word - Document 1").SetStretch(true, false);
	titlebar.AddChild<yui::Button>("_");
	titlebar.AddChild<yui::Button>("[]");
	titlebar.AddChild<yui::Button>("X");

	auto& menu = main_container.AddChild<yui::HBox>();
	menu.AddChild<yui::Button>("File");
	menu.AddChild<yui::Button>("Edit");
	menu.AddChild<yui::Button>("View");
	menu.AddChild<yui::Button>("Insert");
	menu.AddChild<yui::Button>("Format");
	menu.AddChild<yui::Button>("Tools");
	menu.AddChild<yui::Button>("Table");
	menu.AddChild<yui::Button>("Window");
	menu.AddChild<yui::Button>("Help");

	auto& top_toolbar = main_container.AddChild<yui::HBox>();
	top_toolbar.AddChild<yui::Button>("0"); // New
	top_toolbar.AddChild<yui::Button>("1"); // Open
	top_toolbar.AddChild<yui::Button>("2"); // Save
	top_toolbar.AddChild<yui::Button>("3"); // Print
	top_toolbar.AddChild<yui::Button>("4"); // Search
	top_toolbar.AddChild<yui::Button>("5"); // Spell
	top_toolbar.AddChild<yui::Button>("6"); // Cut
	top_toolbar.AddChild<yui::Button>("7"); // Copy
	top_toolbar.AddChild<yui::Button>("8"); // Paste
	top_toolbar.AddChild<yui::Button>("9"); // Format
	top_toolbar.AddChild<yui::Button>("A"); // Undo
	top_toolbar.AddChild<yui::Button>("B"); // Redo

	auto& bottom_toolbar = main_container.AddChild<yui::HBox>();
	bottom_toolbar.AddChild<yui::Button>("Normal");          // Style
	bottom_toolbar.AddChild<yui::Button>("Times New Roman"); // Font
	bottom_toolbar.AddChild<yui::Button>("10");              // Size
	bottom_toolbar.AddChild<yui::Button>("C");               // Bold
	bottom_toolbar.AddChild<yui::Button>("D");               // Italic
	bottom_toolbar.AddChild<yui::Button>("E");               // Underline
	bottom_toolbar.AddChild<yui::Button>("F");               // Left
	bottom_toolbar.AddChild<yui::Button>("?");               // Center
	bottom_toolbar.AddChild<yui::Button>("!");               // Right

	auto& content = main_container.AddChild<yui::VBox>();
	content.SetStretch(true, true);

	auto& status_bar = main_container.AddChild<yui::HBox>();
	status_bar.SetStretch(true, false);
	status_bar.AddChild<yui::Button>("Bass Drum");
	status_bar.AddChild<yui::Button>("100%").SetStretch(true, true);
	status_bar.AddChild<yui::Button>("Center").SetStretch(true, true);
	status_bar.AddChild<yui::Button>("606");
	status_bar.AddChild<yui::Button>("Snare");

	// Draw first frame,
	// TODO, ugly design, the backend should do this
	m_yui.Update({width, height}, reinterpret_cast<uint32_t*>(m_buffer));
}
