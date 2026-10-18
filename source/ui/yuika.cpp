/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include "yuika.hpp"

#include <algorithm>
#include <assert.h>


#include "sans-serif.txt"

// clang-format off
#define EXCEPT(code, exception_object) if( code ) { throw (exception_object); }
// clang-format on


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

	m_root = new Root();
	m_root->SetStretch(true, true); // A good default value
	m_root->SetId("#__root");

	if ((m_font = reinterpret_cast<uint8_t*>(malloc(sizeof(uint8_t) * ATLAS_WIDTH * ATLAS_HEIGHT))) == nullptr)
		throw 1; // TODO

	m_masks[2] = r_mask;
	m_masks[1] = g_mask;
	m_masks[0] = b_mask;

	m_draw_valid = false;
	m_layout_valid = false;

	// SDF,
	// do fragment-shader work offline, as we are 2d
	for (size_t i = 0; i < ATLAS_WIDTH * ATLAS_HEIGHT; i += 1)
	{
		const auto min = static_cast<float>(128 - 8) / 255.0f;
		const auto max = static_cast<float>(128 + 8) / 255.0f;

		auto p = static_cast<float>(255 - ATLAS_DATA[i]) / 255.0f;

		p = (std::clamp(p, min, max) - min) / (max - min);

		// https://registry.khronos.org/OpenGL-Refpages/gl4/html/smoothstep.xhtml
		const float p2 = p * p * (3.0f - 2.0f * p); // I'm not sure, Valve uses it, but they
		// didn't take gamma in consideration, also, should I compensate it here since the
		// blitter has no idea what gamma is?.

		// Edit, it looks better, sharper. It's a balance with the hinting.

		p = (p2 + p) * 0.5f; // Edit2, a bit of both worlds

		m_font[i] = static_cast<uint8_t>(p * 255.0f);
	}
}

void yuika::Screen::Deinitialise() noexcept
{
	delete m_root;
	free(m_font);
}


class yuika::ScreenFriend
{
  public:
	class DrawApiImplementation final : public DrawApi
	{
	  public:
		Screen* fwend;
		struct Rect hit_area;

		Rect ClipRect(Rect rect)
		{
			const int x = std::clamp(rect.pos.x, 0, fwend->m_size.x);
			const int y = std::clamp(rect.pos.y, 0, fwend->m_size.y);
			rect.size.x = (std::clamp(rect.pos.x + rect.size.x, 0, fwend->m_size.x) - x);
			rect.size.y = (std::clamp(rect.pos.y + rect.size.y, 0, fwend->m_size.y) - y) * fwend->m_size.x;
			rect.pos.x = x;
			rect.pos.y = y;
			return rect;
		}

		void SetHitArea(Rect rect) noexcept override
		{
			hit_area = rect; // If already set, we overwrite last one (TODO, validate and clamp area)
		}

		void DrawRectangle(Colour colour, Rect rect) noexcept override
		{
			rect = ClipRect(rect);

			uint32_t* out = fwend->m_out + static_cast<size_t>(rect.pos.x + rect.pos.y * fwend->m_size.x);
			for (uint32_t* row = out; row < out + rect.size.y; row += static_cast<size_t>(fwend->m_size.x))
			{
				for (uint32_t* col = row; col < row + rect.size.x; col += 1)
				{
					*col = colour;
				}
			}
		}

		void DrawCheckerBoardRectangle(Colour colour, Rect rect) noexcept override
		{
			rect = ClipRect(rect);
			uint8_t kiki = 0;
			uint8_t boba = 0;

			uint32_t* out = fwend->m_out + static_cast<size_t>(rect.pos.x + rect.pos.y * fwend->m_size.x);
			for (uint32_t* row = out; row < out + rect.size.y; row += static_cast<size_t>(fwend->m_size.x))
			{
				for (uint32_t* col = row; col < row + rect.size.x; col += 1)
				{
					*col = (((kiki ^ boba) & 1) != 0) ? colour : *col;
					boba++;
				}
				kiki++;
				boba = 0;
			}
		}

		void DrawBorder(Rect rect, BorderStyle style, Colour colour) noexcept override
		{
			switch (style)
			{
			case BorderStyle::Inset:
				DrawRectangle(BEVEL_MID, {{rect.pos.x, rect.pos.y}, {rect.size.x - 1, 1}});
				DrawRectangle(BEVEL_MID, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.y - 1}});
				DrawRectangle(BEVEL_LIGHT, {{rect.pos.x + rect.size.x - 1, rect.pos.y}, {1, rect.size.y}});
				DrawRectangle(BEVEL_LIGHT, {{rect.pos.x, rect.pos.y + rect.size.y - 1}, {rect.size.x, 1}});
				break;
			case BorderStyle::Outset:
				DrawRectangle(BEVEL_LIGHT, {{rect.pos.x, rect.pos.y}, {rect.size.x - 1, 1}});
				DrawRectangle(BEVEL_LIGHT, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.y - 1}});
				DrawRectangle(BEVEL_SHADOW, {{rect.pos.x + rect.size.x - 1, rect.pos.y}, {1, rect.size.y}});
				DrawRectangle(BEVEL_SHADOW, {{rect.pos.x, rect.pos.y + rect.size.y - 1}, {rect.size.x, 1}});

				DrawRectangle(BEVEL_MID, {{rect.pos.x + rect.size.x - 2, rect.pos.y + 1}, {1, rect.size.y - 2}});
				DrawRectangle(BEVEL_MID, {{rect.pos.x + 1, rect.pos.y + rect.size.y - 2}, {rect.size.x - 2, 1}});
				break;
			case BorderStyle::Solid:
				DrawRectangle(colour, {{rect.pos.x, rect.pos.y}, {rect.size.x - 1, 1}});
				DrawRectangle(colour, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.y - 1}});
				DrawRectangle(colour, {{rect.pos.x + rect.size.x - 1, rect.pos.y}, {1, rect.size.y}});
				DrawRectangle(colour, {{rect.pos.x, rect.pos.y + rect.size.y - 1}, {rect.size.x, 1}});
				break;
			}
		}

		void DrawText(Colour colour, Vector pos, const char* text) noexcept override
		{
			auto xf = static_cast<float>(pos.x);
			auto yf = static_cast<float>(pos.y);

			for (const char* c = text; *c != 0x00; c += 1)
			{
				if (static_cast<size_t>(*c) < FIRST_CHARACTER_CODE ||
				    (static_cast<size_t>(*c) - FIRST_CHARACTER_CODE) >= CHARACTERS_NO)
					continue;

				const CharacterMetric* ch = CHARACTERS_METRICS + (static_cast<size_t>(*c) - FIRST_CHARACTER_CODE);

				const Rect rect = {{static_cast<int>(xf + ch->x_offset), static_cast<int>(yf + ch->y_offset)},
				                   {ch->width, ch->height}};
				xf += ch->advance;

				if (*c == ' ')
					continue;

				const auto clipped_rect = ClipRect(rect);
				const auto clip_diff_x = static_cast<size_t>(clipped_rect.pos.x - rect.pos.x);
				const auto clip_diff_y = static_cast<size_t>(clipped_rect.pos.y - rect.pos.y);

				const uint8_t* in_row =
				    fwend->m_font + (ch->atlas_x + clip_diff_x) + ATLAS_WIDTH * (ch->atlas_y + clip_diff_y);
				const uint32_t in_pitch = ATLAS_WIDTH;

				uint32_t* out =
				    fwend->m_out + static_cast<size_t>(clipped_rect.pos.x + clipped_rect.pos.y * fwend->m_size.x);
				for (uint32_t* row = out; row < out + clipped_rect.size.y; row += static_cast<size_t>(fwend->m_size.x))
				{
					const uint8_t* in_col = in_row;

					for (uint32_t* out_col = row; out_col < row + clipped_rect.size.x; out_col += 1)
					{
						const auto a = (static_cast<uint32_t>((*out_col >> 24) & 0xFF) * (*in_col) +
						                static_cast<uint32_t>((colour >> 24) & 0xFF) * (255 - *in_col)) /
						               255;
						const auto r = (static_cast<uint32_t>((*out_col >> 16) & 0xFF) * (*in_col) +
						                static_cast<uint32_t>((colour >> 16) & 0xFF) * (255 - *in_col)) /
						               255;
						const auto g = (static_cast<uint32_t>((*out_col >> 8) & 0xFF) * (*in_col) +
						                static_cast<uint32_t>((colour >> 8) & 0xFF) * (255 - *in_col)) /
						               255;
						const auto b = (static_cast<uint32_t>((*out_col >> 0) & 0xFF) * (*in_col) +
						                static_cast<uint32_t>((colour >> 0) & 0xFF) * (255 - *in_col)) /
						               255;

						*out_col = (b << 0) | (g << 8) | (r << 16) | (a << 24);

						in_col++;
					}

					in_row += in_pitch;
				}
			}
		}

		Vector TextSize(const char* text) override
		{
			float w = 0.0f;

			for (const char* c = text; *c != 0x00; c += 1)
			{
				if (static_cast<size_t>(*c) < FIRST_CHARACTER_CODE ||
				    (static_cast<size_t>(*c) - FIRST_CHARACTER_CODE) >= CHARACTERS_NO)
					continue;

				const CharacterMetric* ch = CHARACTERS_METRICS + (static_cast<size_t>(*c) - FIRST_CHARACTER_CODE);
				w += ch->advance;
			}

			return {static_cast<int>(w), static_cast<int>(FONT_HEIGHT)};
		}

		Vector Margin(const Widget& widget) override
		{
			// TODO, use a hashmap, using GetId() as key first, then GetClassId()
			if (widget.GetClassId() == "Text")
				return {18, 18};
			if (widget.GetClassId() == "VoiceFrame")
				return {16, 16};
			/*else if (strcmp(widget.GetClassId(), "box") == 0)
			    return {4, 4};
			else if (strcmp(widget.GetClassId(), "button") == 0)
			    return {2, 2};*/

			return {};
		}
	};

	static void DrawWidget(DrawApiImplementation& api, Widget* widget, Screen::MiniTreeEntry* mini,
	                       Rect allowed_draw_area)
	{
		// Screen::CreateMiniTree() is the full version of this function

		assert(mini->widget == widget);
		size_t cursor = 1;

		// This is a bit hacky, but we need to find where children's
		// mini tree entries start
		size_t mini_tree_index =
		    static_cast<size_t>(mini->last_child - api.fwend->m_mini_tree) - mini->widget->GetChildrenNo() + 1;

		// Add root first
		{
			Screen::StackEntry* stack_entry = api.fwend->m_stack + 0;
			stack_entry->widget = widget;
			stack_entry->allowed_draw_area = allowed_draw_area;
			stack_entry->mini = mini;
		}

		// Now widget as normal
		while (cursor > 0)
		{
			Screen::StackEntry stack_entry = api.fwend->m_stack[--cursor];

			// Draw widget
			api.hit_area = {};
			stack_entry.widget->OnDraw(api, stack_entry.allowed_draw_area);
			stack_entry.widget->SetValidation(Widget::Validation::Draw, true);

			// Update mini tree entry
			if (stack_entry.mini != nullptr)
			{
				assert(stack_entry.mini->widget == stack_entry.widget); // Did the hack work?
				stack_entry.mini->last_draw_at = stack_entry.allowed_draw_area;
				stack_entry.mini->hit_area = api.hit_area;
			}

			// Iterate children
			const size_t childs_no = stack_entry.widget->GetChildrenNo();
			if (cursor + childs_no >= Screen::STACK_LEN)
				throw 1; // TODO

			cursor += childs_no;
			for (size_t i = 0; i < childs_no; i += 1)
			{
				const auto [child, child_offset, child_size, next_offset] = stack_entry.widget->GetChildInLayout(
				    api, i, stack_entry.allowed_draw_area.size, stack_entry.allowed_draw_area.pos);

				const Vector child_margin = api.Margin(child);

				Screen::StackEntry* child_stack_entry = api.fwend->m_stack + cursor - 1 - i;

				// Stack children
				child_stack_entry->widget = &child;
				child_stack_entry->mini = api.fwend->m_mini_tree + mini_tree_index;
				child_stack_entry->allowed_draw_area = {
				    stack_entry.allowed_draw_area.pos + child_offset + child_margin / 2, child_size - child_margin};

				// Move draw cursor
				stack_entry.allowed_draw_area.pos += next_offset;

				// Next mini tree entry
				mini_tree_index++;
			}
		}
	}
};


static constexpr bool DRAW_LIKE_CRAZY = false;

void yuika::Screen::ColourConversion(uint32_t* in_out, Vector size, int stride) const
{
	if (m_masks[2] == 0x00FF0000 && m_masks[1] == 0x0000FF00 && m_masks[0] == 0x000000FF)
		return;

	// Disgusting but the auto-vectoriser is doing it! (and in a horizontal manner, which
	// makes sense since there is no shift for individual lanes; anyways it's 8 values
	// with AVX and 4 with SSE2)

	const int set[4] = {sFindLastSet(m_masks[0]), //
	                    sFindLastSet(m_masks[1]), //
	                    sFindLastSet(m_masks[2]), 0};

	const int ls[4] = {(set[0] >= 8) ? set[0] - 8 : 0, //
	                   (set[1] >= 8) ? set[1] - 8 : 0, //
	                   (set[2] >= 8) ? set[2] - 8 : 0, 0};
	const uint32_t lm[4] = {(set[0] >= 8) ? m_masks[0] : 0, //
	                        (set[1] >= 8) ? m_masks[1] : 0, //
	                        (set[2] >= 8) ? m_masks[2] : 0, 0};

	const int rs[4] = {(set[0] < 8) ? 8 - set[0] : 0, //
	                   (set[1] < 8) ? 8 - set[1] : 0, //
	                   (set[2] < 8) ? 8 - set[2] : 0, 0};
	const uint32_t rm[4] = {(set[0] < 8) ? m_masks[0] : 0, //
	                        (set[1] < 8) ? m_masks[1] : 0, //
	                        (set[2] < 8) ? m_masks[2] : 0, 0};

	for (uint32_t* row = in_out; row < in_out + static_cast<size_t>(stride * size.y); row += stride)
	{
		for (uint32_t* col = row; col < row + static_cast<size_t>(size.x); col += 1)
		{
			const auto a = static_cast<uint32_t>((*col >> 24) & 0xFF);
			const auto r = static_cast<uint32_t>((*col >> 16) & 0xFF);
			const auto g = static_cast<uint32_t>((*col >> 8) & 0xFF);
			const auto b = static_cast<uint32_t>((*col >> 0) & 0xFF);

			*col = ((b << ls[0]) & lm[0]) | ((g << ls[1]) & lm[1]) | //
			       ((r << ls[2]) & lm[2]) | ((a << ls[3]) & lm[3]) | //
			       ((b >> rs[0]) & rm[0]) | ((g >> rs[1]) & rm[1]) | //
			       ((r >> rs[2]) & rm[2]) | ((a >> rs[3]) & rm[3]);
		}
	}
}


void yuika::Screen::CreateMiniTree()
{
	// Non-recursive, it has the good feature of carry information while
	// descending the tree, like depth, drawable area, and more; also from our
	// end we can identify on which widget we are without asking it to widgets
	// themselves

	size_t cursor = 1;
	m_mini_tree_len = 1;

	// Add root first
	{
		Screen::StackEntry* stack_entry = m_stack + 0;
		stack_entry->widget = m_root;
		stack_entry->mini = m_mini_tree;
		stack_entry->parent_mini = nullptr;
		stack_entry->depth = 1;

		Screen::MiniTreeEntry* mini_entry = m_mini_tree + 0;
		mini_entry->widget = m_root;
		mini_entry->parent = nullptr;
		mini_entry->last_child = nullptr;
		mini_entry->depth = 1;
	}

	// Now widgets as normal
	while (cursor > 0)
	{
		Screen::StackEntry stack_entry = m_stack[--cursor];

		// Update mini tree entry,
		// fields that weren't know when mini tree entry was created
		if (stack_entry.parent_mini != nullptr)
			stack_entry.parent_mini->last_child = stack_entry.mini;

		// Iterate children
		const size_t childs_no = stack_entry.widget->GetChildrenNo();
		if (cursor + childs_no >= Screen::STACK_LEN)
			throw 1; // TODO

		cursor += childs_no;
		for (size_t i = 0; i < childs_no; i += 1)
		{
			Widget& child = stack_entry.widget->GetChild(i);

			Screen::StackEntry* child_stack_entry = m_stack + cursor - 1 - i; // TODO, clang-tidy has an issue with this

			// Stack children,
			// just for iteration in this function
			child_stack_entry->widget = &child;
			child_stack_entry->mini = m_mini_tree + m_mini_tree_len;
			child_stack_entry->parent_mini = stack_entry.mini;
			child_stack_entry->depth = stack_entry.depth + 1;

			// Store children in mini tree,
			// other parts of the code will use it
			child_stack_entry->mini->widget = &child;
			child_stack_entry->mini->parent = stack_entry.mini;
			child_stack_entry->mini->last_child = nullptr;
			child_stack_entry->mini->depth = stack_entry.depth + 1;

			// Allocate mini tree entry
			m_mini_tree_len++;
		}
	}

	// Developers, developers, developers
	if (false)
	{
		printf("\n### CreateMiniTree():\n\n");

		for (const MiniTreeEntry* m = m_mini_tree; m < m_mini_tree + m_mini_tree_len; m += 1)
		{
			printf("%p | ", reinterpret_cast<const void*>(m));
			for (int i = 0; i < m->depth; i += 1)
				printf("-");
			printf(" \"%s\" (childs: %zu, last one: %p)\n", m->widget->GetId().c_str(), m->widget->GetChildrenNo(),
			       reinterpret_cast<const void*>(m->last_child));
		}

		printf("\nWidgets: %zu\n", m_mini_tree_len);
	}
}


yuika::Rect yuika::Screen::Draw(Vector size, uint32_t* out)
{
	ScreenFriend::DrawApiImplementation draw_api;
	draw_api.fwend = this;

	// Is layout updated?
	if (m_layout_valid == false)
	{
		m_layout_valid = true;
		CreateMiniTree();

		// Call OnLayout(), in reverse starting from deepest childs, leafs.
		// The idea is to widgets use this opportunity to update their size,
		// and sizes depends on the size of their child, and the childs of
		// these childs. The idea of doing it in reverse is to avoid recursion.
		for (const MiniTreeEntry* m = m_mini_tree + m_mini_tree_len - 1; m >= m_mini_tree; m -= 1)
		{
			if (m->widget->GetValidation(Widget::Validation::Layout) == false)
			{
				m->widget->OnLayout(draw_api);
				m->widget->SetValidation(Widget::Validation::Layout, true);
			}
		}
	}

	// Do a full draw?
	if (m_size.x != size.x || m_size.y != size.y || DRAW_LIKE_CRAZY == true)
	{
		m_draw_valid = true;
		m_size = size;
		m_out = out;

		// Draw
#ifndef NDEBUG
		draw_api.DrawRectangle(DrawApi::PINK, {{0, 0}, m_size});
#else
		draw_api.DrawRectangle(DrawApi::BACKGROUND, {{0, 0}, m_size});
#endif

		ScreenFriend::DrawWidget(draw_api, m_root, m_mini_tree, Rect{{}, m_size});
		ColourConversion(out, m_size, m_size.x);

		// Done!
		return {{0, 0}, m_size};
	}

	// Partial draw?
	if (m_draw_valid == false)
	{
		m_draw_valid = true;
		Rect changed_area = Rect::Inverse();

		// TODO, here I should set "scissoring area = invalidated area" (a la' OpenGL),
		// (as soon I implement it). Btw, it's called "clipping rectangle" in Win32,
		// also Windows is doing the same, making the scissoring area the sames as
		// the invalidated area. (Petzold, Programming Windows, p78)

		for (MiniTreeEntry* i = m_mini_tree; i < m_mini_tree + m_mini_tree_len; i += 1)
		{
			// TODO, like most for loops iterating the mini tree, an obvious optimisation is to
			// keep different list/arrays/mini-trees, and not use the same one for everything

			if (i->widget->GetValidation(Widget::Validation::Draw) == true)
				continue;

#ifndef NDEBUG
			draw_api.DrawRectangle(DrawApi::PINK, i->last_draw_at);
#else
			draw_api.DrawRectangle(DrawApi::WHITE, i->last_draw_at);
#endif

			ScreenFriend::DrawWidget(draw_api, i->widget, i, i->last_draw_at);
			ColourConversion(out + static_cast<size_t>(i->last_draw_at.pos.x + m_size.x * i->last_draw_at.pos.y),
			                 i->last_draw_at.size, m_size.x);

			changed_area = i->last_draw_at.Union(changed_area);
		}

		// Done!
		return changed_area;
	}

	// Nothing was done
	return {};
}


void yuika::Screen::Bubble(MouseGesture gesture, Vector cursor_pos, MiniTreeEntry* target)
{
	// We do bubbling here
	// https://developer.mozilla.org/en-US/docs/Learn_web_development/Core/Scripting/Event_bubbling

	Widget* target_widget = target->widget;
	uint32_t cookie = target_widget->GetCookie();

	for (MiniTreeEntry* i = target; i != nullptr; i = i->parent)
	{
		const EventPropagation propagation = i->widget->OnMouse(gesture, cursor_pos, cookie, *target_widget);

		if (i->widget->GetValidation(Widget::Validation::Draw) == false)
			m_draw_valid = false;
		if (i->widget->GetValidation(Widget::Validation::Layout) == false)
			m_layout_valid = false;

		if (propagation == EventPropagation::StopIt)
			break;
		else if (propagation == EventPropagation::TakePlaceAsTarget)
			target_widget = i->widget;
	}
}

void yuika::Screen::MousePress(Vector cursor_pos)
{
	// Find target, going down the mini tree
	MiniTreeEntry* target = nullptr;
	for (MiniTreeEntry* next = m_mini_tree; next != nullptr;)
	{
		target = next;
		target->pressed_indirectly = true;

		// Iterate children
		next = nullptr;
		MiniTreeEntry* child = target->last_child;
		for (size_t child_no = 0; child_no < target->widget->GetChildrenNo(); child_no += 1, child -= 1)
		{
			if (child->hit_area.IsPointInside(cursor_pos) == true)
			{
				next = child;
				break;
			}
		}
	}

	// Press event
	target->pressed_as_target = true;
	Bubble(MouseGesture::Press, cursor_pos, target);
}

void yuika::Screen::MouseRelease(Vector cursor_pos)
{
	for (MiniTreeEntry* target = m_mini_tree; target < m_mini_tree + m_mini_tree_len; target += 1)
	{
		// Release event
		if (target->pressed_as_target ==
		    true) // TODO, keep a list of what was pressed, to avoid iterate the entire thing
		{
			target->pressed_as_target = false;
			Bubble(MouseGesture::Release, cursor_pos, target);
		}

		// Click event
		if (target->pressed_indirectly == true) // TODO, same
		{
			target->pressed_indirectly = false;
			if (target->hit_area.IsPointInside(cursor_pos) == true)
				Bubble(MouseGesture::Click, cursor_pos, target);
		}
	}
}

void yuika::Screen::MouseMoves(Vector cursor_pos)
{
	// Leaves event
	for (MiniTreeEntry* target = m_mini_tree; target < m_mini_tree + m_mini_tree_len; target += 1)
	{
		if (target->cursor_inside == true && target->hit_area.IsPointInside(cursor_pos) == false)
		{
			target->cursor_inside = false;
			Bubble(MouseGesture::Leaves, cursor_pos, target);
		}
	}

	// Iterate tree (copy paste from MousePress)
	MiniTreeEntry* target = nullptr;
	for (MiniTreeEntry* next = m_mini_tree; next != nullptr;)
	{
		target = next;

		// Enters event
		if (target->cursor_inside == false)
		{
			target->cursor_inside = true;
			Bubble(MouseGesture::Enters, cursor_pos, target);
		}

		// Iterate children
		next = nullptr;
		MiniTreeEntry* child = target->last_child;
		for (size_t child_no = 0; child_no < target->widget->GetChildrenNo(); child_no += 1, child -= 1)
		{
			if (child->hit_area.IsPointInside(cursor_pos) == true)
			{
				next = child;
				break;
			}
		}
	}
}


yuika::Frame& yuika::Screen::GetRoot()
{
	return *m_root;
}


// ############################


yuika::Widget::Widget()
{
	m_stretch_x = false; // Is not possible to set bitfields on headers
	m_stretch_y = false; // (a C++ quirk)
	m_draw_valid = false;
	m_layout_valid = false;
	m_cookie = 0;
}

yuika::Vector yuika::Widget::GetMinimumSize() const
{
	return m_minimum_size;
}

yuika::Widget& yuika::Widget::SetId(const std::string& id)
{
	m_id = id;
	return *this;
}

yuika::Widget& yuika::Widget::SetClassId(const std::string& class_id)
{
	m_class_id = class_id;
	return *this;
}

yuika::Widget& yuika::Widget::SetCookie(uint32_t cookie)
{
	m_cookie = cookie;
	return *this;
}

yuika::Widget& yuika::Widget::SetValidation(Widget::Validation v, bool value)
{
	switch (v)
	{
	case Widget::Validation::Draw: m_draw_valid = value; break;
	case Widget::Validation::Layout: m_layout_valid = value; break;
	}

	return *this;
}

bool yuika::Widget::GetValidation(Widget::Validation v) const
{
	switch (v)
	{
	case Widget::Validation::Draw: return m_draw_valid;
	case Widget::Validation::Layout: return m_layout_valid;
	}
}

const std::string& yuika::Widget::GetId() const
{
	return m_id;
}

const std::string& yuika::Widget::GetClassId() const
{
	return m_class_id;
}

uint32_t yuika::Widget::GetCookie() const
{
	return m_cookie;
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

yuika::Vector yuika::Widget::GetSize(Vector available_size) const
{
	Vector size = m_minimum_size;
	size.x = (GetStretchX() == true || size.x > available_size.x) ? available_size.x : size.x;
	size.y = (GetStretchY() == true || size.y > available_size.y) ? available_size.y : size.y;
	return size;
}

void yuika::Widget::OnDraw(DrawApi& api, Rect allowed_draw_area) const
{
	// There are less surprises by setting a hit area by default
	api.SetHitArea({allowed_draw_area.pos, GetSize(allowed_draw_area.size)});
}

yuika::EventPropagation yuika::Widget::OnMouse(MouseGesture, Vector, uint32_t&, const Widget&)
{
	return EventPropagation::KeepPassingIt;
}

void yuika::Widget::OnLayout(LayoutApi& api)
{
	m_minimum_size = api.Margin(*this);
}


// ############################


size_t yuika::Leaf::GetChildrenNo() const
{
	return 0;
}

yuika::Widget::ChildInLayout yuika::Leaf::GetChildInLayout(LayoutApi&, size_t, Vector, Vector) const
{
	throw 1; // TODO
}

yuika::Widget& yuika::Leaf::GetChild(size_t) const
{
	throw 1; // TODO
}

yuika::Spacer::Spacer(bool x, bool y)
{
	SetStretch(x, y);
	SetClassId("Spacer");
}


// ############################


size_t yuika::Frame::GetChildrenNo() const
{
	return (m_content == nullptr) ? 0 : 1;
}

yuika::Widget& yuika::Frame::SetChild(std::unique_ptr<Widget> widget)
{
	m_content = std::move(widget);
	SetValidation(Widget::Validation::Layout, false);
	return *m_content;
}

yuika::Widget::ChildInLayout yuika::Frame::GetChildInLayout(LayoutApi&, size_t, Vector size, Vector) const
{
	EXCEPT(m_content == nullptr, 1); // TODO
	return {*m_content, {}, m_content->GetSize(size), {}};
}

yuika::Widget& yuika::Frame::GetChild(size_t) const
{
	EXCEPT(m_content == nullptr, 1); // TODO
	return *m_content;
}

void yuika::Frame::OnLayout(LayoutApi& api)
{
	m_minimum_size = api.Margin(*this);
	if (m_content != nullptr)
		m_minimum_size += m_content->GetMinimumSize();
}


// ############################


yuika::Box::Box(Direction direction, bool wrap) : Container()
{
	m_direction = direction;
	m_stretch_childs = 0;
	m_non_stretch_size = {};
	m_wrap = wrap;
	SetClassId("Box");
}

yuika::Box& yuika::Box::SetWrap(bool wrap)
{
	m_wrap = wrap;
	return *this;
}

bool yuika::Box::GetWrap() const
{
	return m_wrap;
}

size_t yuika::Box::GetChildrenNo() const
{
	return m_children.size();
}

yuika::Widget& yuika::Box::AddChild(std::unique_ptr<Widget> widget)
{
	m_children.push_back(std::move(widget));
	SetValidation(Widget::Validation::Layout, false);
	return *m_children.back();
}

class yuika::BoxFriend
{
  public:
	template <typename T>
	static Widget::ChildInLayout GetChildInLayout(T& fwend, LayoutApi&, size_t no, Vector fwend_size, Vector cursor)
	{
		EXCEPT(no >= fwend.m_children.size(), 1); // TODO
		auto& child = fwend.m_children.at(no);

		Vector next_offset;
		Vector child_size;

		switch (fwend.m_direction)
		{
		case Box::Direction::Horizontal:
		{
			if (fwend.m_wrap == false && child->GetStretchX() == true) // TODO, wrap interacts poorly with stretch
			{
				fwend_size.x =
				    (fwend_size.x - fwend.m_non_stretch_size.x) / std::max(static_cast<int>(fwend.m_stretch_childs), 1);
				fwend_size.x = std::max(fwend_size.x, child->GetMinimumSize().x);
			}

			child_size = child->GetSize(fwend_size);
			next_offset = {child_size.x, 0};

			// If there is no space for next widget, move cursor left and a row below
			if (fwend.m_wrap == true)
			{
				Vector next_size = {};

				if (no + 1 < fwend.m_children.size())
					next_size.x = fwend.m_children.at(no + 1)->GetMinimumSize().x; // TODO, a bit ugly to ask size twice

				if (cursor.x + child_size.x + next_size.x > fwend_size.x)
				{
					next_offset.x -= cursor.x + child_size.x;
					next_offset.y = fwend.m_minimum_size.y; // Works as it's an infinite horizontal row,
					                                        // all rows will have the same height tho
				}
			}
		}
		break;
		case Box::Direction::Vertical:
		{
			if (child->GetStretchY() == true)
			{
				fwend_size.y =
				    (fwend_size.y - fwend.m_non_stretch_size.y) / std::max(static_cast<int>(fwend.m_stretch_childs), 1);
				fwend_size.y = std::max(fwend_size.y, child->GetMinimumSize().y);
			}

			child_size = child->GetSize(fwend_size);
			next_offset = {0, child_size.y};
		}
		break;
		}

		return {*child, {}, child_size, next_offset};
	}
};

yuika::Widget::ChildInLayout yuika::Box::GetChildInLayout(LayoutApi& api, size_t no, Vector size, Vector cursor) const
{
	return BoxFriend::GetChildInLayout(*this, api, no, GetSize(size), cursor);
}

yuika::Widget& yuika::Box::GetChild(size_t no) const
{
	EXCEPT(no >= m_children.size(), 1); // TODO
	return *m_children.at(no);
}

void yuika::Box::OnLayout(LayoutApi& api)
{
	m_minimum_size = {};
	m_non_stretch_size = {};
	m_stretch_childs = 0;

	for (auto& child : m_children)
	{
		const Vector child_minimum_size = child->GetMinimumSize();

		switch (m_direction)
		{
		case Direction::Horizontal:
			m_minimum_size.x += child_minimum_size.x;
			m_minimum_size.y = std::max(child_minimum_size.y, m_minimum_size.y);
			m_stretch_childs += static_cast<size_t>(child->GetStretchX());
			break;
		case Direction::Vertical:
			m_minimum_size.x = std::max(child_minimum_size.x, m_minimum_size.x);
			m_minimum_size.y += child_minimum_size.y;
			m_stretch_childs += static_cast<size_t>(child->GetStretchY());
			break;
		}

		m_non_stretch_size.x += (child->GetStretchX() == false) ? child_minimum_size.x : 0;
		m_non_stretch_size.y += (child->GetStretchY() == false) ? child_minimum_size.y : 0;
	}

	m_minimum_size += api.Margin(*this);
}

yuika::HBox::HBox(bool wrap) : Box(Direction::Horizontal, wrap)
{
	SetClassId("HBox");
}

yuika::VBox::VBox(bool wrap) : Box(Direction::Vertical, wrap)
{
	SetClassId("VBox");
}


// ############################


yuika::Text::Text(std::string text) : Leaf()
{
	m_text = std::move(text);
	SetClassId("Text");
}

void yuika::Text::OnDraw(DrawApi& api, Rect allowed_draw_area) const
{
	api.SetHitArea({allowed_draw_area.pos, api.TextSize(m_text.c_str())});
	api.DrawCheckerBoardRectangle(DrawApi::RED, {allowed_draw_area.pos, api.TextSize(m_text.c_str())});
	api.DrawText(DrawApi::BLACK, allowed_draw_area.pos, m_text.c_str());
}

void yuika::Text::OnLayout(LayoutApi& api)
{
	m_minimum_size = api.TextSize(m_text.c_str()) + api.Margin(*this);
};


// ############################


yuika::Button::Button(Style style, EventPropagation default_propagation) : Frame()
{
	m_style = style;
	m_state = State::Normal;
	m_pressed = false;
	m_inside = false;
	m_default_propagation = default_propagation;
	SetClassId("Button");
}

void yuika::Button::OnDraw(DrawApi& api, Rect allowed_draw_area) const
{
	const Vector size = GetSize(allowed_draw_area.size);

	api.SetHitArea({allowed_draw_area.pos, size});
	api.DrawRectangle(DrawApi::BACKGROUND, {allowed_draw_area.pos, size});

	switch (m_state)
	{
	case State::Normal:
		if (m_style != Style::Flat)
			api.DrawBorder({allowed_draw_area.pos, size}, DrawApi::BorderStyle::Outset);
		break;
	case State::Over:
#if 1 // #ifndef NDEBUG // White for debugging purposes
		api.DrawRectangle(DrawApi::WHITE, {allowed_draw_area.pos, size});
#endif
		api.DrawBorder({allowed_draw_area.pos, size}, DrawApi::BorderStyle::Outset);
		break;
	case State::Pressed:
		api.DrawBorder({allowed_draw_area.pos + 1, size - 2}, DrawApi::BorderStyle::Inset);
		api.DrawBorder({allowed_draw_area.pos, size}, DrawApi::BorderStyle::Solid, DrawApi::BLACK);
		break;
	}
}

yuika::EventPropagation yuika::Button::OnMouse(MouseGesture gesture, Vector, uint32_t& cookie, const Widget& target)
{
	const State prev_state = m_state;

	switch (gesture)
	{
	case yuika::MouseGesture::Press: m_pressed = true; break;
	case yuika::MouseGesture::Release: m_pressed = false; break;

	case yuika::MouseGesture::Enters:
		if (&target == this) // TODO, ugly, think it better
			m_inside = true;
		break;
	case yuika::MouseGesture::Leaves:
		if (&target == this)
			m_inside = false;
		break;

	default: break;
	}

	if (m_inside == true)
	{
#if 1 // #ifndef NDEBUG // Classic Windows don't highlight on mouse over
		m_state = (m_pressed == true) ? State::Pressed : State::Over;
#else
		if (m_style != Style::Flat)
			m_state = (m_pressed == true) ? State::Pressed : State::Normal;
		else // Classic does on flat buttons tho
			m_state = (m_pressed == true) ? State::Pressed : State::Over;
#endif
	}
	else
		m_state = State::Normal;

	if (prev_state != m_state)
		SetValidation(Widget::Validation::Draw, false);

	if (m_default_propagation == EventPropagation::TakePlaceAsTarget)
		cookie = GetCookie();
	return m_default_propagation;
}

yuika::Widget::ChildInLayout yuika::Button::GetChildInLayout(LayoutApi&, size_t, Vector size, Vector) const
{
	EXCEPT(m_content == nullptr, 1); // TODO

	if (m_state == State::Pressed)
		return {*m_content, {1, 1}, m_content->GetSize(size - 2), {}};

	return {*m_content, {}, m_content->GetSize(size), {}};
}


yuika::ButtonWithText::ButtonWithText(const std::string& text, Style style, EventPropagation default_propagation)
    : Button(style, default_propagation)
{
	SetNewChild<Text>(text);
	SetClassId("ButtonWithText");
}
