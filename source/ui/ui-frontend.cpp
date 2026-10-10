/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "ui.hpp"


namespace yui = yuika;


class KnobValuePair : public yui::HBox
{
  public:
	KnobValuePair(const char* label, const char* measure)
	{
		AddNewChild<yui::ButtonWithText>("[]").SetStretch(true, true);

		auto& right_side = AddNewChild<yui::VBox>();
		right_side.AddNewChild<yui::Text>(measure);
		right_side.AddNewChild<yui::Text>(label);
	}
};

class VoiceRack : public yui::HBox
{
  public:
	VoiceRack()
	{
		auto& left_table = AddNewChild<yui::VBox>();

		// TODO, I need a table widget

		auto& top_row = left_table.AddNewChild<yui::HBox>();
		top_row.AddNewChild<KnobValuePair>("VOL", "100%");
		top_row.AddNewChild<KnobValuePair>("TUN", "0");

		auto& bottom_row = left_table.AddNewChild<yui::HBox>();
		bottom_row.AddNewChild<KnobValuePair>("PAN", "CEN");
		bottom_row.AddNewChild<KnobValuePair>("LEN", "100%");

		auto& right_buttons = AddNewChild<yui::VBox>();
		right_buttons.SetStretch(false, true);

		right_buttons.AddNewChild<yui::Spacer>();
		right_buttons.AddNewChild<yui::ButtonWithText>("Lock");
		right_buttons.AddNewChild<yui::Spacer>();
	}
};

class VoiceFrame : public yui::Frame
{
  public:
	const char* m_label;
	yui::Vector m_extra_height;

	VoiceFrame(const char* label) : m_label(label)
	{
		m_extra_height = {};
	}

	yui::Vector UpdateNaturalSize(yui::LayoutApi& api) override
	{
		if (m_natural_size_updated == false)
		{
			m_extra_height.y = api.TextSize("x").y; // TODO, make a new api out of this

			m_natural_size_updated = true;
			m_natural_size = api.Margin(*this) + m_extra_height;
			m_natural_size += m_content->UpdateNaturalSize(api); // [Recursion]
		}

		return m_natural_size;
	}

	yui::Widget::ChildGet GetChild(yui::LayoutApi&, size_t, yui::Vector available_size) const override
	{
		return {*m_content, m_extra_height, GetSize(available_size), {}};
	}

	void OnDraw(yui::DrawApi& api, yui::Rect allowed_draw_area) const override
	{
		api.SetHitArea(allowed_draw_area);
		api.DrawText(yui::DrawApi::BLACK, {allowed_draw_area.pos}, m_label);
		api.DrawBorder({allowed_draw_area.pos + m_extra_height, allowed_draw_area.size - m_extra_height},
		               yui::DrawApi::BorderStyle::Outset);
	}
};

class Toolbar : public yui::HBox
{
  public:
	Toolbar()
	{
		SetStretch(true, false);

		auto& left_pane = AddNewChild<yui::HBox>();
		left_pane.SetStretch(true, true);

		AddNewChild<yui::ButtonWithText>("1"); // Center pane
		AddNewChild<yui::ButtonWithText>("2");
		AddNewChild<yui::ButtonWithText>("3");
		AddNewChild<yui::ButtonWithText>("4");

		auto& right_pane = AddNewChild<yui::HBox>();
		right_pane.SetStretch(true, true);

		right_pane.AddNewChild<yui::Spacer>();
		right_pane.AddNewChild<yui::ButtonWithText>("Matsuri", yui::Button::Style::Flat);
	}
};

void UiFrontend::CreateUi(yui::Frame& root)
{
	auto& main_container = root.SetNewChild<yui::VBox>();
	main_container.SetStretch(true, true);

	main_container.AddNewChild<Toolbar>();

	auto& voices = main_container.AddNewChild<yui::HBox>();
	voices.SetStretch(true, true);

	voices.AddNewChild<VoiceFrame>("Bass Drum").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Snare Drum").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Closed Hat").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Open Hat").SetNewChild<VoiceRack>();
}


void UiFrontend::Initialise(int width, int height)
{
	UiBackend::Initialise(width, height);

	CreateUi(m_yui.GetRoot());

	// Draw first frame,
	// TODO, ugly design, the backend should do this
	m_yui.Draw({width, height}, reinterpret_cast<uint32_t*>(m_buffer));
}
