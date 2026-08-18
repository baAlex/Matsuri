/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <algorithm>
#include <new>
#include <stdlib.h>

#include "new-ui.hpp"

#if 1
#define DEBUGPRINT(...) __builtin_printf(__VA_ARGS__)
#else
#define DEBUGPRINT(...) // Empty
#endif

template <typename T> static T Min(T a, T b) noexcept
{
	return (a < b) ? a : b;
}
template <typename T> static T Max(T a, T b) noexcept
{
	return (a > b) ? a : b;
}
template <typename T> static T Clamp(T v, T min, T max) noexcept
{
	return Min(Max(v, min), max);
}


void Ui::Screen::Initialise()
{
	m_out = m_dummy;
	m_out_stride = 4;
	m_size = {1, 1};

	m_root = new Root();
	m_root->SetStretch(true, true); // A good default value

	m_click_pressed = nullptr;
}

void Ui::Screen::Deinitialise() noexcept
{
	delete m_root;
}


struct DrawStackEntry
{
	unsigned depth;
	Ui::Widget* widget;
	Ui::Rect rect;
};

static constexpr size_t STACK_LEN = 256;
static thread_local DrawStackEntry s_stack[STACK_LEN];

class Ui::ScreenFriend
{
  public:
	class DrawApiImplementation final : public Ui::DrawApi
	{
	  public:
		Ui::Screen* fwend;
		bool clickable_set;
		struct Ui::Screen::Clickable clickable;

		void SetClickableArea(Ui::Rect rect) noexcept override
		{
			const int x1 = Clamp(rect.pos.x, 0, fwend->m_size.w);
			const int y1 = Clamp(rect.pos.y, 0, fwend->m_size.h);
			const int x2 = Clamp(rect.pos.x + rect.size.w, 0, fwend->m_size.w);
			const int y2 = Clamp(rect.pos.y + rect.size.h, 0, fwend->m_size.h);

			clickable_set = true; // If already set, we overwrite last one
			clickable.a = {x1, y1};
			clickable.b = {x2, y2};
		}

		void DrawRectangle(Colour colour, Ui::Rect rect) noexcept override
		{
			const int x1 = Clamp(rect.pos.x, 0, fwend->m_size.w);
			const int y1 = Clamp(rect.pos.y, 0, fwend->m_size.h);
			rect.size.w = (Clamp(rect.pos.x + rect.size.w, 0, fwend->m_size.w) - x1) * 4;
			rect.size.h = (Clamp(rect.pos.y + rect.size.h, 0, fwend->m_size.h) - y1) * fwend->m_out_stride;

			uint8_t* out = fwend->m_out + (x1 * 4) + (y1 * fwend->m_out_stride);
			for (uint8_t* row = out; row < out + rect.size.h; row += fwend->m_out_stride)
			{
				for (uint8_t* col = row; col < row + rect.size.w; col += 4)
				{
					col[0] = colour.a;
					col[1] = colour.b;
					col[2] = colour.g;
					col[3] = colour.r;
				}
			}
		}

		void Draw3dBevel(Ui::Rect rect, BevelStyle style) noexcept override
		{
			if (style == BevelStyle::Inset)
			{
				DrawRectangle(BEVEL_MID_COLOUR, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(BEVEL_MID_COLOUR, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(BEVEL_LIGHT_COLOUR, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(BEVEL_LIGHT_COLOUR, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});
			}
			else
			{
				DrawRectangle(BEVEL_LIGHT_COLOUR, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(BEVEL_LIGHT_COLOUR, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(BEVEL_SHADOW_COLOUR, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(BEVEL_SHADOW_COLOUR, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});

				DrawRectangle(BEVEL_MID_COLOUR, {{rect.pos.x + rect.size.w - 2, rect.pos.y + 1}, {1, rect.size.h - 2}});
				DrawRectangle(BEVEL_MID_COLOUR, {{rect.pos.x + 1, rect.pos.y + rect.size.h - 2}, {rect.size.w - 2, 1}});
			}
		}
	};

	static void DrawWidgets(DrawApiImplementation& api)
	{
		// Non-recursive draw, it has the good feature of carry information while
		// descending the tree, like depth, and also from our end we can identify
		// on what widget we are without asking it to widgets themselves

		size_t cursor = 0;
		s_stack[cursor++] = {0, api.fwend->m_root, Rect{{0, 0}, api.fwend->m_size}};

		while (cursor > 0)
		{
			DrawStackEntry current = s_stack[--cursor]; // Yes, copy it

			// Draw
			api.clickable_set = false;
			current.widget->Draw(api, current.rect);

			if (api.clickable_set == true) // Widget set a clickable area
			{
				api.clickable.depth = current.depth;
				api.clickable.widget = current.widget;

				// Widget wants to receive clicks
				if ((api.clickable.widget->GetReceivingEvents() & EVENT_MOUSE_CLICK) != 0)
					api.fwend->m_clickables[api.fwend->m_clickables_no++] = api.clickable; // TODO
			}

			// Stack children
			if (cursor + current.widget->GetChildrenNo() >= STACK_LEN)
			{
				fprintf(stderr, "Too many widgets\n");
				return;
			}

			for (size_t i = 0; i < current.widget->GetChildrenNo(); i += 1)
			{
				const auto [child, delta, child_size] = current.widget->GetChild(i, current.rect.size);

				s_stack[cursor++] = {current.depth + 1, &child, {current.rect.pos, child_size}};

				current.rect.pos.x += delta.x;
				current.rect.pos.y += delta.y;
			}
		}
	}
};


static constexpr bool UPDATE_NATURAL_SIZE_LIKE_CRAZY = false;
static constexpr bool DRAW_LIKE_CRAZY = false;

static thread_local unsigned s_frame = 0;


void Ui::Screen::Update(Size size, int stride, void* out_raw)
{
	m_out = reinterpret_cast<uint8_t*>(out_raw);
	m_out_stride = stride;

	m_root->UpdateNaturalSize(); // [Recursion]

	if (m_size.w != size.w || m_size.h != size.h || DRAW_LIKE_CRAZY == true)
	{
		// Set stuff
		m_size = size;

		ScreenFriend::DrawApiImplementation api;
		api.fwend = this;

		api.DrawRectangle(BKG_COLOUR, {{0, 0}, m_size});

		// Draw
		m_clickables_no = 0; // DrawWidgets() next will recreate them
		ScreenFriend::DrawWidgets(api);

		// Sort clickable areas,
		// so when dispatching events we start close from those widgets that,
		// potentially, setup everything to receive events (buttons mostly)
		if (m_clickables_no != 0)
			std::sort(m_clickables, m_clickables + m_clickables_no,
			          [](Clickable& a, Clickable& b) { return a.depth > b.depth; });

		// for (size_t i = 0; i < m_clickables_no; i += 1)
		//	DEBUGPRINT("%i\n", m_clickables[i].depth);
		//  DEBUGPRINT("### %zu\n", m_clickables_no);
	}

	s_frame++;
}

void Ui::Screen::MouseClick(MouseClickGesture gesture, Position mouse_pos)
{
	// TODO, check if a parent also set a clickable, ask if it wants to intersect event

	switch (gesture)
	{
	case MouseClickGesture::Press:
		if (m_click_pressed != nullptr) // Yes
		{
			m_click_pressed->OnMouseClick(MouseClickGesture::Release, mouse_pos);
			m_click_pressed = nullptr;
		}

		for (size_t i = 0; i < m_clickables_no; i += 1)
		{
			const Clickable& c = m_clickables[i];
			if (mouse_pos.x < c.a.x || mouse_pos.y < c.a.y || mouse_pos.x > c.b.x || mouse_pos.y > c.b.y)
				continue;

			c.widget->OnMouseClick(MouseClickGesture::Press, mouse_pos);
			m_click_pressed = c.widget;
			break;
		}
		break;

	case MouseClickGesture::Release:
		if (m_click_pressed != nullptr)
		{
			m_click_pressed->OnMouseClick(MouseClickGesture::Release, mouse_pos);
			m_click_pressed = nullptr;
		}
		break;
	}
}


Ui::Wrapper& Ui::Screen::GetRoot()
{
	return *m_root;
}

std::string_view Ui::Screen::Root::GetType() const
{
	return "Root";
}


// ############################


Ui::Widget::Widget()
{
	m_stretch_x = false; // Is not possible to set bitfields on headers
	m_stretch_y = false; // (a C++ quirk)
	m_natural_size_updated = false;
}

Ui::Size Ui::Widget::GetNaturalSize() const
{
	return m_natural_size;
}

Ui::Widget& Ui::Widget::SetStretch(bool x, bool y)
{
	m_stretch_x = x;
	m_stretch_y = y;
	return *this;
}

bool Ui::Widget::GetStretchX() const
{
	return m_stretch_x;
}

bool Ui::Widget::GetStretchY() const
{
	return m_stretch_y;
}

Ui::Size Ui::Widget::GetSize(Size available_size) const
{
	Size size = m_natural_size;
	size.w = (GetStretchX() == true || size.w > available_size.w) ? available_size.w : size.w;
	size.h = (GetStretchY() == true || size.h > available_size.h) ? available_size.h : size.h;
	return size;
}

void Ui::Widget::Draw(DrawApi& api, Rect allowed_area) const
{
	// There are less surprises by setting a clickable area by default,
	// it still can be overridden if more fine control is needed
	api.SetClickableArea({allowed_area.pos, GetSize(allowed_area.size)});
}

void Ui::Widget::SetReceivingEvents(uint32_t events)
{
	m_receiving_events = events;
}

uint32_t Ui::Widget::GetReceivingEvents() const
{
	return m_receiving_events;
};

void Ui::Widget::OnMouseClick(MouseClickGesture, Position){};


// ############################


size_t Ui::Wrapper::GetChildrenNo() const
{
	if (m_content == nullptr)
		return 0;
	return 1;
}

Ui::Widget& Ui::Wrapper::SetChild(std::unique_ptr<Widget> widget)
{
	m_content = std::move(widget);
	m_natural_size_updated = false;
	return *m_content;
}

Ui::Widget::ChildGet Ui::Wrapper::GetChild(size_t, Size available_size)
{
	if (m_content == nullptr)
		throw 1; // TODO
	return {*m_content, {}, GetSize(available_size)};
}

const Ui::Widget::ChildGet Ui::Wrapper::GetChild(size_t, Size available_size) const
{
	if (m_content == nullptr)
		throw 1; // TODO
	return {*m_content, {}, GetSize(available_size)};
}

Ui::Widget& Ui::Wrapper::GetChild(size_t)
{
	if (m_content == nullptr)
		throw 1; // TODO
	return *m_content;
}

const Ui::Widget& Ui::Wrapper::GetChild(size_t) const
{
	if (m_content == nullptr)
		throw 1; // TODO
	return *m_content;
}

Ui::Size Ui::Wrapper::UpdateNaturalSize()
{
	if (m_natural_size_updated == false || UPDATE_NATURAL_SIZE_LIKE_CRAZY == true)
	{
		// DEBUGPRINT("%u | Ui::Wrapper::UpdateNaturalSize\n", s_frame);
		m_natural_size_updated = true;
		m_natural_size = (m_content != nullptr) ? m_content->UpdateNaturalSize() : Size{32, 32}; // [Recursion]
	}

	return m_natural_size;
}


// ############################


Ui::Box::Box(Direction direction) : Container()
{
	m_direction = direction;
	m_non_stretch_size = {};
}

size_t Ui::Box::GetChildrenNo() const
{
	return m_children.size();
}

Ui::Widget& Ui::Box::AddChild(std::unique_ptr<Widget> widget)
{
	m_children.push_back(std::move(widget));
	m_natural_size_updated = false;
	return *m_children.back();
}

class Ui::BoxFriend
{
  public:
	template <typename T> static Ui::Widget::ChildGet GetChild(T& fwend, size_t no, Ui::Size available_size)
	{
		Delta delta;
		Size size;

		if (no >= fwend.m_children.size())
			throw 1; // TODO

		auto& child = fwend.m_children.at(no);

		switch (fwend.m_direction)
		{
		case Ui::Box::Direction::Horizontal:
		{
			if (child->GetStretchX() == true)
			{
				available_size.w =
				    (available_size.w - fwend.m_non_stretch_size.w) / Max(static_cast<int>(fwend.m_stretch_childs), 1);
				available_size.w = Max(available_size.w, child->GetNaturalSize().w);
			}
			size = child->GetSize(available_size);
			delta = {(no < fwend.m_children.size() - 1) ? size.w : 0, 0};
		}
		break;
		case Ui::Box::Direction::Vertical:
		{
			if (child->GetStretchY() == true)
			{
				available_size.h =
				    (available_size.h - fwend.m_non_stretch_size.h) / Max(static_cast<int>(fwend.m_stretch_childs), 1);
				available_size.h = Max(available_size.h, child->GetNaturalSize().h);
			}
			size = child->GetSize(available_size);
			delta = {0, (no < fwend.m_children.size() - 1) ? size.h : 0};
		}
		break;
		}

		return {*child, delta, size};
	}
};

Ui::Widget::ChildGet Ui::Box::GetChild(size_t no, Size available_size)
{
	return BoxFriend::GetChild(*this, no, available_size);
}

const Ui::Widget::ChildGet Ui::Box::GetChild(size_t no, Size available_size) const
{
	return BoxFriend::GetChild(*this, no, available_size);
}

Ui::Widget& Ui::Box::GetChild(size_t no)
{
	if (no >= m_children.size())
		throw 1; // TODO
	return *m_children.at(no);
}

const Ui::Widget& Ui::Box::GetChild(size_t no) const
{
	if (no >= m_children.size())
		throw 1; // TODO
	return *m_children.at(no);
}

std::string_view Ui::Box::GetType() const
{
	return "Box";
}

Ui::Size Ui::Box::UpdateNaturalSize()
{
	if (m_natural_size_updated == false || UPDATE_NATURAL_SIZE_LIKE_CRAZY == true)
	{
		// DEBUGPRINT("%u | Ui::Box::UpdateNaturalSize\n", s_frame);
		m_natural_size_updated = true;
		m_natural_size = {};
		m_non_stretch_size = {};
		m_stretch_childs = 0;

		for (auto& child : m_children)
		{
			const Size size = child->UpdateNaturalSize(); // [Recursion]

			switch (m_direction)
			{
			case Direction::Horizontal:
				m_natural_size.w += size.w;
				m_natural_size.h = Max(size.h, m_natural_size.h);
				m_stretch_childs += static_cast<size_t>(child->GetStretchX());
				break;
			case Direction::Vertical:
				m_natural_size.w = Max(size.w, m_natural_size.w);
				m_natural_size.h += size.h;
				m_stretch_childs += static_cast<size_t>(child->GetStretchY());
				break;
			}

			if (child->GetStretchX() == false)
				m_non_stretch_size.w += size.w;
			if (child->GetStretchY() == false)
				m_non_stretch_size.h += size.h;
		}
	}

	return m_natural_size;
}

Ui::HBox::HBox() : Box(Direction::Horizontal) {}
Ui::VBox::VBox() : Box(Direction::Vertical) {}


// ############################


Ui::Button::Button(std::string text) : Wrapper()
{
	m_text = text; // TODO, it has to create a label
}

std::string_view Ui::Button::GetType() const
{
	return "Button";
}

void Ui::Button::Draw(DrawApi& api, Rect allowed_area) const
{
	// DEBUGPRINT("%u | Ui::Button::Draw\n", s_frame);
	api.SetClickableArea({allowed_area.pos, GetSize(allowed_area.size)});
	api.Draw3dBevel({allowed_area.pos, GetSize(allowed_area.size)}, DrawApi::BevelStyle::Outset);
}

void Ui::Button::SetMouseClickCallback(std::function<MouseClickCallback> callback)
{
	SetReceivingEvents(GetReceivingEvents() | Ui::EVENT_MOUSE_CLICK);
	m_mouse_click_callback = callback;
}

void Ui::Button::OnMouseClick(Ui::MouseClickGesture gesture, Ui::Position mouse_pos)
{
	m_mouse_click_callback(*this, gesture, mouse_pos);
}
