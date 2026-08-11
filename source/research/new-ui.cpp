/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <new>
#include <stdlib.h>

#include "new-ui.hpp"


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
	m_mouse = {0, 0};

	m_root = new Root();
	m_root->SetStretch(true, true); // A good default value
}

void Ui::Screen::Deinitialise() noexcept
{
	delete m_root;
}


class Ui::ScreenFriend final : public Ui::DrawApi
{
  public:
	Ui::Screen* fwend;

	void DrawRectangle(uint32_t colour, Ui::Rect rect) noexcept override
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
				col[0] = static_cast<uint8_t>((colour >> 0) & 0xFF);
				col[1] = static_cast<uint8_t>((colour >> 8) & 0xFF);
				col[2] = static_cast<uint8_t>((colour >> 16) & 0xFF);
				col[3] = static_cast<uint8_t>((colour >> 24) & 0xFF);
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


void Ui::Screen::Update(Position mouse_pos, Size size, int stride, void* out_raw)
{
	m_out = reinterpret_cast<uint8_t*>(out_raw);
	m_out_stride = stride;
	m_size = size;
	m_mouse = mouse_pos;

	ScreenFriend fwend;
	fwend.fwend = this;

	m_root->UpdateNaturalSize(); // [Recursion]

	fwend.DrawRectangle(DrawApi::BKG_COLOUR, {{0, 0}, m_size});
	fwend.DrawRectangle(DrawApi::GREEN, {m_mouse, {32, 32}});

	m_root->Draw(fwend, Rect{{0, 0}, m_size}); // [Recursion]
}


Ui::Wrapper& Ui::Screen::GetRoot()
{
	return *m_root;
}

const char* Ui::Screen::Root::GetType() const
{
	return "Root";
}


// ############################


Ui::Widget::Widget()
{
	m_natural_size = {};
	m_stretch_x = false;
	m_stretch_y = false;
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


// ############################


Ui::Wrapper::Wrapper() : Widget()
{
	m_content = nullptr;
}

Ui::Wrapper::~Wrapper() noexcept
{
	if (m_content != nullptr)
		delete m_content;
}

size_t Ui::Wrapper::GetChildrenNo() const
{
	return 1;
}

Ui::Widget& Ui::Wrapper::SetChild(Widget* widget)
{
	m_content = widget;
	return *widget;
}

Ui::Widget& Ui::Wrapper::GetChild(size_t, Size available_size, Delta* layout_delta_out, Size* child_size_out)
{
	if (layout_delta_out != nullptr)
		*layout_delta_out = {};
	if (child_size_out != nullptr)
		*child_size_out = GetSize(available_size); // Wrapper size is also the size of child
	return *m_content;
}

const Ui::Widget& Ui::Wrapper::GetChild(size_t, Size available_size, Delta* layout_delta_out,
                                        Size* child_size_out) const
{
	if (layout_delta_out != nullptr)
		*layout_delta_out = {};
	if (child_size_out != nullptr)
		*child_size_out = GetSize(available_size);
	return *m_content;
}

Ui::Size Ui::Wrapper::UpdateNaturalSize()
{
	m_natural_size = (m_content != nullptr) ? m_content->UpdateNaturalSize() : Size{32, 32}; // [Recursion]
	return m_natural_size;
}

void Ui::Wrapper::Draw(DrawApi& api, Rect allowed_area) const
{
	if (m_content != nullptr)
		m_content->Draw(api, {allowed_area.pos, GetSize(allowed_area.size)}); // [Recursion]
}


// ############################


Ui::Box::Box(Direction direction) : Container()
{
	m_direction = direction;
	m_non_stretch_size = {};
}

Ui::Box::~Box() noexcept
{
	for (Widget* child : m_children)
		delete child;
}

size_t Ui::Box::GetChildrenNo() const
{
	return m_children.size();
}

Ui::Widget& Ui::Box::AddChild(Widget* widget)
{
	m_children.push_back(widget);
	return *widget;
}

class Ui::BoxFriend
{
  public:
	template <typename T>
	static Ui::Widget& GetChild(T* fwend, size_t no, Ui::Size available_size, Ui::Delta* layout_delta_out,
	                            Ui::Size* child_size_out)
	{
		Delta delta;
		Size size;

		auto child = fwend->m_children.at(no);

		switch (fwend->m_direction)
		{
		case Ui::Box::Direction::Horizontal:
		{
			if (child->GetStretchX() == true)
			{
				available_size.w = (available_size.w - fwend->m_non_stretch_size.w) /
				                   Max(static_cast<int>(fwend->m_stretch_childs), 1);
				available_size.w = Max(available_size.w, child->GetNaturalSize().w);
			}
			size = child->GetSize(available_size);
			delta = {(no < fwend->m_children.size() - 1) ? size.w : 0, 0};
		}
		break;
		case Ui::Box::Direction::Vertical:
		{
			if (child->GetStretchY() == true)
			{
				available_size.h = (available_size.h - fwend->m_non_stretch_size.h) /
				                   Max(static_cast<int>(fwend->m_stretch_childs), 1);
				available_size.h = Max(available_size.h, child->GetNaturalSize().h);
			}
			size = child->GetSize(available_size);
			delta = {0, (no < fwend->m_children.size() - 1) ? size.h : 0};
		}
		break;
		}

		if (layout_delta_out != nullptr)
			*layout_delta_out = delta;
		if (child_size_out != nullptr)
			*child_size_out = size;

		return *child;
	}
};

Ui::Widget& Ui::Box::GetChild(size_t no, Size available_size, Delta* layout_delta_out, Size* child_size_out)
{
	return BoxFriend::GetChild(this, no, available_size, layout_delta_out, child_size_out);
}

const Ui::Widget& Ui::Box::GetChild(size_t no, Size available_size, Delta* layout_delta_out, Size* child_size_out) const
{
	return BoxFriend::GetChild(this, no, available_size, layout_delta_out, child_size_out);
}

const char* Ui::Box::GetType() const
{
	return "Box";
}

void Ui::Box::Draw(DrawApi& api, Rect rect) const
{
	for (size_t i = 0; i < m_children.size(); i += 1)
	{
		Delta delta;
		Size child_size;
		const Widget& child = GetChild(i, rect.size, &delta, &child_size);

		child.Draw(api, {rect.pos, child_size}); // [Recursion]

		switch (m_direction)
		{
		case Direction::Horizontal: rect.pos.x += delta.x; break;
		case Direction::Vertical: rect.pos.y += delta.y; break;
		}
	}
}

Ui::Size Ui::Box::UpdateNaturalSize()
{
	m_natural_size = {};
	m_non_stretch_size = {};
	m_stretch_childs = 0;

	for (Widget* child : m_children)
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

	return m_natural_size;
}

Ui::HBox::HBox() : Box(Direction::Horizontal) {}
Ui::VBox::VBox() : Box(Direction::Vertical) {}


// ############################


Ui::Button::Button(const char*) : Wrapper() {}

const char* Ui::Button::GetType() const
{
	return "Button";
}

void Ui::Button::Draw(DrawApi& api, Rect allowed_area) const
{
	api.Draw3dBevel({allowed_area.pos, GetSize(allowed_area.size)}, DrawApi::BevelStyle::Outset);
}
