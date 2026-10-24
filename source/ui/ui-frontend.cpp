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
static constexpr uint32_t COOKIE_TUN = 0b000'011;
static constexpr uint32_t COOKIE_LEN = 0b000'100;
static constexpr uint32_t COOKIE_MUTE = 0b000'101;
static constexpr uint32_t COOKIE_SOLO = 0b000'110;
static constexpr uint32_t COOKIE_MORE = 0b000'111;

static constexpr uint32_t COOKIE_BD = 0b001'000;
static constexpr uint32_t COOKIE_SD = 0b010'000;
static constexpr uint32_t COOKIE_HC = 0b011'000;
static constexpr uint32_t COOKIE_HO = 0b100'000;
static constexpr uint32_t COOKIE_CY = 0b101'000;
static constexpr uint32_t COOKIE_TH = 0b110'000;
static constexpr uint32_t COOKIE_TL = 0b111'000;


class Knob : public yui::Button
{
  public:
	Knob(const std::string& label, const std::string& measure)
	{
		auto& box = SetNewChild<yui::VBox>();
		box.AddNewChild<yui::Text>(measure);
		box.AddNewChild<yui::Text>(label);
	}

	yui::EventPropagation OnMouse(yui::MouseGesture gesture, yui::Vector position, uint32_t& cookie,
	                              const Widget& target) override
	{
		const auto ret = yui::Button::OnMouse(gesture, position, cookie, target);
		if (target.GetClassId() == "ButtonWithText")
			cookie = GetCookie(); // Replace with our cookie
		return ret;
	}
};

class VoiceRack : public yui::HBox
{
  public:
	VoiceRack(const char* label)
	{
		SetStretch(true, false);

		auto& vbox = AddNewChild<yui::VBox>();
		vbox.SetStretch(true, false);

		// TODO, I need a table widget

		auto& row1 = vbox.AddNewChild<yui::HBox>();
		row1.SetStretch(true, false);
		row1.AddNewChild<yui::Text>(label).SetId("RackText");
		row1.AddNewChild<yui::Spacer>();
		row1.AddNewChild<yui::ButtonWithText>("+").SetCookie(COOKIE_MORE);

		auto& row2 = vbox.AddNewChild<yui::HBox>();
		row2.AddNewChild<yui::ButtonWithText>("M").SetCookie(COOKIE_MUTE);
		row2.AddNewChild<yui::ButtonWithText>("S").SetCookie(COOKIE_SOLO);

		auto& row3 = vbox.AddNewChild<yui::HBox>();
		row3.SetStretch(true, false);
		row3.AddNewChild<Knob>("VOL", "100%").SetCookie(COOKIE_VOL).SetStretch(true, false);
		row3.AddNewChild<Knob>("PAN", "CEN").SetCookie(COOKIE_PAN).SetStretch(true, false);

		auto& row4 = vbox.AddNewChild<yui::HBox>();
		row4.SetStretch(true, false);
		row4.AddNewChild<Knob>("TUN", "0").SetCookie(COOKIE_TUN).SetStretch(true, false);
		row4.AddNewChild<Knob>("LEN", "100%").SetCookie(COOKIE_LEN).SetStretch(true, false);
	}

	yui::EventPropagation OnMouse(yui::MouseGesture, yui::Vector, uint32_t& cookie, const yui::Widget&) override
	{
		cookie |= GetCookie(); // Append our cookie
		return yui::EventPropagation::KeepPassingIt;
	}
};

class AboutRack : public yui::HBox
{
  public:
	AboutRack()
	{
		AddNewChild<yui::ButtonWithText>("Matsuri", yui::Button::Style::Flat);
	}
};

class RackFrame : public yui::Frame // Just decoration
{
  public:
	static constexpr int PADDING = 16; // Is not a good idea to bloat yuika with proper padding right now,
	                                   // let's keep it on the widget side of things

	RackFrame()
	{
		SetStretch(true, false);
		SetClassId("RackFrame");
	}

	void OnLayout(yui::LayoutApi&) override
	{
		m_minimum_size.x = PADDING;
		m_minimum_size.y = PADDING;
		if (m_content != nullptr)
			m_minimum_size += m_content->GetMinimumSize() + PADDING;
	}

	yui::Widget::ChildInLayout GetChildInLayout(yui::LayoutApi&, size_t, yui::Vector size, yui::Vector) const override
	{
		return {*m_content, {PADDING / 2, PADDING / 2}, m_content->GetSize(size - PADDING), {}};
	}

	void OnDraw(yui::DrawApi& api, yui::Rect allowed_draw_area) override
	{
		yui::Vector size = GetSize(allowed_draw_area.size);
		size.x = std::max(size.x, m_minimum_size.x - PADDING); // Try to preserve minimum size in the cases where
		size.y = std::max(size.y, m_minimum_size.y - PADDING); // screen gives us less that that... obviously it
		                                                       // will clip

		api.SetHitArea({allowed_draw_area.pos, size});
		api.DrawBorder({allowed_draw_area.pos, size}, yui::DrawApi::BorderStyle::Frame);

#if 0
		api.DrawCheckerBoardRectangle(yui::DrawApi::RED, allowed_draw_area);
#endif
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
	if (true)
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

	voices.AddNewChild<RackFrame>().SetNewChild<VoiceRack>("Bass Drum").SetCookie(COOKIE_BD);
	voices.AddNewChild<RackFrame>().SetNewChild<VoiceRack>("Snare Drum").SetCookie(COOKIE_SD);
	voices.AddNewChild<RackFrame>().SetNewChild<VoiceRack>("Hat Closed").SetCookie(COOKIE_HC);
	voices.AddNewChild<RackFrame>().SetNewChild<VoiceRack>("Hat Open").SetCookie(COOKIE_HO);
	voices.AddNewChild<RackFrame>().SetNewChild<VoiceRack>("Cymbal").SetCookie(COOKIE_CY);
	voices.AddNewChild<RackFrame>().SetNewChild<VoiceRack>("Tom High").SetCookie(COOKIE_TH);
	voices.AddNewChild<RackFrame>().SetNewChild<VoiceRack>("Tom Low").SetCookie(COOKIE_TL);
	voices.AddNewChild<RackFrame>().SetNewChild<AboutRack>();

	// I cannot think on a purpose for a status bar, so far it only
	// helps on debuging the layout
	if (true)
	{
		main_container.AddNewChild<yui::ButtonWithText>("[status bar]").SetStretch(true, false);
	}
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
