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

Ui::Widget* Ui::Window::GetChild(int no, Delta* layout_delta_out, Size* size_out)
{
	(void)no;
	if (layout_delta_out != nullptr)
		*layout_delta_out = {};
	if (size_out != nullptr)
		*size_out = GetNaturalSize();
	return m_content;
}

const Ui::Widget* Ui::Window::GetChild(int no, Delta* layout_delta_out, Size* size_out) const
{
	(void)no;
	if (layout_delta_out != nullptr)
		*layout_delta_out = {};
	if (size_out != nullptr)
		*size_out = GetNaturalSize();
	return m_content;
}

void Ui::Window::SetChild(Widget* widget)
{
	assert(widget != nullptr);
	m_content = widget;
	MakeItDirty();
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

void Ui::Window::Draw(Position pos) const
{
	m_draw_api->Draw3dBevel({pos, m_natural_size});
}

Ui::DrawAPI* Ui::Window::GetDrawApi() const
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

int Ui::Box::GetChildrenNo() const
{
	return m_children_no;
}

static constexpr float SPACING = 0.2f; // TODO, hardcoded for now

class Ui::BoxFriend
{
  public:
	template <typename T> static Ui::Widget* GetChild(T& fwend, int no, Delta* layout_delta_out, Size* size_out)
	{
		assert(no >= 0);
		if (no >= fwend.m_children_no)
			return nullptr;

		if (layout_delta_out != nullptr)
		{
			const auto size = fwend.m_children[no]->GetNaturalSize();
			const auto last = no - fwend.m_children_no;

			if (size_out != nullptr)
				*size_out = size;

			switch (fwend.m_direction)
			{
			case T::Direction::Horizontal: *layout_delta_out = {(last < 0) ? size.w + SPACING : 0.0f, 0.0f}; break;
			case T::Direction::Vertical: *layout_delta_out = {0.0f, (last < 0) ? size.h + SPACING : 0.0f}; break;
			}
		}

		return fwend.m_children[no];
	}
};

Ui::Widget* Ui::Box::GetChild(int no, Delta* layout_delta_out, Size* size_out)
{
	return Ui::BoxFriend::GetChild<Ui::Box>(*this, no, layout_delta_out, size_out);
}

const Ui::Widget* Ui::Box::GetChild(int no, Delta* layout_delta_out, Size* size_out) const
{
	return BoxFriend::GetChild<const Ui::Box>(*this, no, layout_delta_out, size_out);
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
	switch (m_direction)
	{
	case Direction::Horizontal:
		for (int i = 0; i < m_children_no; i += 1)
		{
			const auto size = m_children[i]->UpdateLayout();
			m_natural_size.w += size.w + SPACING;
			m_natural_size.h = Max(m_natural_size.h, size.h);
		}
		m_natural_size.w -= SPACING;
		break;
	case Direction::Vertical:
		for (int i = 0; i < m_children_no; i += 1)
		{
			const auto size = m_children[i]->UpdateLayout();
			m_natural_size.w = Max(m_natural_size.w, size.w);
			m_natural_size.h += size.h + SPACING;
		}
		m_natural_size.h -= SPACING;
		break;
	}

	m_dirty = false;
	return m_natural_size;
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
}

int Ui::Text::GetChildrenNo() const
{
	return 0;
}

Ui::Widget* Ui::Text::GetChild(int no, Delta* layout_delta_out, Size* size_out)
{
	(void)no;
	(void)layout_delta_out;
	(void)size_out;
	return nullptr;
}

const Ui::Widget* Ui::Text::GetChild(int no, Delta* layout_delta_out, Size* size_out) const
{
	(void)no;
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
		m_natural_size = m_parent->GetDrawApi()->GetTextSize(m_text);
	else
		m_natural_size = {};

	m_natural_size.w += MARGIN * 2.0f;
	m_natural_size.h += MARGIN * 2.0f;

	m_dirty = false;
	return m_natural_size;
}

void Ui::Text::Draw(Position pos) const
{
	if (m_text == nullptr)
		return;

	pos.x += MARGIN;
	pos.y += MARGIN;

	m_parent->GetDrawApi()->DrawText(pos, m_text);
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

Ui::Widget* Ui::Button::GetChild(int no, Delta* layout_delta_out, Size* size_out)
{
	(void)no;
	if (layout_delta_out != nullptr)
		*layout_delta_out = {}; // Valid only for containers with more than one child...
	if (size_out != nullptr)
		*size_out = GetNaturalSize();
	return m_content;
}

const Ui::Widget* Ui::Button::GetChild(int no, Delta* layout_delta_out, Size* size_out) const
{
	(void)no;
	if (layout_delta_out != nullptr)
		*layout_delta_out = {}; // ... as it starts affecting from the second child
	if (size_out != nullptr)
		*size_out = GetNaturalSize();
	return m_content;
}

void Ui::Button::SetChild(Widget* widget)
{
	assert(widget != nullptr);
	m_content = widget;
	MakeItDirty();
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

void Ui::Button::Draw(Position pos) const
{
	m_parent->GetDrawApi()->Draw3dBevel({pos, m_natural_size});
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
}

Ui::Container::Container(Container* container_parent, Wrapper* wrapper_parent) //
    : Widget(container_parent, wrapper_parent)
{
}

Ui::Wrapper::Wrapper(Container* container_parent, Wrapper* wrapper_parent) //
    : Widget(container_parent, wrapper_parent)
{
}
