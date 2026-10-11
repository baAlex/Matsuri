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
	StopIt
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
		Vector child_size;   // Includes child margin
		Vector next_offset;  // Relative from parent
	};

	virtual size_t GetChildrenNo() const = 0;
	virtual ChildInLayout GetChildInLayout(LayoutApi& api, size_t no, Vector size,
	                                       Vector cursor) const = 0; // Throws if there is no child
	virtual Widget& GetChild(size_t no) const = 0;                   // Ditto

	virtual Vector UpdateNaturalSize(LayoutApi& api) = 0; // Also returns natural size
	virtual Vector GetNaturalSize() const;                // Includes margin
	virtual Vector GetSize(Vector available_size) const;

	virtual void OnDraw(DrawApi& api, Rect allowed_draw_area) const; // On- prefixed methods are called from screen,
	                                                                 // widgets themselves shouldn't call these

	virtual Widget& SetStretch(bool x, bool y);
	virtual bool GetStretchX() const;
	virtual bool GetStretchY() const;

	Widget& SetId(const char* id);
	const char* GetId() const;
	Widget& SetClassId(const char* id); // A la CSS
	const char* GetClassId() const;
	Widget& SetInvalidated(bool value);
	bool GetInvalidated() const;

	virtual EventPropagation OnMouse(MouseGesture gesture, Vector cursor_pos, const Widget& target);

  protected:
	Vector m_natural_size;           // Most widgets should follow/implement this,
	bool m_stretch_x : 1;            // this,
	bool m_stretch_y : 1;            // this,
	bool m_natural_size_updated : 1; // and this

  private:
	bool m_invalidated : 1;
	const char* m_id;
	const char* m_class_id;
};


class Leaf : public Widget // Widget without children
{
  public:
	size_t GetChildrenNo() const override; // Always returns zero
	ChildInLayout GetChildInLayout(LayoutApi&, size_t, Vector, Vector) const override;
	Widget& GetChild(size_t) const override;
	Vector UpdateNaturalSize(LayoutApi&) override;
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

	ChildInLayout GetChildInLayout(LayoutApi& api, size_t no, Vector size, Vector cursor) const override;
	Widget& GetChild(size_t no) const override;
	Vector UpdateNaturalSize(LayoutApi& api) override;

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

	ChildInLayout GetChildInLayout(LayoutApi& api, size_t no, Vector size, Vector cursor) const override;
	Widget& GetChild(size_t no) const override;
	Vector UpdateNaturalSize(LayoutApi& api) override;

	Box& SetWrap(bool);
	bool GetWrap() const;

  protected:
	friend BoxFriend; // :)

	Direction m_direction;
	std::vector<std::unique_ptr<Widget>> m_children;
	size_t m_stretch_childs;
	Vector m_non_stretch_size;
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


class Text : public Widget
{
  public:
	Text(std::string text);
	void OnDraw(DrawApi& api, Rect allowed_draw_area) const override;

	size_t GetChildrenNo() const override;
	ChildInLayout GetChildInLayout(LayoutApi&, size_t, Vector, Vector) const override;
	Widget& GetChild(size_t) const override;
	Vector UpdateNaturalSize(LayoutApi& api) override;

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

	Button(Style style = Style::Normal);
	void OnDraw(DrawApi& api, Rect allowed_draw_area) const override;
	EventPropagation OnMouse(MouseGesture gesture, Vector, const Widget& target) override;
	Widget::ChildInLayout GetChildInLayout(LayoutApi& api, size_t, Vector size, Vector cursor) const override;

  protected:
	Style m_style;
	State m_state;
	bool m_pressed : 1;
	bool m_inside : 1;
};


class ButtonWithText : public Button
{
  public:
	ButtonWithText(const std::string& text, Style style = Style::Normal) : Button(style)
	{
		SetNewChild<Text>(text);
	}
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
		Widget* widget;
		MiniTreeEntry* last_child;
		MiniTreeEntry* parent;

		Rect last_draw_at;

		Rect hit_area;
		bool pressed_as_target : 1;
		bool pressed_indirectly : 1;
		bool cursor_inside : 1;
	};

	MiniTreeEntry m_mini_tree[STACK_LEN]; // It has to be the same as stack
	size_t m_mini_tree_len;

	struct StackEntry
	{
		Widget* widget;
		MiniTreeEntry* parent_mini;
		MiniTreeEntry* mini;

		Rect allowed_draw_area;
	};

	StackEntry m_stack[STACK_LEN];

	uint8_t* m_font;

	bool m_invalidated;

	void ColourConversion(uint32_t* in_out, Vector size, int stride) const;
	void Bubble(MouseGesture gesture, Vector cursor_pos, MiniTreeEntry* target);
};

} // namespace yuika
#endif
