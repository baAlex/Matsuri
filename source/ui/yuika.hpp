/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef YUIKA_HPP
#define YUIKA_HPP

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

namespace yuika
{

struct Vector
{
	int x, y;

	// clang-format off
	Vector& operator+=(const Vector& other) { this->x += other.x; this->y += other.y; return *this; }
	Vector& operator-=(const Vector& other) { this->x -= other.x; this->y -= other.y; return *this; }

	Vector& operator+=(int other) { this->x += other; this->y += other; return *this; }
	Vector& operator-=(int other) { this->x -= other; this->y -= other; return *this; }
	Vector& operator*=(int other) { this->x *= other; this->y *= other; return *this; }
	Vector& operator/=(int other) { this->x /= other; this->y /= other; return *this; }

	Vector operator+(const Vector& other) const { return {this->x + other.x, this->y + other.y}; }
	Vector operator-(const Vector& other) const { return {this->x - other.x, this->y - other.y}; }
	Vector operator+(int other) const           { return {this->x + other, this->y + other}; }
	Vector operator-(int other) const           { return {this->x - other, this->y - other}; }
	Vector operator*(int other) const           { return {this->x * other, this->y * other}; }
	Vector operator/(int other) const           { return {this->x / other, this->y / other}; }
	// clang-format on
};

struct Rect
{
	Vector pos;
	Vector size;

	bool IsPointInside(Vector position) const
	{
		if (position.x >= this->pos.x && position.y >= this->pos.y && //
		    position.x < this->pos.x + this->size.x && position.y < this->pos.y + this->size.y)
			return true;
		return false;
	}

	Rect Union(Rect other) const
	{
		const int x1 = std::min(this->pos.x, other.pos.x);
		const int y1 = std::min(this->pos.y, other.pos.y);
		const int x2 = std::max(this->pos.x + this->size.x, other.pos.x + other.size.x);
		const int y2 = std::max(this->pos.y + this->size.y, other.pos.y + other.size.y);
		return {{x1, y1}, {x2 - x1, y2 - y1}};
	}

	static Rect Inverse()
	{
		Rect ret;
		ret.pos.x = INT_MAX;
		ret.pos.y = INT_MAX;
		ret.size.x = INT_MIN;
		ret.size.y = INT_MIN;
		return ret;
	}
};

enum class MouseGesture : uint8_t
{
	Press,   // Primary button, triggers if cursor is on top of widget
	Release, // Primary button, triggers only if widget was previously
	         // pressed, it does it regardless of where cursor is

	Click, // Similar to release, except it checks if cursor is over widget

	Enters, // Enters the widget
	Leaves, // Guess it
};

enum class EventPropagation : uint8_t
{
	KeepPassingIt,
	StopIt,
	TakePlaceAsTarget
};

class Widget;

class LayoutApi
{
  public:
	virtual Vector TextSize(const char* text) = 0; // Vertical is x-size only, forget about ascenders and descenders
	virtual Vector Margin(const Widget&) = 0;
};

class DrawApi : public LayoutApi
{
  public:
	using Colour = uint32_t;

	static constexpr Colour BLACK = 0xFF000000;
	static constexpr Colour WHITE = 0xFFFFFFFF;
	static constexpr Colour RED = 0xFFFF0000;
	static constexpr Colour GREEN = 0xFF00FF00;
	static constexpr Colour BLUE = 0xFF0000FF;
	static constexpr Colour PINK = 0xFFFF00FF;
	static constexpr Colour BACKGROUND = 0xFFD4D0C8; // Windows 2000
	static constexpr Colour BEVEL_MID = 0xFF808080;
	static constexpr Colour BEVEL_SHADOW = 0xFF404040;
	static constexpr Colour BEVEL_LIGHT = WHITE;

	virtual void SetHitArea(Rect rect) = 0;
	virtual void DrawRectangle(Colour colour, Rect rect) = 0;
	virtual void DrawCheckerBoardRectangle(Colour colour, Rect rect) = 0;

	enum class BorderStyle : uint8_t
	{
		Inset,
		Outset,
		Frame,
		Solid, // Uses 'colour' argument
	};

	virtual void DrawBorder(Rect rect, BorderStyle style, Colour colour = BLACK) = 0;
	virtual void DrawText(Colour colour, Vector pos, const char* text) = 0;
};


class Widget
{
  public:
	Widget();
	virtual ~Widget() noexcept = default;

	struct ChildInLayout
	{
		Widget& child;
		Vector child_offset; // Relative from parent
		Vector child_size;   // Includes margin
		Vector next_offset;  // Relative from parent
	};

	enum class Validation : uint8_t
	{
		Draw,
		Layout
	};

	virtual size_t GetChildrenNo() const = 0;
	virtual Widget& GetChild(size_t no) const = 0; // Throws if there is no child
	virtual ChildInLayout GetChildInLayout(LayoutApi& api, size_t no, Vector size,
	                                       Vector cursor) const = 0; // Ditto

	virtual Vector GetMinimumSize() const;               // Includes margin
	virtual Vector GetSize(Vector available_size) const; // Includes margin

	// On- prefixed methods are called from screen, from specific points of execution,
	// widgets themselves shouldn't call these

	virtual void OnLayout(LayoutApi& api);
	virtual void OnDraw(DrawApi& api, Rect allowed_draw_area);
	virtual EventPropagation OnMouse(MouseGesture gesture, Vector cursor_pos, uint32_t& cookie, const Widget& target);

	virtual Widget& SetStretch(bool x, bool y);
	virtual bool GetStretchX() const;
	virtual bool GetStretchY() const;

	Widget& SetId(const std::string& text);
	const std::string& GetId() const;
	Widget& SetClassId(const std::string& id); // A la CSS
	const std::string& GetClassId() const;
	Widget& SetCookie(uint32_t cookie);
	uint32_t GetCookie() const;
	Widget& SetValidation(Validation, bool value);
	bool GetValidation(Validation) const;

  protected:
	Vector m_minimum_size; // Most widgets should follow/implement this,
	bool m_stretch_x : 1;  // this,
	bool m_stretch_y : 1;  // and this

  private:
	bool m_draw_valid : 1;
	bool m_layout_valid : 1;
	std::string m_id;
	std::string m_class_id;
	uint32_t m_cookie;
};


class Leaf : public Widget // Widget without children
{
  public:
	size_t GetChildrenNo() const override; // Always returns zero
	Widget& GetChild(size_t) const override;
	ChildInLayout GetChildInLayout(LayoutApi&, size_t, Vector, Vector) const override;
};

class Spacer : public Leaf
{
  public:
	Spacer(bool stretch_x = true, bool stretch_y = true);
};


class Frame : public Widget // Widget with one children
{
  public:
	size_t GetChildrenNo() const override; // Returns zero or one
	Widget& SetChild(std::unique_ptr<Widget> widget);

	template <typename T, typename... ARGS> T& SetNewChild(ARGS&&... args)
	{
		auto widget = std::make_unique<T>(std::forward<ARGS>(args)...);
		auto& ret = *widget; // Manoeuvre to return T
		SetChild(std::unique_ptr<Widget>(std::move(widget)));
		return ret;
	}

	Widget& GetChild(size_t no) const override;
	ChildInLayout GetChildInLayout(LayoutApi& api, size_t no, Vector size, Vector cursor) const override;
	void OnLayout(LayoutApi& api) override;

  protected:
	std::unique_ptr<Widget> m_content;
};


class Container : public Widget // Widget with many children
{
  public:
	virtual Widget& AddChild(std::unique_ptr<Widget> widget) = 0;

  protected:
	Container() = default;
};


class BoxFriend;
class Box : public Container
{
  public:
	enum class Direction : uint8_t
	{
		Horizontal,
		Vertical
	};

	Box(Direction direction, bool wrap = false);

	size_t GetChildrenNo() const override;
	Widget& AddChild(std::unique_ptr<Widget> widget) override;

	template <typename T, typename... ARGS> T& AddNewChild(ARGS&&... args)
	{
		auto widget = std::make_unique<T>(std::forward<ARGS>(args)...);
		auto& ret = *widget;
		AddChild(std::unique_ptr<Widget>(std::move(widget)));
		return ret;
	}

	Widget& GetChild(size_t no) const override;
	ChildInLayout GetChildInLayout(LayoutApi& api, size_t no, Vector size, Vector cursor) const override;
	void OnLayout(LayoutApi& api) override;
	void OnDraw(DrawApi& api, Rect allowed_draw_area) override;

	Box& SetWrap(bool);
	bool GetWrap() const;

  protected:
	friend BoxFriend; // :)

	Vector m_updated_layout_at;

	struct ChildEntry
	{
		std::unique_ptr<Widget> widget;
		size_t row;
		size_t position; // In row

		int stretch;           // This one is tricky, it's only defined
		                       // on entries with 'position == 0'
		int stretch_remainder; // This one as well!
	};

	Direction m_direction;
	std::vector<ChildEntry> m_children;
	bool m_wrap : 1;
};


class HBox : public Box
{
  public:
	HBox(bool wrap = false);
};

class VBox : public Box
{
  public:
	VBox(bool wrap = false);
};


class Text : public Leaf
{
  public:
	Text(std::string text);
	void OnDraw(DrawApi& api, Rect allowed_draw_area) override;
	void OnLayout(LayoutApi& api) override;

  private:
	std::string m_text;
};


class Button : public Frame
{
  public:
	enum class Style : uint8_t
	{
		Normal,
		Flat // Without bevel except if cursor is on top
	};

	enum class State : uint8_t
	{
		Normal,
		Over,
		Pressed
	};

	Button(Style style = Style::Normal, EventPropagation default_propagation = EventPropagation::TakePlaceAsTarget);
	Widget::ChildInLayout GetChildInLayout(LayoutApi& api, size_t, Vector size, Vector cursor) const override;
	void OnDraw(DrawApi& api, Rect allowed_draw_area) override;
	EventPropagation OnMouse(MouseGesture gesture, Vector, uint32_t&, const Widget& target) override;

  protected:
	Style m_style;
	State m_state;
	bool m_pressed : 1;
	bool m_inside : 1;
	EventPropagation m_default_propagation;
};


class ButtonWithText : public Button
{
  public:
	ButtonWithText(const std::string& text, Style style = Style::Normal,
	               EventPropagation default_propagation = EventPropagation::TakePlaceAsTarget);
};


class ScreenFriend;
class Screen
{
  public:
	void Initialise(uint32_t r_mask, uint32_t g_mask, uint32_t b_mask);
	void Deinitialise() noexcept;
	Rect Draw(Vector size, uint32_t* out);

	void MousePress(Vector cursor_pos);
	void MouseRelease(Vector cursor_pos);
	void MouseMoves(Vector cursor_pos);

	Frame& GetRoot();

  private:
	friend ScreenFriend; // :)

	Vector m_size;
	uint32_t* m_out;
	uint32_t m_dummy;

	uint32_t m_masks[3];

	class Root final : public Frame
	{
	  public:
		Root() = default;
	};

	Root* m_root; // A pointer, so it can survive a memset and being in a C struct

	static constexpr size_t STACK_LEN = 512; // TODO, hardcoded

	struct MiniTreeEntry
	{
		// CreateMiniTree()
		Widget* widget;
		MiniTreeEntry* parent;
		MiniTreeEntry* last_child;
		int depth;

		// DrawWidget()
		Rect last_draw_at;
		Rect hit_area;

		//
		bool pressed_as_target : 1;
		bool pressed_indirectly : 1;
		bool cursor_inside : 1;
	};

	MiniTreeEntry m_mini_tree[STACK_LEN]; // It has to be the same as stack
	size_t m_mini_tree_len;

	struct StackEntry
	{
		// CreateMiniTree(), DrawWidget()
		Widget* widget;
		MiniTreeEntry* mini;
		MiniTreeEntry* parent_mini;
		int depth;

		// DrawWidget()
		Rect allowed_draw_area;
	};

	StackEntry m_stack[STACK_LEN];

	uint8_t* m_font;

	bool m_draw_valid;
	bool m_layout_valid;

	void CreateMiniTree();
	void ColourConversion(uint32_t* in_out, Vector size, int stride) const;
	void Bubble(MouseGesture gesture, Vector cursor_pos, MiniTreeEntry* target);
};

} // namespace yuika
#endif
