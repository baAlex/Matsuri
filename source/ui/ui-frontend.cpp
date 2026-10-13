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
		right_buttons.AddNewChild<yui::ButtonWithText>("L"); // Lock
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
		// SetStretch(true, false); // For debuging how features interact each other
		m_extra_height = {};
	}

	void OnLayout(yui::LayoutApi& api) override
	{
		m_extra_height.y = api.TextSize("x").y; // TODO, make a new api out of this

		m_natural_size = api.Margin(*this) + m_extra_height;
		m_natural_size += m_content->GetNaturalSize();
	}

	yui::Widget::ChildInLayout GetChildInLayout(yui::LayoutApi&, size_t, yui::Vector size, yui::Vector) const override
	{
		return {*m_content, m_extra_height, m_content->GetSize(size), {}};
	}

	void OnDraw(yui::DrawApi& api, yui::Rect allowed_draw_area) const override
	{
		api.SetHitArea(allowed_draw_area);
		api.DrawText(yui::DrawApi::BLACK, allowed_draw_area.pos, m_label);
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
		AddNewChild<yui::Spacer>();
		AddNewChild<yui::ButtonWithText>("Matsuri", yui::Button::Style::Flat);
	}
};

void UiFrontend::CreateUi(yui::Frame& root)
{
	auto& main_container = root.SetNewChild<yui::VBox>();
	main_container.SetStretch(true, true);

#if 1
	main_container.AddNewChild<Toolbar>();

	auto& voices = main_container.AddNewChild<yui::HBox>();
	voices.SetStretch(true, true);
	voices.SetWrap(true);

	voices.AddNewChild<VoiceFrame>("Bass Drum").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Snare Drum").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Hat Closed").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Hat Open").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Cymbal").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Tom High").SetNewChild<VoiceRack>();
	voices.AddNewChild<VoiceFrame>("Tom Low").SetNewChild<VoiceRack>();

	main_container.AddNewChild<yui::ButtonWithText>("<Status bar>").SetStretch(true, false);
#else
	main_container.AddNewChild<yui::Text>("Simple test");
	main_container.AddNewChild<yui::ButtonWithText>("Hello 1");
	main_container.AddNewChild<yui::ButtonWithText>("Hello 2");
#endif
}


void UiFrontend::Initialise(int width, int height)
{
	UiBackend::Initialise(width, height);

	CreateUi(m_yui.GetRoot());

	// Draw first frame,
	// TODO, ugly design, the backend should do this
	m_yui.Draw({width, height}, reinterpret_cast<uint32_t*>(m_buffer));
}
