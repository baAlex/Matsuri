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

template <typename T> static T Min(T a, T b)
{
	return (a < b) ? a : b;
}
template <typename T> static T Max(T a, T b)
{
	return (a > b) ? a : b;
}
template <typename T> static T Clamp(T v, T min, T max)
{
	return Min(Max(v, min), max);
}


void Ui::Initialise()
{
	m_out = m_dummy;
	m_out_stride = 4;
	m_size = {1, 1};
	m_mouse = {0, 0};

	// TODO, return failure
	m_root = reinterpret_cast<Root*>(malloc(sizeof(Root)));
	(new (m_root) Root())->Initialise();
	m_root->SetStretch(true, true); // A good default value
}

void Ui::Deinitialise()
{
	m_root->Deinitialise(); // [Recursion]
	free(m_root);
}


void Ui::Update(Position mouse_pos, Size size, int stride, void* out_raw)
{
	m_out = reinterpret_cast<uint8_t*>(out_raw);
	m_out_stride = stride;
	m_size = size;
	m_mouse = mouse_pos;

	DrawRectangle(BKG_COLOUR, {{0, 0}, m_size});
	DrawRectangle(GREEN, {m_mouse, {32, 32}});

	m_root->UpdateNaturalSize();              // [Recursion]
	m_root->Draw(this, Rect{{0, 0}, m_size}); // [Recursion]
}


void Ui::DrawRectangle(uint32_t colour, Rect rect)
{
	const int x1 = Clamp(rect.pos.x, 0, m_size.w);
	const int y1 = Clamp(rect.pos.y, 0, m_size.h);
	rect.size.w = (Clamp(rect.pos.x + rect.size.w, 0, m_size.w) - x1) * 4;
	rect.size.h = (Clamp(rect.pos.y + rect.size.h, 0, m_size.h) - y1) * m_out_stride;

	uint8_t* out = m_out + (x1 * 4) + (y1 * m_out_stride);
	for (uint8_t* row = out; row < out + rect.size.h; row += m_out_stride)
	{
		for (uint8_t* col = row; col < row + rect.size.w; col += 4)
		{
			col[0] = (colour >> 0) & 0xFF;
			col[1] = (colour >> 8) & 0xFF;
			col[2] = (colour >> 16) & 0xFF;
			col[3] = (colour >> 24) & 0xFF;
		}
	}
}

void Ui::Draw3dBevel(Rect rect, BevelStyle style)
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


Ui::Wrapper* Ui::GetRoot()
{
	return m_root;
}

void Ui::Root::Initialise()
{
	Wrapper::Initialise(nullptr, nullptr);
}

const char* Ui::Root::GetType() const
{
	return "Root";
}


// ############################


Ui::Widget* Ui::Widget::Initialise(Container* container_parent, Wrapper* wrapper_parent)
{
	m_parent = nullptr;

	if (container_parent != nullptr)
	{
		m_parent = container_parent;
		container_parent->AppendChild(this); // [Recursion]
	}
	else if (wrapper_parent != nullptr)
	{
		m_parent = wrapper_parent;
		wrapper_parent->SetChild(this); // [Recursion]
	}

	MakeItDirty(); // Sets 'm_dirty' and 'm_natural_size', [Recursion]

	m_stretch_x = false;
	m_stretch_y = false;

	return this;
}

void Ui::Widget::Deinitialise() {}

Ui::Size Ui::Widget::GetNaturalSize() const
{
	return m_natural_size;
}

Ui::Widget* Ui::Widget::SetStretch(bool x, bool y)
{
	m_stretch_x = x;
	m_stretch_y = y;
	return this;
}

bool Ui::Widget::GetStretchX() const
{
	return m_stretch_x;
}

bool Ui::Widget::GetStretchY() const
{
	return m_stretch_y;
}

void Ui::Widget::MakeItDirty()
{
	m_dirty = true;
	m_natural_size = {};
	if (m_parent != nullptr)
		m_parent->MakeItDirty(); // [Recursion]
}

Ui::Size Ui::Widget::GetSize(Size available_size) const
{
	Size size = m_natural_size;
	size.w = (GetStretchX() == true || size.w > available_size.w) ? available_size.w : size.w;
	size.h = (GetStretchY() == true || size.h > available_size.h) ? available_size.h : size.h;
	return size;
}


// ############################


Ui::Wrapper* Ui::Wrapper::Initialise(Container* container_parent, Wrapper* wrapper_parent, Widget* child)
{
	Widget::Initialise(container_parent, wrapper_parent); // [Recursion]
	m_content = child;
	return this;
}

void Ui::Wrapper::Deinitialise()
{
	if (m_content != nullptr)
	{
		m_content->Deinitialise(); // [Recursion]
		free(m_content);
	}
}

int Ui::Wrapper::GetChildrenNo() const
{
	return 1;
}

void Ui::Wrapper::SetChild(Widget* widget)
{
	m_content = widget;
	MakeItDirty(); // [Recursion]
}

Ui::Widget* Ui::Wrapper::GetChild(int, Size available_size, Delta* layout_delta_out, Size* child_size_out)
{
	if (layout_delta_out != nullptr)
		*layout_delta_out = {};
	if (child_size_out != nullptr)
		*child_size_out = GetSize(available_size); // Wrapper size is also the size of child
	return m_content;
}

const Ui::Widget* Ui::Wrapper::GetChild(int, Size available_size, Delta* layout_delta_out, Size* child_size_out) const
{
	if (layout_delta_out != nullptr)
		*layout_delta_out = {};
	if (child_size_out != nullptr)
		*child_size_out = GetSize(available_size);
	return m_content;
}

Ui::Size Ui::Wrapper::UpdateNaturalSize()
{
	if (m_dirty == false)
		return m_natural_size;
	m_dirty = false;
	m_natural_size = (m_content != nullptr) ? m_content->UpdateNaturalSize() : Size{32, 32}; // [Recursion]
	return m_natural_size;
}

void Ui::Wrapper::Draw(Ui* ui, Rect allowed_area) const
{
	if (m_content != nullptr)
		m_content->Draw(ui, {allowed_area.pos, GetSize(allowed_area.size)}); // [Recursion]
}

Ui::Wrapper* Ui::Wrapper::SetStretch(bool x, bool y)
{
	Widget::SetStretch(x, y);
	return this;
}


// ############################


Ui::Container* Ui::Container::Initialise(Container* container_parent, Wrapper* wrapper_parent)
{
	Widget::Initialise(container_parent, wrapper_parent); // [Recursion]
	return this;
}

Ui::Container* Ui::Container::SetStretch(bool x, bool y)
{
	Widget::SetStretch(x, y);
	return this;
}


// ############################


Ui::Box* Ui::Box::Create(Container* parent, Direction direction)
{
	return (new (malloc(sizeof(Box))) Box())->Initialise(parent, nullptr, direction);
}

Ui::Box* Ui::Box::Create(Wrapper* parent, Direction direction)
{
	return (new (malloc(sizeof(Box))) Box())->Initialise(nullptr, parent, direction);
}

Ui::Box* Ui::Box::Initialise(Container* container_parent, Wrapper* wrapper_parent, Direction direction)
{
	Container::Initialise(container_parent, wrapper_parent); // [Recursion]
	m_direction = direction;
	m_children_no = 0;
	m_stretch_childs = 0;
	m_non_stretch_size = {};
	return this;
}

void Ui::Box::Deinitialise()
{
	for (int i = 0; i < m_children_no; i += 1)
	{
		m_children[i]->Deinitialise(); // [Recursion]
		free(m_children[i]);
	}
}

int Ui::Box::GetChildrenNo() const
{
	return m_children_no;
}

int Ui::Box::AppendChild(Widget* widget)
{
	if (m_children_no == MAX_CHILDREN)
		return MAX_CHILDREN;

	m_children[m_children_no++] = widget;
	MakeItDirty(); // [Recursion]

	return m_children_no - 1;
}


class Ui::BoxFriend
{
  public:
	template <typename T>
	static Ui::Widget* GetChild(T* fwend, int no, Ui::Size available_size, Ui::Delta* layout_delta_out,
	                            Ui::Size* child_size_out)
	{
		Delta delta;
		Size size;

		switch (fwend->m_direction)
		{
		case Ui::Box::Direction::Horizontal:
		{
			if (fwend->m_children[no]->GetStretchX() == true)
			{
				available_size.w = (available_size.w - fwend->m_non_stretch_size.w) / Max(fwend->m_stretch_childs, 1);
				available_size.w = Max(available_size.w, fwend->m_children[no]->GetNaturalSize().w);
			}
			size = fwend->m_children[no]->GetSize(available_size);
			delta = {(no - fwend->m_children_no < 0) ? size.w : 0, 0};
		}
		break;
		case Ui::Box::Direction::Vertical:
		{
			if (fwend->m_children[no]->GetStretchY() == true)
			{
				available_size.h = (available_size.h - fwend->m_non_stretch_size.h) / Max(fwend->m_stretch_childs, 1);
				available_size.h = Max(available_size.h, fwend->m_children[no]->GetNaturalSize().h);
			}
			size = fwend->m_children[no]->GetSize(available_size);
			delta = {0, (no - fwend->m_children_no < 0) ? size.h : 0};
		}
		break;
		}

		if (layout_delta_out != nullptr)
			*layout_delta_out = delta;
		if (child_size_out != nullptr)
			*child_size_out = size;

		return fwend->m_children[no];
	}
};

Ui::Widget* Ui::Box::GetChild(int no, Size available_size, Delta* layout_delta_out, Size* child_size_out)
{
	return Ui::BoxFriend::GetChild(this, no, available_size, layout_delta_out, child_size_out);
}

const Ui::Widget* Ui::Box::GetChild(int no, Size available_size, Delta* layout_delta_out, Size* child_size_out) const
{
	return Ui::BoxFriend::GetChild(this, no, available_size, layout_delta_out, child_size_out);
}


const char* Ui::Box::GetType() const
{
	return "Box";
}

void Ui::Box::Draw(Ui* ui, Rect rect) const
{
	for (int i = 0; i < m_children_no; i += 1)
	{
		Delta delta;
		Size child_size;
		const Widget* child = GetChild(i, rect.size, &delta, &child_size);

		child->Draw(ui, {rect.pos, child_size}); // [Recursion]

		switch (m_direction)
		{
		case Direction::Horizontal: rect.pos.x += delta.x; break;
		case Direction::Vertical: rect.pos.y += delta.y; break;
		}
	}
}

Ui::Size Ui::Box::UpdateNaturalSize()
{
	if (m_dirty == false)
		return m_natural_size;

	m_natural_size = {};
	m_stretch_childs = 0;

	for (int i = 0; i < m_children_no; i += 1)
	{
		const Size size = m_children[i]->UpdateNaturalSize(); // [Recursion]

		switch (m_direction)
		{
		case Direction::Horizontal:
			m_natural_size.w += size.w;
			m_natural_size.h = Max(size.h, m_natural_size.h);
			m_stretch_childs += m_children[i]->GetStretchX();
			break;
		case Direction::Vertical:
			m_natural_size.w = Max(size.w, m_natural_size.w);
			m_natural_size.h += size.h;
			m_stretch_childs += m_children[i]->GetStretchY();
			break;
		}

		if (m_children[i]->GetStretchX() == false)
			m_non_stretch_size.w += size.w;
		if (m_children[i]->GetStretchY() == false)
			m_non_stretch_size.h += size.h;
	}

	m_dirty = false;
	return m_natural_size;
}


Ui::Box* Ui::HBox::Create(Ui::Container* parent)
{
	return Box::Create(parent, Direction::Horizontal);
}

Ui::Box* Ui::HBox::Create(Ui::Wrapper* parent)
{
	return Box::Create(parent, Direction::Horizontal);
}

Ui::Box* Ui::VBox::Create(Ui::Container* parent)
{
	return Box::Create(parent, Direction::Vertical);
}

Ui::Box* Ui::VBox::Create(Ui::Wrapper* parent)
{
	return Box::Create(parent, Direction::Vertical);
}


// ############################


Ui::Button* Ui::Button::Create(Container* parent, const char* text)
{
	return (new (malloc(sizeof(Button))) Button())->Initialise(parent, nullptr, text);
}

Ui::Button* Ui::Button::Create(Wrapper* parent, const char* text)
{
	return (new (malloc(sizeof(Button))) Button())->Initialise(nullptr, parent, text);
}

Ui::Button* Ui::Button::Initialise(Container* container_parent, Wrapper* wrapper_parent, const char* text)
{
	(void)text;
	Wrapper::Initialise(container_parent, wrapper_parent,
	                    /*(text != nullptr) ? Text::Create(this, text) :*/ nullptr); // [Recursion]
	return this;
}

const char* Ui::Button::GetType() const
{
	return "Button";
}

void Ui::Button::Draw(Ui* ui, Rect allowed_area) const
{
	ui->Draw3dBevel({allowed_area.pos, GetSize(allowed_area.size)}, BevelStyle::Outset);
}
