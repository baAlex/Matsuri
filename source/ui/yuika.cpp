/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "yuika.hpp"
#include <assert.h>

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


#if defined(__clang__) || defined(__GNUC__)
static int sFindLastSet(uint32_t v) noexcept
{
	return v == 0 ? 0 : 32 - __builtin_clz(v);
}
#elif defined(_MSC_VER)
#include <intrin.h>
static int sFindLastSet(uint32_t v) noexcept
{
	unsigned long index;
	return _BitScanReverse(&index, v) ? static_cast<int>(index + 1) : 0;
}
#else
static int sClz(uint32_t x) noexcept
{
	// https://blog.stephencleary.com/2010/10/implementing-gccs-builtin-functions.html
	int n = 32;
	uint32_t y;

	// clang-format off
	y = x >>16; if (y != 0) {n = n -16; x = y;}
	y = x >> 8; if (y != 0) {n = n - 8; x = y;}
	y = x >> 4; if (y != 0) {n = n - 4; x = y;}
	y = x >> 2; if (y != 0) {n = n - 2; x = y;}
	y = x >> 1; if (y != 0) return n - 2;
	// clang-format on

	return n - static_cast<int>(x);
}
static int sFindLastSet(uint32_t v) noexcept
{
	return v == 0 ? 0 : 32 - sClz(v);
}
#endif


void yuika::Screen::Initialise(uint32_t r_mask, uint32_t g_mask, uint32_t b_mask)
{
	m_size = {1, 1};
	m_out = &m_dummy;

	auto set_colour = [&](DrawApi::Colour colour, uint8_t r, uint8_t g, uint8_t b)
	{
		const int rl = sFindLastSet(r_mask);
		const int gl = sFindLastSet(g_mask);
		const int bl = sFindLastSet(b_mask);

		uint32_t* c = m_palette + static_cast<int>(colour);
		*c = 0;
		*c = *c | (static_cast<uint32_t>((rl >= 8) ? (r << (rl - 8)) : (r >> (8 - rl))) & r_mask);
		*c = *c | (static_cast<uint32_t>((gl >= 8) ? (g << (gl - 8)) : (g >> (8 - gl))) & g_mask);
		*c = *c | (static_cast<uint32_t>((bl >= 8) ? (b << (bl - 8)) : (b >> (8 - bl))) & b_mask);
	};

	set_colour(DrawApi::Colour::Black, 0x00, 0x00, 0x00);
	set_colour(DrawApi::Colour::White, 0xFF, 0xFF, 0xFF);
	set_colour(DrawApi::Colour::Red, 0xFF, 0x00, 0x00);
	set_colour(DrawApi::Colour::Green, 0x00, 0xFF, 0x00);
	set_colour(DrawApi::Colour::Blue, 0x00, 0x00, 0xFF);

	set_colour(DrawApi::Colour::Background, 212, 208, 200); // Windows Me
	set_colour(DrawApi::Colour::BevelMid, 128, 128, 128);
	set_colour(DrawApi::Colour::BevelShadow, 64, 64, 64);

	m_root = new Root();
	m_root->SetStretch(true, true); // A good default value
}

void yuika::Screen::Deinitialise() noexcept
{
	delete m_root;
}


class yuika::ScreenFriend
{
  public:
	class DrawApiImplementation final : public DrawApi
	{
	  public:
		Screen* fwend;
		struct Rect clickable_area;

		void SetClickableArea(Rect rect) noexcept override
		{
			clickable_area = rect; // If already set, we overwrite last one (TODO, validate and clamp area)
		}

		void DrawRectangle(Colour colour, Rect rect) noexcept override
		{
			const int x1 = Clamp(rect.pos.x, 0, fwend->m_size.w);
			const int y1 = Clamp(rect.pos.y, 0, fwend->m_size.h);
			rect.size.w = (Clamp(rect.pos.x + rect.size.w, 0, fwend->m_size.w) - x1);
			rect.size.h = (Clamp(rect.pos.y + rect.size.h, 0, fwend->m_size.h) - y1) * fwend->m_size.w;

			uint32_t* out = fwend->m_out + static_cast<size_t>(x1 + y1 * fwend->m_size.w);
			for (uint32_t* row = out; row < out + rect.size.h; row += static_cast<size_t>(fwend->m_size.w))
			{
				for (uint32_t* col = row; col < row + rect.size.w; col += 1)
				{
					*col = fwend->m_palette[static_cast<int>(colour)];
				}
			}
		}

		void Draw3dBevel(Rect rect, BevelStyle style) noexcept override
		{
			if (style == BevelStyle::Inset)
			{
				DrawRectangle(Colour::BevelMid, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(Colour::BevelMid, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(Colour::BevelLight, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(Colour::BevelLight, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});
			}
			else
			{
				DrawRectangle(Colour::BevelLight, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(Colour::BevelLight, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(Colour::BevelShadow, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(Colour::BevelShadow, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});

				DrawRectangle(Colour::BevelMid, {{rect.pos.x + rect.size.w - 2, rect.pos.y + 1}, {1, rect.size.h - 2}});
				DrawRectangle(Colour::BevelMid, {{rect.pos.x + 1, rect.pos.y + rect.size.h - 2}, {rect.size.w - 2, 1}});
			}
		}
	};

	static void DrawWidgets(DrawApiImplementation& api)
	{
		// Non-recursive draw, it has the good feature of carry information while
		// descending the tree, like depth, drawable area, and more; also from our
		// end we can identify on which widget we are without asking it to widgets
		// themselves

		Screen::StackEntry* stack = api.fwend->m_stack;
		api.fwend->m_mini_tree_len = 0;

		size_t cursor = 0;
		stack[cursor++] = {0, api.fwend->m_root, nullptr, nullptr, Rect{{0, 0}, api.fwend->m_size}};

		while (cursor > 0)
		{
			Screen::StackEntry current = stack[--cursor]; // Yes, copy it

			// Draw
			api.clickable_area = {};
			current.widget->Draw(api, current.allowed_draw_area);

			// Update mini tree entry
			if (current.mini != nullptr)
				current.mini->clickable_area = api.clickable_area;

			if (current.parent_mini != nullptr)
				current.parent_mini->last_child = current.mini;

			// Iterate children
			if (cursor + current.widget->GetChildrenNo() >= Screen::STACK_LEN)
			{
				fprintf(stderr, "Too many widgets\n"); // TODO, use a exception
				return;
			}

			cursor += current.widget->GetChildrenNo();
			for (size_t i = 0; i < current.widget->GetChildrenNo(); i += 1)
			{
				const auto [child, delta, child_size] = current.widget->GetChild(i, current.allowed_draw_area.size);

				// Stack children,
				// just for iteration in this function
				stack[cursor - 1 - i] = {/* depth */ current.depth + 1,
				                         /* widget */ &child,
				                         /* parent_mini */ current.mini,
				                         /* mini */ &api.fwend->m_mini_tree[api.fwend->m_mini_tree_len],
				                         /* allowed_draw_area */ {current.allowed_draw_area.pos, child_size}};

				current.allowed_draw_area.pos.x += delta.x;
				current.allowed_draw_area.pos.y += delta.y;

				// Store children in mini tree,
				// other parts of the code will use it
				api.fwend->m_mini_tree[api.fwend->m_mini_tree_len] = {/* depth */ current.depth + 1,
				                                                      /* widget */ &child,
				                                                      /* last_child */ nullptr,
				                                                      /* parent_mini */ current.mini,
				                                                      /* clickable_area */ {},
				                                                      /* pressed */ false};
				api.fwend->m_mini_tree_len++;
			}
		}
	}
};


static constexpr bool UPDATE_NATURAL_SIZE_LIKE_CRAZY = false;
static constexpr bool DRAW_LIKE_CRAZY = false;

void yuika::Screen::Update(Size size, uint32_t* out)
{
	m_out = out;

	m_root->UpdateNaturalSize(); // [Recursion]

	if (m_size.w != size.w || m_size.h != size.h || DRAW_LIKE_CRAZY == true)
	{
		// Set stuff
		m_size = size;

		ScreenFriend::DrawApiImplementation api;
		api.fwend = this;

		api.DrawRectangle(DrawApi::Colour::Background, {{0, 0}, m_size});

		// Draw
		ScreenFriend::DrawWidgets(api);

		// Developers, developers, developers
		if (false)
		{
			for (MiniTreeEntry* m = m_mini_tree; m < m_mini_tree + m_mini_tree_len; m += 1)
			{
				printf("%p | ", reinterpret_cast<const void*>(m));
				for (size_t d = 0; d < m->depth - 1; d += 1)
					printf("   ");

				const Button* button = dynamic_cast<Button*>(m->widget);
				if (button != nullptr)
					printf("%s, \"%s\" (childs: %zu, last one: %p)\n", m->widget->GetType().cbegin(),
					       button->m_text.c_str(), m->widget->GetChildrenNo(),
					       reinterpret_cast<const void*>(m->last_child));
				else
					printf("%s (childs: %zu, last one: %p)\n", m->widget->GetType().cbegin(),
					       m->widget->GetChildrenNo(), reinterpret_cast<const void*>(m->last_child));
			}
		}
	}
}

void yuika::Screen::MouseEvent(MouseGesture gesture, Position cursor_pos) // TODO, lot of copy-paste
{
	// https://developer.mozilla.org/en-US/docs/Learn_web_development/Core/Scripting/Event_bubbling
	assert(gesture != MouseGesture::Click);

	if (gesture == MouseGesture::Press)
	{
		// Capture phase
		MiniTreeEntry* next = m_mini_tree;
		MiniTreeEntry* current = nullptr;

		while (next != nullptr)
		{
			current = next;

			// Developers, developers, developers
			if (false)
			{
				printf("Capturing | ");
				for (size_t d = 0; d < current->depth - 1; d += 1)
					printf("   ");

				const Button* button = dynamic_cast<Button*>(current->widget);
				if (button != nullptr)
					printf("%p, %s, \"%s\"\n", reinterpret_cast<const void*>(current->widget),
					       current->widget->GetType().cbegin(), button->m_text.c_str());
				else
					printf("%p, %s\n", reinterpret_cast<const void*>(current->widget),
					       current->widget->GetType().cbegin());
			}

			// Send event
			current->pressed = true;
			if (current->widget->OnMouseCapturing(MouseGesture::Press, cursor_pos) == EventPropagation::StopIt)
				break;

			// Go down, iterate children
			next = nullptr;

			MiniTreeEntry* child = current->last_child;
			for (size_t i = 0; i < current->widget->GetChildrenNo(); i += 1, child -= 1)
			{
				if (child->clickable_area.size.w <= 0 || child->clickable_area.size.h <= 0)
					continue;

				if (cursor_pos.x >= child->clickable_area.pos.x && cursor_pos.y >= child->clickable_area.pos.y &&
				    cursor_pos.x < child->clickable_area.pos.x + child->clickable_area.size.w &&
				    cursor_pos.y < child->clickable_area.pos.y + child->clickable_area.size.h)
				{
					next = child;
					break;
				}
			}
		}

		// Bubble phase
		Widget* target = current->widget;
		next = current;

		while (next != nullptr)
		{
			// Developers, developers, developers
			if (false)
			{
				printf("Bubbling  | ");
				for (size_t d = 0; d < next->depth - 1; d += 1)
					printf("   ");

				const Button* button = dynamic_cast<Button*>(next->widget);
				if (button != nullptr)
					printf("%p, %s, \"%s\"\n", reinterpret_cast<const void*>(next->widget),
					       next->widget->GetType().cbegin(), button->m_text.c_str());
				else
					printf("%p, %s\n", reinterpret_cast<const void*>(next->widget), next->widget->GetType().cbegin());
			}

			// Send event
			if (next->widget->OnMouseBubbling(MouseGesture::Press, cursor_pos, *target) == EventPropagation::StopIt)
				break;

			// Go up
			next = next->parent_mini;
		}
	}
	else if (gesture == MouseGesture::Release)
	{
		for (MiniTreeEntry* m = m_mini_tree; m < m_mini_tree + m_mini_tree_len; m += 1)
		{
			if (m->pressed == false)
				continue;

			// """Capture""" phase
			m->pressed = false;
			m->widget->OnMouseCapturing(MouseGesture::Release, cursor_pos); // I'm ignoring if pass or not the event

			if (cursor_pos.x >= m->clickable_area.pos.x && cursor_pos.y >= m->clickable_area.pos.y &&
			    cursor_pos.x < m->clickable_area.pos.x + m->clickable_area.size.w &&
			    cursor_pos.y < m->clickable_area.pos.y + m->clickable_area.size.h)
			{
				m->widget->OnMouseCapturing(MouseGesture::Click, cursor_pos); // Same
			}

			// Bubble phase
			MiniTreeEntry* next = m;
			while (next != nullptr)
			{
				if (next->widget->OnMouseBubbling(MouseGesture::Release, cursor_pos, *m->widget) ==
				    EventPropagation::StopIt)
					break;

				next = next->parent_mini;
			}

			// Click event, bubble phase
			next = m;
			while (next != nullptr)
			{
				if (cursor_pos.x >= next->clickable_area.pos.x && cursor_pos.y >= next->clickable_area.pos.y &&
				    cursor_pos.x < next->clickable_area.pos.x + next->clickable_area.size.w &&
				    cursor_pos.y < next->clickable_area.pos.y + next->clickable_area.size.h)
				{
					if (next->widget->OnMouseBubbling(MouseGesture::Click, cursor_pos, *m->widget) ==
					    EventPropagation::StopIt)
						break;
				}

				next = next->parent_mini;
			}
		}
	}
}


yuika::Wrapper& yuika::Screen::GetRoot()
{
	return *m_root;
}

std::string_view yuika::Screen::Root::GetType() const
{
	return "Root";
}


// ############################


yuika::Widget::Widget()
{
	m_stretch_x = false; // Is not possible to set bitfields on headers
	m_stretch_y = false; // (a C++ quirk)
	m_natural_size_updated = false;
	m_id = "";
}

yuika::Size yuika::Widget::GetNaturalSize() const
{
	return m_natural_size;
}

void yuika::Widget::SetId(const char* id)
{
	m_id = id;
}

const char* yuika::Widget::GetId() const
{
	return m_id;
}

yuika::Widget& yuika::Widget::SetStretch(bool x, bool y)
{
	m_stretch_x = x;
	m_stretch_y = y;
	return *this;
}

bool yuika::Widget::GetStretchX() const
{
	return m_stretch_x;
}

bool yuika::Widget::GetStretchY() const
{
	return m_stretch_y;
}

yuika::Size yuika::Widget::GetSize(Size available_size) const
{
	Size size = m_natural_size;
	size.w = (GetStretchX() == true || size.w > available_size.w) ? available_size.w : size.w;
	size.h = (GetStretchY() == true || size.h > available_size.h) ? available_size.h : size.h;
	return size;
}

void yuika::Widget::Draw(DrawApi& api, Rect allowed_draw_area) const
{
	// There are less surprises by setting a clickable area by default,
	// it still can be overridden if more fine control is needed
	api.SetClickableArea({allowed_draw_area.pos, GetSize(allowed_draw_area.size)});
}

void yuika::Widget::SetReceivingEvents(uint32_t events)
{
	m_receiving_events = events;
}

uint32_t yuika::Widget::GetReceivingEvents() const
{
	return m_receiving_events;
}

yuika::EventPropagation yuika::Widget::OnMouseCapturing(MouseGesture, Position)
{
	return EventPropagation::KeepPassingIt;
}

yuika::EventPropagation yuika::Widget::OnMouseBubbling(MouseGesture, Position, Widget&)
{
	return EventPropagation::KeepPassingIt;
}


// ############################


size_t yuika::Wrapper::GetChildrenNo() const
{
	if (m_content == nullptr)
		return 0;
	return 1;
}

yuika::Widget& yuika::Wrapper::SetChild(std::unique_ptr<Widget> widget)
{
	m_content = std::move(widget);
	m_natural_size_updated = false;
	return *m_content;
}

yuika::Widget::ChildGet yuika::Wrapper::GetChild(size_t, Size available_size)
{
	if (m_content == nullptr)
		throw 1; // TODO
	return {*m_content, {}, GetSize(available_size)};
}

const yuika::Widget::ChildGet yuika::Wrapper::GetChild(size_t, Size available_size) const
{
	if (m_content == nullptr)
		throw 1; // TODO
	return {*m_content, {}, GetSize(available_size)};
}

yuika::Widget& yuika::Wrapper::GetChild(size_t)
{
	if (m_content == nullptr)
		throw 1; // TODO
	return *m_content;
}

const yuika::Widget& yuika::Wrapper::GetChild(size_t) const
{
	if (m_content == nullptr)
		throw 1; // TODO
	return *m_content;
}

yuika::Size yuika::Wrapper::UpdateNaturalSize()
{
	if (m_natural_size_updated == false || UPDATE_NATURAL_SIZE_LIKE_CRAZY == true)
	{
		// DEBUGPRINT("%u | yuika::Wrapper::UpdateNaturalSize\n", s_frame);
		m_natural_size_updated = true;
		m_natural_size = (m_content != nullptr) ? m_content->UpdateNaturalSize() : Size{30, 30}; // [Recursion]
	}

	return m_natural_size;
}


// ############################


yuika::Box::Box(Direction direction) : Container()
{
	m_direction = direction;
	m_non_stretch_size = {};
}

size_t yuika::Box::GetChildrenNo() const
{
	return m_children.size();
}

yuika::Widget& yuika::Box::AddChild(std::unique_ptr<Widget> widget)
{
	m_children.push_back(std::move(widget));
	m_natural_size_updated = false;
	return *m_children.back();
}

class yuika::BoxFriend
{
  public:
	template <typename T> static Widget::ChildGet GetChild(T& fwend, size_t no, Size available_size)
	{
		Delta delta;
		Size size;

		if (no >= fwend.m_children.size())
			throw 1; // TODO

		auto& child = fwend.m_children.at(no);

		switch (fwend.m_direction)
		{
		case Box::Direction::Horizontal:
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
		case Box::Direction::Vertical:
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

yuika::Widget::ChildGet yuika::Box::GetChild(size_t no, Size available_size)
{
	return BoxFriend::GetChild(*this, no, available_size);
}

const yuika::Widget::ChildGet yuika::Box::GetChild(size_t no, Size available_size) const
{
	return BoxFriend::GetChild(*this, no, available_size);
}

yuika::Widget& yuika::Box::GetChild(size_t no)
{
	if (no >= m_children.size())
		throw 1; // TODO
	return *m_children.at(no);
}

const yuika::Widget& yuika::Box::GetChild(size_t no) const
{
	if (no >= m_children.size())
		throw 1; // TODO
	return *m_children.at(no);
}

std::string_view yuika::Box::GetType() const
{
	return "Box";
}

yuika::Size yuika::Box::UpdateNaturalSize()
{
	if (m_natural_size_updated == false || UPDATE_NATURAL_SIZE_LIKE_CRAZY == true)
	{
		// DEBUGPRINT("%u | yuika::Box::UpdateNaturalSize\n", s_frame);
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

yuika::HBox::HBox() : Box(Direction::Horizontal) {}
yuika::VBox::VBox() : Box(Direction::Vertical) {}


// ############################


yuika::Button::Button(std::string text) : Wrapper()
{
	m_text = std::move(text); // TODO, it has to create a label
}

std::string_view yuika::Button::GetType() const
{
	return "Button";
}

void yuika::Button::Draw(DrawApi& api, Rect allowed_draw_area) const
{
	// DEBUGPRINT("%u | yuika::Button::Draw\n", s_frame);
	api.SetClickableArea({allowed_draw_area.pos, GetSize(allowed_draw_area.size)});
	api.Draw3dBevel({allowed_draw_area.pos, GetSize(allowed_draw_area.size)}, DrawApi::BevelStyle::Outset);
}
