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

	auto& main_container = m_yui.GetRoot().SetNewChild<yui::VBox>();

	auto& titlebar = main_container.AddNewChild<yui::HBox>();
	titlebar.SetStretch(true, false);
	titlebar.AddNewChild<yui::ButtonWithLabel>("");
	titlebar.AddNewChild<yui::ButtonWithLabel>("Microsoft Word - Document 1").SetStretch(true, false);
	titlebar.AddNewChild<yui::ButtonWithLabel>("_");
	titlebar.AddNewChild<yui::ButtonWithLabel>("[]");
	titlebar.AddNewChild<yui::ButtonWithLabel>("X");

	auto& menu = main_container.AddNewChild<yui::HBox>();
	menu.AddNewChild<yui::ButtonWithLabel>("File").SetId("#file");
	menu.AddNewChild<yui::ButtonWithLabel>("Edit").SetId("#edit");
	menu.AddNewChild<yui::ButtonWithLabel>("View").SetId("#view");
	menu.AddNewChild<yui::ButtonWithLabel>("Insert").SetId("#insert");
	menu.AddNewChild<yui::ButtonWithLabel>("Format").SetId("#format");
	menu.AddNewChild<yui::ButtonWithLabel>("Tools").SetId("#tools");
	menu.AddNewChild<yui::ButtonWithLabel>("Table").SetId("#table");
	menu.AddNewChild<yui::ButtonWithLabel>("Window").SetId("#window");
	menu.AddNewChild<yui::ButtonWithLabel>("Help").SetId("#help");

	auto& top_toolbar = main_container.AddNewChild<yui::HBox>();
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("0").SetId("#0"); // New
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("1").SetId("#1"); // Open
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("2").SetId("#2"); // Save
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("3").SetId("#3"); // Print
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("4").SetId("#4"); // Search
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("5").SetId("#5"); // Spell
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("6").SetId("#6"); // Cut
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("7").SetId("#7"); // Copy
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("8").SetId("#8"); // Paste
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("9").SetId("#9"); // Format
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("A").SetId("#A"); // Undo
	top_toolbar.AddNewChild<yui::ButtonWithLabel>("B").SetId("#B"); // Redo

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

	auto& status_bar = main_container.AddNewChild<yui::HBox>();
	status_bar.SetStretch(true, false);
	status_bar.AddNewChild<yui::ButtonWithLabel>("Bass Drum");
	status_bar.AddNewChild<yui::ButtonWithLabel>("100%").SetStretch(true, true);
	status_bar.AddNewChild<yui::ButtonWithLabel>("Center").SetStretch(true, true);
	status_bar.AddNewChild<yui::ButtonWithLabel>("606");
	status_bar.AddNewChild<yui::ButtonWithLabel>("Snare");

	// Draw first frame,
	// TODO, ugly design, the backend should do this
	m_yui.Update({width, height}, reinterpret_cast<uint32_t*>(m_buffer));
}
