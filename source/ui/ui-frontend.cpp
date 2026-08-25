/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "ui.hpp"


void Ui::Initialise(int width, int height)
{
	UiBackend::Initialise(width, height);

	// Create UI
	using namespace yuika;

	auto& main_container = m_yui.GetRoot().SetChild<VBox>();

	auto& titlebar = main_container.AddChild<HBox>();
	titlebar.SetStretch(true, false);
	titlebar.AddChild<Button>("");
	titlebar.AddChild<Button>("Microsoft Word - Document 1").SetStretch(true, false);
	titlebar.AddChild<Button>("_");
	titlebar.AddChild<Button>("[]");
	titlebar.AddChild<Button>("X");

	auto& menu = main_container.AddChild<HBox>();
	menu.AddChild<Button>("File");
	menu.AddChild<Button>("Edit");
	menu.AddChild<Button>("View");
	menu.AddChild<Button>("Insert");
	menu.AddChild<Button>("Format");
	menu.AddChild<Button>("Tools");
	menu.AddChild<Button>("Table");
	menu.AddChild<Button>("Window");
	menu.AddChild<Button>("Help");

	auto& top_toolbar = main_container.AddChild<HBox>();
	top_toolbar.AddChild<Button>("0"); // New
	top_toolbar.AddChild<Button>("1"); // Open
	top_toolbar.AddChild<Button>("2"); // Save
	top_toolbar.AddChild<Button>("3"); // Print
	top_toolbar.AddChild<Button>("4"); // Search
	top_toolbar.AddChild<Button>("5"); // Spell
	top_toolbar.AddChild<Button>("6"); // Cut
	top_toolbar.AddChild<Button>("7"); // Copy
	top_toolbar.AddChild<Button>("8"); // Paste
	top_toolbar.AddChild<Button>("9"); // Format
	top_toolbar.AddChild<Button>("A"); // Undo
	top_toolbar.AddChild<Button>("B"); // Redo

	auto& bottom_toolbar = main_container.AddChild<HBox>();
	bottom_toolbar.AddChild<Button>("Normal");          // Style
	bottom_toolbar.AddChild<Button>("Times New Roman"); // Font
	bottom_toolbar.AddChild<Button>("10");              // Size
	bottom_toolbar.AddChild<Button>("C");               // Bold
	bottom_toolbar.AddChild<Button>("D");               // Italic
	bottom_toolbar.AddChild<Button>("E");               // Underline
	bottom_toolbar.AddChild<Button>("F");               // Left
	bottom_toolbar.AddChild<Button>("?");               // Center
	bottom_toolbar.AddChild<Button>("!");               // Right

	auto& content = main_container.AddChild<VBox>();
	content.SetStretch(true, true);

	auto& status_bar = main_container.AddChild<HBox>();
	status_bar.SetStretch(true, false);
	status_bar.AddChild<Button>("Bass Drum");
	status_bar.AddChild<Button>("100%").SetStretch(true, true);
	status_bar.AddChild<Button>("Center").SetStretch(true, true);
	status_bar.AddChild<Button>("606");
	status_bar.AddChild<Button>("Snare");

	// Draw first frame,
	// TODO, ugly design, the backend should do this
	m_yui.Update({width, height}, reinterpret_cast<uint32_t*>(m_buffer));
}
