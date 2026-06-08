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


#define I_DO_NOT_WANT_MY_STACK_TO_BE_DESTROYED // Undefine it for debuging (or curiosity) purposes
                                               // (EDIT: funny name, but it's somewhat outdated)

#if 1
#define DEBUGPRINT(...) // Empty
#else
#define DEBUGPRINT(...) __builtin_printf(__VA_ARGS__)
#endif


// ================================
// Window
// ================================

Ui::Window* Ui::Window::Create(DrawAPI* draw_api)
{
	assert(draw_api != nullptr);
	return new Window(draw_api);
}

Ui::Window::Window(DrawAPI* draw_api) : Wrapper(nullptr, nullptr)
{
	m_draw_api = draw_api;
	m_content = nullptr;
}

Ui::Window::~Window()
{
	if (m_content != nullptr)
		delete m_content;
}

const char* Ui::Window::GetType() const
{
	return "Window";
}

Size Ui::Window::UpdateLayout()
{
#ifdef I_DO_NOT_WANT_MY_STACK_TO_BE_DESTROYED
	if (m_dirty == false)
		return m_natural_size;
#endif

	// Re-calculate layout
	DEBUGPRINT("%p Window::UpdateLayout()\n", reinterpret_cast<void*>(this));

	if (m_content != nullptr)
		m_natural_size = m_content->UpdateLayout();
	else
		m_natural_size = {};

	m_dirty = false;
	return m_natural_size;
}

void Ui::Window::Draw(Rect allowed_area) const
{
	m_draw_api->Draw3dBevel(allowed_area);
}

DrawAPI* Ui::Window::GetDrawApi() const
{
	return m_draw_api;
}


// ================================
// Box
// ================================

Ui::Box* Ui::Box::Create(Container* parent, Direction direction)
{
	assert(parent != nullptr);
	return new Box(parent, nullptr, direction);
}

Ui::Box* Ui::Box::Create(Wrapper* parent, Direction direction)
{
	assert(parent != nullptr);
	return new Box(nullptr, parent, direction);
}

Ui::Box::Box(Container* container_parent, Wrapper* wrapper_parent, Direction direction)
    : Container(container_parent, wrapper_parent)
{
	assert(container_parent != nullptr || wrapper_parent != nullptr);

	m_direction = direction;
	m_children_no = 0;
}

Ui::Box::~Box()
{
	for (int i = 0; i < m_children_no; i += 1)
		delete m_children[i];
}

int Ui::Box::GetChildrenNo() const
{
	return m_children_no;
}

static constexpr float SPACING = 0.2f; // TODO, hardcoded for now

class Ui::BoxFriend
{
  public:
	template <typename T>
	static Ui::Widget* GetChild(T& fwend, int no, Size available_size, Delta* layout_delta_out, Size* size_out)
	{
		assert(no >= 0);
		if (no >= fwend.m_children_no)
			return nullptr;

		// Calculate child size, with stretch logic
		auto size = fwend.m_children[no]->GetNaturalSize();

		switch (fwend.m_direction)
		{
		case T::Direction::Horizontal:
			if (fwend.m_children[no]->GetStretchedX() == true)
			{
				size.w += (available_size.w - fwend.m_natural_size.w) / static_cast<float>(fwend.m_stretched_childs);
			}
			if (fwend.m_children[no]->GetStretchedY() == true)
			{
				size.h = available_size.h;
			}
			break;
		case T::Direction::Vertical:
			if (fwend.m_children[no]->GetStretchedX() == true)
			{
				size.w = available_size.w;
			}
			if (fwend.m_children[no]->GetStretchedY() == true)
			{
				size.h += (available_size.h - fwend.m_natural_size.h) / static_cast<float>(fwend.m_stretched_childs);
			}
			break;
		}

		// Output delta
		if (layout_delta_out != nullptr)
		{
			const auto last = no - fwend.m_children_no;

			switch (fwend.m_direction)
			{
			case T::Direction::Horizontal: *layout_delta_out = {(last < 0) ? size.w + SPACING : 0.0f, 0.0f}; break;
			case T::Direction::Vertical: *layout_delta_out = {0.0f, (last < 0) ? size.h + SPACING : 0.0f}; break;
			}
		}

		// Output size
		if (size_out != nullptr)
			*size_out = size;

		return fwend.m_children[no];
	}
};

Ui::Widget* Ui::Box::GetChild(int no, Size available_size, Delta* layout_delta_out, Size* size_out)
{
	return Ui::BoxFriend::GetChild<Ui::Box>(*this, no, available_size, layout_delta_out, size_out);
}

const Ui::Widget* Ui::Box::GetChild(int no, Size available_size, Delta* layout_delta_out, Size* size_out) const
{
	return BoxFriend::GetChild<const Ui::Box>(*this, no, available_size, layout_delta_out, size_out);
}

int Ui::Box::AppendChild(Widget* widget)
{
	assert(widget != nullptr);
	if (m_children_no == MAX_CHILDREN)
		return MAX_CHILDREN;

	m_children[m_children_no++] = widget;
	MakeItDirty();

	return m_children_no - 1;
}

const char* Ui::Box::GetType() const
{
	return "Box";
}

const char* Ui::Box::GetPrintableInformation() const
{
	switch (m_direction)
	{
	case Direction::Horizontal: return "Horizontal";
	case Direction::Vertical: return "Vertical";
	}
}

Size Ui::Box::UpdateLayout()
{
#ifdef I_DO_NOT_WANT_MY_STACK_TO_BE_DESTROYED
	if (m_dirty == false)
		return m_natural_size;
#endif

	// Re-calculate layout
	DEBUGPRINT("%p Box::UpdateLayout()\n", reinterpret_cast<void*>(this));

	m_natural_size = {};
	m_stretched_childs = 0;

	switch (m_direction)
	{
	case Direction::Horizontal:
		for (int i = 0; i < m_children_no; i += 1)
		{
			const auto size = m_children[i]->UpdateLayout();
			m_natural_size.w += size.w + SPACING;
			m_natural_size.h = Max(m_natural_size.h, size.h);

			m_stretched_childs += m_children[i]->GetStretchedX();
		}
		m_natural_size.w -= SPACING;
		break;
	case Direction::Vertical:
		for (int i = 0; i < m_children_no; i += 1)
		{
			const auto size = m_children[i]->UpdateLayout();
			m_natural_size.w = Max(m_natural_size.w, size.w);
			m_natural_size.h += size.h + SPACING;

			m_stretched_childs += m_children[i]->GetStretchedY();
		}
		m_natural_size.h -= SPACING;
		break;
	}

	m_dirty = false;
	return m_natural_size;
}

Ui::Box* Ui::Box::SetStretch(bool x, bool y)
{
	Widget::SetStretch(x, y);
	return this;
}


// ================================
// HBox
// ================================

Ui::HBox* Ui::HBox::Create(Container* parent)
{
	assert(parent != nullptr);
	return new HBox(parent, nullptr);
}

Ui::HBox* Ui::HBox::Create(Wrapper* parent)
{
	assert(parent != nullptr);
	return new HBox(nullptr, parent);
}


// ================================
// VBox
// ================================

Ui::VBox* Ui::VBox::Create(Container* parent)
{
	assert(parent != nullptr);
	return new VBox(parent, nullptr);
}

Ui::VBox* Ui::VBox::Create(Wrapper* parent)
{
	assert(parent != nullptr);
	return new VBox(nullptr, parent);
}


// ================================
// Text
// ================================

Ui::Text* Ui::Text::Create(Container* parent, const char* text)
{
	assert(parent != nullptr);
	return new Text(parent, nullptr, text);
}

Ui::Text* Ui::Text::Create(Wrapper* parent, const char* text)
{
	assert(parent != nullptr);
	return new Text(nullptr, parent, text);
}

Ui::Text::Text(Container* container_parent, Wrapper* wrapper_parent, const char* text)
    : Widget(container_parent, wrapper_parent)
{
	assert(container_parent != nullptr || wrapper_parent != nullptr);
	m_text = text;
	m_font = Font::Normal;
}

int Ui::Text::GetChildrenNo() const
{
	return 0;
}

Ui::Widget* Ui::Text::GetChild(int no, Size available_size, Delta* layout_delta_out, Size* size_out)
{
	(void)no;
	(void)available_size;
	(void)layout_delta_out;
	(void)size_out;
	return nullptr;
}

const Ui::Widget* Ui::Text::GetChild(int no, Size available_size, Delta* layout_delta_out, Size* size_out) const
{
	(void)no;
	(void)available_size;
	(void)layout_delta_out;
	(void)size_out;
	return nullptr;
}

const char* Ui::Text::GetType() const
{
	return "Text";
}

const char* Ui::Text::GetPrintableInformation() const
{
	return m_text;
}


static constexpr float MARGIN = 1.0f / 2.0f; // TODO, hardcoded for now

Size Ui::Text::UpdateLayout()
{
#ifdef I_DO_NOT_WANT_MY_STACK_TO_BE_DESTROYED
	if (m_dirty == false)
		return m_natural_size;
#endif

	// Re-calculate layout
	DEBUGPRINT("%p Text::UpdateLayout()\n", reinterpret_cast<void*>(this));

	if (m_text != nullptr)
		m_natural_size = m_parent->GetDrawApi()->GetTextSize(m_font, m_text);
	else
		m_natural_size = {};

	m_natural_size.w += MARGIN * 2.0f;
	m_natural_size.h += MARGIN * 2.0f;

	m_dirty = false;
	return m_natural_size;
}

void Ui::Text::Draw(Rect allowed_area) const
{
	if (m_text == nullptr)
		return;

	allowed_area.pos.x += MARGIN;
	allowed_area.pos.y += MARGIN;
	m_parent->GetDrawApi()->DrawText(m_font, allowed_area.pos, m_text, COLOUR_BLACK);
}

Ui::Text* Ui::Text::SetFont(Font font)
{
	m_font = font;
	return this;
}


// ================================
// Button
// ================================

Ui::Button* Ui::Button::Create(Container* parent, const char* text)
{
	assert(parent != nullptr);
	return new Button(parent, nullptr, text);
}

Ui::Button* Ui::Button::Create(Wrapper* parent, const char* text)
{
	assert(parent != nullptr);
	return new Button(nullptr, parent, text);
}

Ui::Button::Button(Container* container_parent, Wrapper* wrapper_parent, const char* text)
    : Wrapper(container_parent, wrapper_parent)
{
	assert(container_parent != nullptr || wrapper_parent != nullptr);

	if (text != nullptr)
		Text::Create(this, text); // Calls SetChild() already
	else
		m_content = nullptr;
}

Ui::Button::~Button()
{
	if (m_content != nullptr)
		delete m_content;
}

const char* Ui::Button::GetType() const
{
	return "Button";
}

Size Ui::Button::UpdateLayout()
{
#ifdef I_DO_NOT_WANT_MY_STACK_TO_BE_DESTROYED
	if (m_dirty == false)
		return m_natural_size;
#endif

	// Re-calculate layout
	DEBUGPRINT("%p Button::UpdateLayout()\n", reinterpret_cast<void*>(this));

	if (m_content != nullptr)
		m_natural_size = m_content->UpdateLayout();
	else
		m_natural_size = {};

	m_dirty = false;
	return m_natural_size;
}

void Ui::Button::Draw(Rect allowed_area) const
{
	m_parent->GetDrawApi()->Draw3dBevel(allowed_area);
}

Ui::Button* Ui::Button::SetStretch(bool x, bool y)
{
	Widget::SetStretch(x, y);
	return this;
}


// ================================
// Widget/Container/Wrapper
// ================================

Ui::Widget::Widget(Container* container_parent, Wrapper* wrapper_parent)
{
	m_parent = nullptr; // Just because Window widget

	if (container_parent != nullptr)
	{
		m_parent = container_parent;
		container_parent->AppendChild(this);
	}
	else if (wrapper_parent != nullptr)
	{
		m_parent = wrapper_parent;
		wrapper_parent->SetChild(this);
	}

	MakeItDirty();

	m_stretch_x = false;
	m_stretch_y = false;
}

Ui::Container::Container(Container* container_parent, Wrapper* wrapper_parent) //
    : Widget(container_parent, wrapper_parent)
{
}

Ui::Wrapper::Wrapper(Container* container_parent, Wrapper* wrapper_parent) //
    : Widget(container_parent, wrapper_parent)
{
}


Ui::Widget* Ui::Wrapper::GetChild(int no, Size available_size, Delta* layout_delta_out, Size* size_out)
{
	(void)no;
	(void)available_size;
	if (layout_delta_out != nullptr)
		*layout_delta_out = {};
	if (size_out != nullptr)
		*size_out = GetNaturalSize();
	return m_content;
}

const Ui::Widget* Ui::Wrapper::GetChild(int no, Size available_size, Delta* layout_delta_out, Size* size_out) const
{
	(void)no;
	(void)available_size;

	// Calculate content size, with stretch logic
	auto size = m_content->GetNaturalSize();

	if (m_content->GetStretchedX() == true)
		size.w = available_size.w;

	if (m_content->GetStretchedY() == true)
		size.h = available_size.h;

	// Output delta and size
	if (layout_delta_out != nullptr)
		*layout_delta_out = {};
	if (size_out != nullptr)
		*size_out = size;

	return m_content;
}

void Ui::Wrapper::SetChild(Widget* widget)
{
	assert(widget != nullptr);
	m_content = widget;
	MakeItDirty();
}
