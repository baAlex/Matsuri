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
#include <limits.h>
#include <string.h>


#include "arialn.inc"


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


static bool sInside(yuika::Position pos, yuika::Rect rect)
{
	if (pos.x >= rect.pos.x && pos.y >= rect.pos.y && //
	    pos.x < rect.pos.x + rect.size.w && pos.y < rect.pos.y + rect.size.h)
		return true;
	return false;
}

static yuika::Rect sOr(yuika::Rect a, yuika::Rect b)
{
	const int x1 = Min(a.pos.x, b.pos.x);
	const int y1 = Min(a.pos.y, b.pos.y);
	const int x2 = Max(a.pos.x + a.size.w, b.pos.x + b.size.w);
	const int y2 = Max(a.pos.y + a.size.h, b.pos.y + b.size.h);
	return {{x1, y1}, {x2 - x1, y2 - y1}};
}

static yuika::Rect sInverseRect()
{
	yuika::Rect ret;
	ret.pos.x = INT_MAX;
	ret.pos.y = INT_MAX;
	ret.size.w = INT_MIN;
	ret.size.h = INT_MIN;
	return ret;
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

	m_root = new Root();
	m_root->SetStretch(true, true); // A good default value
	m_root->SetId("#__root");

	if ((m_font = reinterpret_cast<uint8_t*>(malloc(sizeof(uint8_t) * ATLAS_WIDTH * ATLAS_HEIGHT))) == nullptr)
		throw 1; // TODO

	m_masks[2] = r_mask;
	m_masks[1] = g_mask;
	m_masks[0] = b_mask;

	m_dirty_area = sInverseRect();

	// SDF,
	// do fragment-shader work offline, as we are 2d
	for (size_t i = 0; i < ATLAS_WIDTH * ATLAS_HEIGHT; i += 1)
	{
		const auto min = static_cast<float>(128 - 8) / 255.0f;
		const auto max = static_cast<float>(128 + 8) / 255.0f;

		auto p = static_cast<float>(255 - ATLAS_DATA[i]) / 255.0f;

		p = (Clamp(p, min, max) - min) / (max - min);

		// https://registry.khronos.org/OpenGL-Refpages/gl4/html/smoothstep.xhtml
		const float p2 = p * p * (3.0f - 2.0f * p); // I'm not sure, Valve uses it, but they
		// didn't take gamma in consideration, also, should I compensate it here
		// since the blitter has no idea what gamma is?.

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

		void SetHitArea(Rect rect) noexcept override
		{
			hit_area = rect; // If already set, we overwrite last one (TODO, validate and clamp area)
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
					*col = colour;
				}
			}
		}

		void DrawCheckerBoardRectangle(Colour colour, Rect rect) noexcept override
		{
			const int x1 = Clamp(rect.pos.x, 0, fwend->m_size.w);
			const int y1 = Clamp(rect.pos.y, 0, fwend->m_size.h);
			rect.size.w = (Clamp(rect.pos.x + rect.size.w, 0, fwend->m_size.w) - x1);
			rect.size.h = (Clamp(rect.pos.y + rect.size.h, 0, fwend->m_size.h) - y1) * fwend->m_size.w;

			uint8_t kiki = 0;
			uint8_t boba = 0;

			uint32_t* out = fwend->m_out + static_cast<size_t>(x1 + y1 * fwend->m_size.w);
			for (uint32_t* row = out; row < out + rect.size.h; row += static_cast<size_t>(fwend->m_size.w))
			{
				for (uint32_t* col = row; col < row + rect.size.w; col += 1)
				{
					*col = (((kiki ^ boba) & 1) != 0) ? colour : *col;
					boba++;
				}
				kiki++;
				boba = 0;
			}
		}

		void Draw3dBevel(Rect rect, BevelStyle style) noexcept override
		{
			if (style == BevelStyle::Inset)
			{
				DrawRectangle(BEVEL_MID, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(BEVEL_MID, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(BEVEL_LIGHT, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(BEVEL_LIGHT, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});
			}
			else
			{
				DrawRectangle(BEVEL_LIGHT, {{rect.pos.x, rect.pos.y}, {rect.size.w - 1, 1}});
				DrawRectangle(BEVEL_LIGHT, {{rect.pos.x, rect.pos.y + 1}, {1, rect.size.h - 1}});
				DrawRectangle(BEVEL_SHADOW, {{rect.pos.x + rect.size.w - 1, rect.pos.y}, {1, rect.size.h}});
				DrawRectangle(BEVEL_SHADOW, {{rect.pos.x, rect.pos.y + rect.size.h - 1}, {rect.size.w, 1}});

				DrawRectangle(BEVEL_MID, {{rect.pos.x + rect.size.w - 2, rect.pos.y + 1}, {1, rect.size.h - 2}});
				DrawRectangle(BEVEL_MID, {{rect.pos.x + 1, rect.pos.y + rect.size.h - 2}, {rect.size.w - 2, 1}});
			}
		}

		void DrawText(Colour colour, Position pos, const char* text) noexcept override
		{
			auto xf = static_cast<float>(pos.x);
			auto yf = static_cast<float>(pos.y);

			for (const char* c = text; *c != 0x00; c += 1)
			{
				if (static_cast<size_t>(*c) < FIRST_CHARACTER_CODE ||
				    (static_cast<size_t>(*c) - FIRST_CHARACTER_CODE) >= CHARACTERS_NO)
					continue;

				const CharacterMetric* ch = CHARACTERS_METRICS + (static_cast<size_t>(*c) - FIRST_CHARACTER_CODE);

				Rect rect = {{static_cast<int>(xf + ch->x_offset), static_cast<int>(yf + ch->y_offset)},
				             {ch->width, ch->height}};
				xf += ch->advance;

				if (*c == ' ')
					continue;

				const int x1 = Clamp(rect.pos.x, 0, fwend->m_size.w);
				const int y1 = Clamp(rect.pos.y, 0, fwend->m_size.h);
				rect.size.w = (Clamp(rect.pos.x + rect.size.w, 0, fwend->m_size.w) - x1);
				rect.size.h = (Clamp(rect.pos.y + rect.size.h, 0, fwend->m_size.h) - y1) * fwend->m_size.w;

				const auto clamp_diff_x = static_cast<size_t>(x1 - rect.pos.x);
				const auto clamp_diff_y = static_cast<size_t>(y1 - rect.pos.y);
				const uint8_t* in_row =
				    fwend->m_font + (ch->atlas_x + clamp_diff_x) + ATLAS_WIDTH * (ch->atlas_y + clamp_diff_y);
				const uint32_t in_pitch = ATLAS_WIDTH;

				uint32_t* out = fwend->m_out + static_cast<size_t>(x1 + y1 * fwend->m_size.w);
				for (uint32_t* row = out; row < out + rect.size.h; row += static_cast<size_t>(fwend->m_size.w))
				{
					const uint8_t* in_col = in_row;

					for (uint32_t* out_col = row; out_col < row + rect.size.w; out_col += 1)
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

		Size TextSize(const char* text) override
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
	};

	static void DrawWidgetsAndSetMiniTree(DrawApiImplementation& api)
	{
		// Non-recursive draw, it has the good feature of carry information while
		// descending the tree, like depth, drawable area, and more; also from our
		// end we can identify on which widget we are without asking it to widgets
		// themselves

		api.fwend->m_mini_tree_len = 0;
		size_t cursor = 0;

		// Add root first
		{
			Screen::StackEntry* stack_entry = api.fwend->m_stack + cursor;
			cursor++;

			stack_entry->widget = api.fwend->m_root;
			stack_entry->parent_mini = nullptr;
			stack_entry->mini = api.fwend->m_mini_tree;
			stack_entry->allowed_draw_area = Rect{{0, 0}, api.fwend->m_size};

			Screen::MiniTreeEntry* mini_entry = api.fwend->m_mini_tree + api.fwend->m_mini_tree_len;
			api.fwend->m_mini_tree_len++;

			memset(mini_entry, 0, sizeof(Screen::MiniTreeEntry));
			mini_entry->widget = api.fwend->m_root;
		}

		// Now widget as normal
		while (cursor > 0)
		{
			Screen::StackEntry current = api.fwend->m_stack[--cursor]; // Yes, copy it

			// Draw widget
			api.hit_area = {};
			current.widget->Draw(api, current.allowed_draw_area);
			current.widget->SetDirty(false);

			// Update mini tree entry,
			// those fields that weren't know when mini tree entry was created
			if (current.mini != nullptr)
			{
				current.mini->last_draw_at = current.allowed_draw_area;
				current.mini->hit_area = api.hit_area;
			}

			if (current.parent_mini != nullptr)
				current.parent_mini->last_child = current.mini;

			// Iterate children
			if (cursor + current.widget->GetChildrenNo() >= Screen::STACK_LEN)
				throw 1; // TODO

			cursor += current.widget->GetChildrenNo();
			for (size_t i = 0; i < current.widget->GetChildrenNo(); i += 1)
			{
				const auto [child, delta, child_size] = current.widget->GetChild(i, current.allowed_draw_area.size);

				// Stack children,
				// just for iteration in this function
				Screen::StackEntry* stack_entry = api.fwend->m_stack + cursor - 1 - i;

				stack_entry->widget = &child;
				stack_entry->parent_mini = current.mini;
				stack_entry->mini = &api.fwend->m_mini_tree[api.fwend->m_mini_tree_len];
				stack_entry->allowed_draw_area = {current.allowed_draw_area.pos, child_size};

				//
				current.allowed_draw_area.pos.x += delta.x;
				current.allowed_draw_area.pos.y += delta.y;

				// Store children in mini tree,
				// other parts of the code will use it
				Screen::MiniTreeEntry* mini_entry = api.fwend->m_mini_tree + api.fwend->m_mini_tree_len;
				api.fwend->m_mini_tree_len++;

				memset(mini_entry, 0, sizeof(Screen::MiniTreeEntry));
				mini_entry->widget = &child;
				mini_entry->parent = current.mini;
			}
		}
	}

	static void DrawWidget(DrawApiImplementation& api, Widget* widget, Rect allowed_draw_area)
	{
		size_t cursor = 0;

		// Add root first
		{
			Screen::StackEntry* stack_entry = api.fwend->m_stack + cursor;
			cursor++;

			stack_entry->widget = widget;
			stack_entry->allowed_draw_area = allowed_draw_area;
		}

		// Now widget as normal
		while (cursor > 0)
		{
			Screen::StackEntry current = api.fwend->m_stack[--cursor];

			// Draw widget (TODO?, I'm ignoring the hit area, we are not adjusting tree anyways)
			current.widget->Draw(api, current.allowed_draw_area);
			current.widget->SetDirty(false);

			// Iterate children
			if (cursor + current.widget->GetChildrenNo() >= Screen::STACK_LEN)
				throw 1; // TODO

			cursor += current.widget->GetChildrenNo();
			for (size_t i = 0; i < current.widget->GetChildrenNo(); i += 1)
			{
				const auto [child, delta, child_size] = current.widget->GetChild(i, current.allowed_draw_area.size);

				// Stack children,
				// just for iteration in this function
				Screen::StackEntry* stack_entry = api.fwend->m_stack + cursor - 1 - i;

				stack_entry->widget = &child;
				stack_entry->allowed_draw_area = {current.allowed_draw_area.pos, child_size};

				//
				current.allowed_draw_area.pos.x += delta.x;
				current.allowed_draw_area.pos.y += delta.y;
			}
		}
	}
};


static constexpr bool UPDATE_NATURAL_SIZE_LIKE_CRAZY = false;
static constexpr bool DRAW_LIKE_CRAZY = false;

yuika::Rect yuika::Screen::Draw(Size size, uint32_t* out)
{
	ScreenFriend::DrawApiImplementation draw_api;
	draw_api.fwend = this;

	m_out = out;

	// Full draw
	if (m_size.w != size.w || m_size.h != size.h || DRAW_LIKE_CRAZY == true)
	{
		m_size = size;
		m_dirty_area = sInverseRect(); // We are doing a full draw

		// Update natural sizes
		m_root->UpdateNaturalSize(draw_api); // [Recursion]

		// Draw
		draw_api.DrawRectangle(DrawApi::BACKGROUND, {{0, 0}, m_size});
		ScreenFriend::DrawWidgetsAndSetMiniTree(draw_api);

		// Do conversion
		if (m_masks[2] != 0x00FF0000 || m_masks[1] != 0x0000FF00 || m_masks[0] != 0x000000FF)
		{
			// Disgusting but the auto-vectoriser is doing it! (and in a horizontal
			// manner, which makes sense since there is no shift for individual
			// lanes; anyways it's 8 values with AVX and 4 with SSE2)

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

			for (uint32_t* p = out; p < out + static_cast<size_t>(size.w * size.h); p += 1)
			{
				const auto a = static_cast<uint32_t>((*p >> 24) & 0xFF);
				const auto r = static_cast<uint32_t>((*p >> 16) & 0xFF);
				const auto g = static_cast<uint32_t>((*p >> 8) & 0xFF);
				const auto b = static_cast<uint32_t>((*p >> 0) & 0xFF);

				*p = ((b << ls[0]) & lm[0]) | ((g << ls[1]) & lm[1]) | //
				     ((r << ls[2]) & lm[2]) | ((a << ls[3]) & lm[3]) | //
				     ((b >> rs[0]) & rm[0]) | ((g >> rs[1]) & rm[1]) | //
				     ((r >> rs[2]) & rm[2]) | ((a >> rs[3]) & rm[3]);
			}
		}

		// Developers, developers, developers
		if (false)
		{
			for (const MiniTreeEntry* m = m_mini_tree; m < m_mini_tree + m_mini_tree_len; m += 1)
			{
				printf("%p | ", reinterpret_cast<const void*>(m));
				printf("%s (childs: %zu, last one: %p)\n", m->widget->GetId(), m->widget->GetChildrenNo(),
				       reinterpret_cast<const void*>(m->last_child));
			}
		}

		// Done!
		return {}; // TODO
	}

	// Partial draw
	const auto inverse = sInverseRect();
	if (memcmp(&m_dirty_area, &inverse, sizeof(Rect)) != 0)
	{
		draw_api.DrawRectangle(DrawApi::PINK, m_dirty_area);

		// TODO, here I should set the dirty area as the scissoring box (a la' OpenGL)
		// (as soon I implement it)

		for (MiniTreeEntry* i = m_mini_tree; i < m_mini_tree + m_mini_tree_len; i += 1)
		{
			// TODO, like most for loops iterating the mini tree, an obvious optimisation is to
			// keep different list/arrays/mini-trees, and not use the same one for everything
			if (i->widget->GetDirty() == true)
			{
				ScreenFriend::DrawWidget(draw_api, i->widget, i->last_draw_at);
				// i->widget->SetDirty(false); // Set inside DrawWidget()
			}
		}

		m_dirty_area = sInverseRect();
	}

	// Nothing was done
	return {};
}


void yuika::Screen::MousePress(Position cursor_pos)
{
	// We do bubbling here
	// https://developer.mozilla.org/en-US/docs/Learn_web_development/Core/Scripting/Event_bubbling

	// Find target, going down the mini tree
	MiniTreeEntry* target = nullptr;
	for (MiniTreeEntry* next = m_mini_tree; next != nullptr;)
	{
		target = next;

		if (false) // Developers, developers, developers
			printf("Find | %p, \"%s\" (%zu)\n", reinterpret_cast<const void*>(target->widget), target->widget->GetId(),
			       target->widget->GetChildrenNo());

		target->pressed_indirectly = true;

		// Iterate children
		next = nullptr;
		MiniTreeEntry* child = target->last_child;
		for (size_t child_no = 0; child_no < target->widget->GetChildrenNo(); child_no += 1, child -= 1)
		{
			if (sInside(cursor_pos, child->hit_area) == true)
			{
				next = child;
				break;
			}
		}
	}

	// Press event
	target->pressed_as_target = true;
	for (MiniTreeEntry* i = target; i != nullptr; i = i->parent)
	{
		if (false) // Developers, developers, developers
			printf("Bubbling | %p, \"%s\"\n", reinterpret_cast<const void*>(i->widget), i->widget->GetId());

		if (i->widget->OnMouse(MouseGesture::Press, cursor_pos, *target->widget) == EventPropagation::StopIt)
			break;
	}
}

void yuika::Screen::MouseRelease(Position cursor_pos)
{
	for (MiniTreeEntry* i = m_mini_tree; i < m_mini_tree + m_mini_tree_len; i += 1)
	{
		// Release event
		if (i->pressed_as_target == true) // TODO, keep a list of what was pressed, to avoid iterate the entire thing
		{
			i->pressed_as_target = false;

			for (MiniTreeEntry* u = i; u != nullptr; u = u->parent)
			{
				if (u->widget->OnMouse(MouseGesture::Release, cursor_pos, *i->widget) == EventPropagation::StopIt)
					break;
			}
		}

		// Click event
		if (i->pressed_indirectly == true) // TODO, same
		{
			i->pressed_indirectly = false;

			if (sInside(cursor_pos, i->hit_area) == true)
			{
				for (MiniTreeEntry* u = i; u != nullptr; u = u->parent)
				{
					if (u->widget->OnMouse(MouseGesture::Click, cursor_pos, *i->widget) == EventPropagation::StopIt)
						break;
				}
			}
		}
	}
}

void yuika::Screen::MouseMoves(Position cursor_pos)
{
	// Leaves event
	for (MiniTreeEntry* i = m_mini_tree; i < m_mini_tree + m_mini_tree_len; i += 1)
	{
		if (i->cursor_inside == true && sInside(cursor_pos, i->hit_area) == false)
		{
			i->cursor_inside = false;
			for (MiniTreeEntry* u = i; u != nullptr; u = u->parent)
			{
				if (u->widget->OnMouse(MouseGesture::Leaves, cursor_pos, *i->widget) == EventPropagation::StopIt)
					break;
				if (u->widget->GetDirty() == true)
					m_dirty_area = sOr(m_dirty_area, u->last_draw_at);
			}
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
			for (MiniTreeEntry* u = target; u != nullptr; u = u->parent)
			{
				if (u->widget->OnMouse(MouseGesture::Enters, cursor_pos, *target->widget) == EventPropagation::StopIt)
					break;
				if (u->widget->GetDirty() == true)
					m_dirty_area = sOr(m_dirty_area, u->last_draw_at);
			}
		}

		// Iterate children
		next = nullptr;
		MiniTreeEntry* child = target->last_child;
		for (size_t child_no = 0; child_no < target->widget->GetChildrenNo(); child_no += 1, child -= 1)
		{
			if (sInside(cursor_pos, child->hit_area) == true)
			{
				next = child;
				break;
			}
		}
	}
}


yuika::Wrapper& yuika::Screen::GetRoot()
{
	return *m_root;
}


// ############################


yuika::Widget::Widget()
{
	m_stretch_x = false; // Is not possible to set bitfields on headers
	m_stretch_y = false; // (a C++ quirk)
	m_natural_size_updated = false;
	m_dirty = false;
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

void yuika::Widget::SetDirty(bool value)
{
	m_dirty = value;
}

bool yuika::Widget::GetDirty() const
{
	return m_dirty;
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
	// There are less surprises by setting a hit area by default
	api.SetHitArea({allowed_draw_area.pos, GetSize(allowed_draw_area.size)});
}

yuika::EventPropagation yuika::Widget::OnMouse(MouseGesture, Position, const Widget&)
{
	return EventPropagation::KeepPassingIt;
}


// ############################


size_t yuika::Wrapper::GetChildrenNo() const
{
	return (m_content == nullptr) ? 0 : 1;
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

yuika::Size yuika::Wrapper::UpdateNaturalSize(SimpleApi& api)
{
	if (m_natural_size_updated == false || UPDATE_NATURAL_SIZE_LIKE_CRAZY == true)
	{
		m_natural_size_updated = true;
		m_natural_size = (m_content != nullptr) ? m_content->UpdateNaturalSize(api) : Size{0, 0}; // [Recursion]
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
	return BoxFriend::GetChild(*this, no, GetSize(available_size));
}

const yuika::Widget::ChildGet yuika::Box::GetChild(size_t no, Size available_size) const
{
	return BoxFriend::GetChild(*this, no, GetSize(available_size));
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

yuika::Size yuika::Box::UpdateNaturalSize(SimpleApi& api)
{
	if (m_natural_size_updated == false || UPDATE_NATURAL_SIZE_LIKE_CRAZY == true)
	{
		m_natural_size_updated = true;
		m_natural_size = {};
		m_non_stretch_size = {};
		m_stretch_childs = 0;

		for (auto& child : m_children)
		{
			const Size size = child->UpdateNaturalSize(api); // [Recursion]

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


yuika::Text::Text(std::string text) : Widget()
{
	m_text = std::move(text);
}


static constexpr int TEXT_MARGIN = 18; // TODO, implement styles or something similar

void yuika::Text::Draw(DrawApi& api, Rect allowed_draw_area) const
{
	api.SetHitArea({allowed_draw_area.pos, allowed_draw_area.size});
	// api.DrawCheckerBoardRectangle(DrawApi::RED, {allowed_draw_area.pos, allowed_draw_area.size});

	api.DrawText(DrawApi::BLACK, {allowed_draw_area.pos.x + TEXT_MARGIN / 2, allowed_draw_area.pos.y + TEXT_MARGIN / 2},
	             m_text.c_str());
}


size_t yuika::Text::GetChildrenNo() const
{
	return 0;
};

yuika::Widget::ChildGet yuika::Text::GetChild(size_t, Size)
{
	throw 1;
};

const yuika::Widget::ChildGet yuika::Text::GetChild(size_t, Size) const
{
	throw 1;
};

yuika::Widget& yuika::Text::GetChild(size_t)
{
	throw 1;
};

const yuika::Widget& yuika::Text::GetChild(size_t) const
{
	throw 1;
};

yuika::Size yuika::Text::UpdateNaturalSize(SimpleApi& api)
{
	if (m_natural_size_updated == false || UPDATE_NATURAL_SIZE_LIKE_CRAZY == true)
	{
		m_natural_size_updated = true;

		m_natural_size = api.TextSize(m_text.c_str());
		m_natural_size.w += TEXT_MARGIN;
		m_natural_size.h += TEXT_MARGIN;
	}

	return m_natural_size;
};


// ############################


yuika::Button::Button() : Wrapper() {}

void yuika::Button::Draw(DrawApi& api, Rect allowed_draw_area) const
{
	api.SetHitArea({allowed_draw_area.pos, GetSize(allowed_draw_area.size)});
	api.Draw3dBevel({allowed_draw_area.pos, GetSize(allowed_draw_area.size)}, DrawApi::BevelStyle::Outset);
}
