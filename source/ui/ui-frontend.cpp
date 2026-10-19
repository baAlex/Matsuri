/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "ui.hpp"
#include <assert.h>


namespace yui = yuika;


// Cookies are easier to check than ids (strings are nasty)

static constexpr uint32_t COOKIE_VOL = 0b000'001;
static constexpr uint32_t COOKIE_PAN = 0b000'010;
static constexpr uint32_t COOKIE_TRA = 0b000'011;
static constexpr uint32_t COOKIE_TUN = 0b000'100;
static constexpr uint32_t COOKIE_LEN = 0b000'101;
static constexpr uint32_t COOKIE_SOLO = 0b000'110;
static constexpr uint32_t COOKIE_MUTE = 0b000'111;

static constexpr uint32_t COOKIE_BD = 0b001'000;
static constexpr uint32_t COOKIE_SD = 0b010'000;
static constexpr uint32_t COOKIE_HC = 0b011'000;
static constexpr uint32_t COOKIE_HO = 0b100'000;
static constexpr uint32_t COOKIE_CY = 0b101'000;
static constexpr uint32_t COOKIE_TH = 0b110'000;
static constexpr uint32_t COOKIE_TL = 0b111'000;


class KnobValuePair : public yui::HBox
{
  public:
	KnobValuePair(const std::string& label, const std::string& measure)
	{
		AddNewChild<yui::ButtonWithText>("[ ]").SetStretch(true, true);

		auto& right_side = AddNewChild<yui::VBox>();
		right_side.AddNewChild<yui::Text>(measure);
		right_side.AddNewChild<yui::Text>(label);
	}

	yui::EventPropagation OnMouse(yui::MouseGesture, yui::Vector, uint32_t& cookie, const yui::Widget& target) override
	{
		if (target.GetClassId() == "ButtonWithText")
			cookie = GetCookie(); // Replace with our cookie
		return yui::EventPropagation::KeepPassingIt;
	}
};

static constexpr uint32_t EXTRA_NONE = 1;
static constexpr uint32_t EXTRA_TRANSIENT = 2;

class VoiceRack : public yui::HBox
{
  public:
	VoiceRack(uint32_t extra = EXTRA_NONE)
	{
		auto& left_table = AddNewChild<yui::VBox>();

		// TODO, I need a table widget

		auto& top_row = left_table.AddNewChild<yui::HBox>();
		top_row.AddNewChild<KnobValuePair>("VOL", "100%").SetCookie(COOKIE_VOL);
		top_row.AddNewChild<KnobValuePair>("PAN", "CEN").SetCookie(COOKIE_PAN);
		if ((extra & EXTRA_TRANSIENT) != 0)
			top_row.AddNewChild<KnobValuePair>("TRA", "100%").SetCookie(COOKIE_TRA); // Should go beyond 100%

		auto& bottom_row = left_table.AddNewChild<yui::HBox>();
		bottom_row.AddNewChild<KnobValuePair>("TUN", "0").SetCookie(COOKIE_TUN);
		bottom_row.AddNewChild<KnobValuePair>("LEN", "100%").SetCookie(COOKIE_LEN); // Beyond as well

		auto& right_buttons = AddNewChild<yui::VBox>();

		right_buttons.AddNewChild<yui::ButtonWithText>("S").SetStretch(true, false).SetCookie(COOKIE_SOLO);
		right_buttons.AddNewChild<yui::ButtonWithText>("M").SetStretch(true, false).SetCookie(COOKIE_MUTE);
		right_buttons.AddNewChild<yui::Spacer>();
	}

	yui::EventPropagation OnMouse(yui::MouseGesture, yui::Vector, uint32_t& cookie, const yui::Widget&) override
	{
		cookie |= GetCookie(); // Append our cookie
		return yui::EventPropagation::KeepPassingIt;
	}
};

class VoiceFrame : public yui::Frame // Just decoration
{
  public:
	static constexpr int PADDING = 16; // Is not a good idea to bloat yuika with proper padding right now,
	                                   // let's keep it on the widget side of things
	const char* m_label;
	yui::Vector m_extra_height;

	VoiceFrame(const char* label) : m_label(label)
	{
		SetStretch(true, false); // Stretching and wrapping, is a cool programming achievement (it
		                         // behaves like a fancy CSS flexbox), but looks horrible at the eye
		                         // with all the widgets moving when resizing, and how irregular the
		                         // spacing between them is comparing row and row (is like justified
		                         // text in Word)(which I don't like)

		SetClassId("VoiceFrame");
		m_extra_height = {};
	}

	void OnLayout(yui::LayoutApi& api) override
	{
		m_extra_height.y = api.TextSize("").y; // TODO?, the day I implement clipping the ascenders will be decimated

		m_minimum_size = api.Margin(*this) + m_extra_height;
		if (GetChildrenNo() != 0)
			m_minimum_size += m_content->GetMinimumSize() + PADDING;
	}

	yui::Widget::ChildInLayout GetChildInLayout(yui::LayoutApi&, size_t, yui::Vector size, yui::Vector) const override
	{
		return {*m_content, m_extra_height + PADDING / 2, m_content->GetSize(size - PADDING), {}};
	}

	void OnDraw(yui::DrawApi& api, yui::Rect allowed_draw_area) override
	{
		api.SetHitArea(allowed_draw_area);
		api.DrawBorder({allowed_draw_area.pos + m_extra_height, allowed_draw_area.size - m_extra_height},
		               yui::DrawApi::BorderStyle::Frame);

		api.DrawText(yui::DrawApi::BLACK, {allowed_draw_area.pos.x + PADDING / 2, allowed_draw_area.pos.y}, m_label);

#if 0
		api.DrawCheckerBoardRectangle(yui::DrawApi::RED, allowed_draw_area);
#endif
	}
};

class Toolbar : public yui::HBox
{
  public:
	Toolbar()
	{
		SetStretch(true, false);
		AddNewChild<yui::Spacer>();
		AddNewChild<yui::ButtonWithText>("matsuri", yui::Button::Style::Flat);
	}
};

class MainContainer : public yui::VBox
{
	yui::EventPropagation OnMouse(yui::MouseGesture gesture, yui::Vector, uint32_t& cookie, const yui::Widget&) override
	{
		if (cookie != 0 && gesture == yui::MouseGesture::Press)
			printf("Received cookie %b\n", cookie); // TODO, do proper decoding, check for zero is good enough for now

		return yui::EventPropagation::StopIt; // No more bubbling, thanks
	}
};

void UiFrontend::CreateUi(yui::Frame& root)
{
	auto& main_container = root.SetNewChild<MainContainer>();
	main_container.SetStretch(true, true);

#if 1
	main_container.AddNewChild<Toolbar>();

	{
		// Just another toolbar for debugging purposes
		auto& dev_toolbar = main_container.AddNewChild<yui::HBox>();
		dev_toolbar.SetStretch(true, false);
		dev_toolbar.AddNewChild<yui::Button>().SetStretch(true, true);
		dev_toolbar.AddNewChild<yui::ButtonWithText>("File");
		dev_toolbar.AddNewChild<yui::ButtonWithText>("Edit");
		dev_toolbar.AddNewChild<yui::ButtonWithText>("View");
		dev_toolbar.AddNewChild<yui::ButtonWithText>("Format");
		dev_toolbar.AddNewChild<yui::ButtonWithText>("Tools");
		dev_toolbar.AddNewChild<yui::ButtonWithText>("Table");
		dev_toolbar.AddNewChild<yui::ButtonWithText>("Window");
		dev_toolbar.AddNewChild<yui::ButtonWithText>("Help");
		dev_toolbar.AddNewChild<yui::Button>().SetStretch(true, true);
		dev_toolbar.AddNewChild<yui::ButtonWithText>("?");
	}

	auto& voices = main_container.AddNewChild<yui::HBox>();
	voices.SetStretch(true, true);
	voices.SetWrap(true);

	voices.AddNewChild<VoiceFrame>("bass drum").SetNewChild<VoiceRack>(EXTRA_TRANSIENT).SetCookie(COOKIE_BD);
	voices.AddNewChild<VoiceFrame>("snare drum").SetNewChild<VoiceRack>().SetCookie(COOKIE_SD);
	voices.AddNewChild<VoiceFrame>("hat closed").SetNewChild<VoiceRack>().SetCookie(COOKIE_HC);
	voices.AddNewChild<VoiceFrame>("hat open").SetNewChild<VoiceRack>().SetCookie(COOKIE_HO);
	voices.AddNewChild<VoiceFrame>("cymbal").SetNewChild<VoiceRack>().SetCookie(COOKIE_CY);
	voices.AddNewChild<VoiceFrame>("tom high").SetNewChild<VoiceRack>(EXTRA_TRANSIENT).SetCookie(COOKIE_TH);
	voices.AddNewChild<VoiceFrame>("tom low").SetNewChild<VoiceRack>(EXTRA_TRANSIENT).SetCookie(COOKIE_TL);

	// I cannot think on a purpose for a status bar, so far it only
	// helps on debuging the layout
	main_container.AddNewChild<yui::ButtonWithText>("[status bar]").SetStretch(true, false);
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
