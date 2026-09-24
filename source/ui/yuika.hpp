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

struct Position
{
	int x, y;
};

struct Delta
{
	int x, y;
};

struct Size
{
	int w, h;
};

struct Rect
{
	Position pos;
	Size size;
};

enum class MouseGesture
{
	Press,   // Primary button, triggers if cursor is on top of widget
	Release, // Primary button, triggers only if widget was previously
	         // pressed, it does it regardless of where cursor is

	Click, // Similar to release, except it checks if cursor is over widget

	Enters, // Enters the widget
	Leaves, // Guess it
};

enum class EventPropagation
{
	KeepPassingIt,
	StopIt
};

class SimpleApi
{
  public:
	virtual Size TextSize(const char* text) = 0;
};

class DrawApi : public SimpleApi
{
  public:
	using Colour = uint32_t;

	static constexpr Colour BLACK = 0xFF000000;
	static constexpr Colour WHITE = 0xFFFFFFFF;
	static constexpr Colour RED = 0xFFFF0000;
	static constexpr Colour GREEN = 0xFF00FF00;
	static constexpr Colour BLUE = 0xFF0000FF;
	static constexpr Colour PINK = 0xFFFF00FF;
	static constexpr Colour BACKGROUND = 0xFFD4D0C8;
	static constexpr Colour BEVEL_MID = 0xFF808080;
	static constexpr Colour BEVEL_SHADOW = 0xFF404040;
	static constexpr Colour BEVEL_LIGHT = WHITE;

	virtual void SetHitArea(Rect rect) = 0;
	virtual void DrawRectangle(Colour colour, Rect rect) = 0;
	virtual void DrawCheckerBoardRectangle(Colour colour, Rect rect) = 0;

	enum class BevelStyle
	{
		Inset,
		Outset
	};

	virtual void Draw3dBevel(Rect rect, BevelStyle style) = 0;
	virtual void DrawText(Colour colour, Position pos, const char* text) = 0;
};


class Widget
{
  public:
	Widget();
	virtual ~Widget() noexcept = default; // C++ quirk
	// Edit, is more of a logical thing:
	// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c35-a-base-class-destructor-should-be-either-public-and-virtual-or-protected-and-non-virtual

	struct ChildGet
	{
		Widget& child;
		Delta layout_delta;
		Size child_size;
	};

	virtual size_t GetChildrenNo() const = 0;
	virtual ChildGet GetChild(size_t no, Size available_size) = 0;             // Throws if there is no child
	virtual const ChildGet GetChild(size_t no, Size available_size) const = 0; // Ditto
	virtual Widget& GetChild(size_t no) = 0;                                   // Ditto
	virtual const Widget& GetChild(size_t no) const = 0;                       // Ditto

	virtual Size UpdateNaturalSize(SimpleApi& api) = 0; // Also returns natural size
	virtual Size GetNaturalSize() const;
	virtual Size GetSize(Size available_size) const;
	virtual void Draw(DrawApi& api, Rect allowed_draw_area) const;

	virtual Widget& SetStretch(bool x, bool y);
	virtual bool GetStretchX() const;
	virtual bool GetStretchY() const;

	void SetId(const char* id);
	const char* GetId() const;
	void SetDirty(bool value);
	bool GetDirty() const;

	virtual EventPropagation OnMouse(MouseGesture gesture, Position cursor_pos, const Widget& target);

  protected:
	Size m_natural_size = {};        // Most widgets should follow/implement this,
	bool m_stretch_x : 1;            // this,
	bool m_stretch_y : 1;            // this,
	bool m_natural_size_updated : 1; // and this

  private:
	bool m_dirty : 1;
	const char* m_id;
};


class Wrapper : public Widget
{
  public:
	size_t GetChildrenNo() const override; // Always returns 1
	Widget& SetChild(std::unique_ptr<Widget> widget);

	template <typename T, typename... ARGS> T& SetNewChild(ARGS&&... args)
	{
		auto widget = std::make_unique<T>(std::forward<ARGS>(args)...);
		auto& ret = *widget; // Manoeuvre to return T
		SetChild(std::unique_ptr<Widget>(std::move(widget)));
		return ret;
	}

	ChildGet GetChild(size_t no, Size available_size) override;
	const ChildGet GetChild(size_t no, Size available_size) const override;
	Widget& GetChild(size_t no) override;
	const Widget& GetChild(size_t no) const override;

	Size UpdateNaturalSize(SimpleApi& api) override;

  protected:
	std::unique_ptr<Widget> m_content;
};


class Container : public Widget
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
	enum class Direction
	{
		Horizontal,
		Vertical
	};

	Box(Direction direction);

	size_t GetChildrenNo() const override;
	Widget& AddChild(std::unique_ptr<Widget> widget) override;

	template <typename T, typename... ARGS> T& AddNewChild(ARGS&&... args)
	{
		auto widget = std::make_unique<T>(std::forward<ARGS>(args)...);
		auto& ret = *widget;
		AddChild(std::unique_ptr<Widget>(std::move(widget)));
		return ret;
	}

	ChildGet GetChild(size_t no, Size available_size) override;
	const ChildGet GetChild(size_t no, Size available_size) const override;
	Widget& GetChild(size_t no) override;
	const Widget& GetChild(size_t no) const override;

	Size UpdateNaturalSize(SimpleApi& api) override;

  protected:
	friend BoxFriend; // :)

	Direction m_direction = Direction::Horizontal;
	std::vector<std::unique_ptr<Widget>> m_children;
	size_t m_stretch_childs = 0;
	Size m_non_stretch_size = {};
};


class HBox : public Box
{
  public:
	HBox();
};

class VBox : public Box
{
  public:
	VBox();
};


class Text : public Widget
{
  public:
	Text(std::string text);
	void Draw(DrawApi& api, Rect allowed_draw_area) const override;

	size_t GetChildrenNo() const override;
	ChildGet GetChild(size_t, Size) override;
	const ChildGet GetChild(size_t, Size) const override;
	Widget& GetChild(size_t) override;
	const Widget& GetChild(size_t) const override;
	Size UpdateNaturalSize(SimpleApi& api) override;

  private:
	std::string m_text;
};


class Button : public Wrapper
{
  public:
	Button();
	void Draw(DrawApi& api, Rect allowed_draw_area) const override;
};


class ButtonWithText : public Button
{
  public:
	ButtonWithText(const std::string& text)
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
	Rect Draw(Size size, uint32_t* out);

	void MousePress(Position cursor_pos);
	void MouseRelease(Position cursor_pos);
	void MouseMoves(Position cursor_pos);

	Wrapper& GetRoot();

  private:
	friend ScreenFriend; // :)

	Size m_size;
	uint32_t* m_out;
	uint32_t m_dummy;

	uint32_t m_masks[3];

	class Root final : public Wrapper
	{
	  public:
		Root() = default;
	};

	Root* m_root; // A pointer, so it can survive a memset and being in a C struct

	static constexpr size_t STACK_LEN = 256; // TODO, hardcoded

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

	Rect m_dirty_area;
};

} // namespace yuika
#endif
