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

// #include <functional>
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

class DrawApi
{
  public:
	enum class Colour
	{
		Black = 0,
		White = 1,
		Red = 2,
		Green = 3,
		Blue = 4,

		Background = 5,
		BevelMid = 6,
		BevelLight = White,
		BevelShadow = Black,
	};

	virtual void SetClickableArea(Rect rect) = 0;
	virtual void DrawRectangle(Colour colour, Rect rect) = 0;

	enum class BevelStyle
	{
		Inset,
		Outset
	};

	virtual void Draw3dBevel(Rect rect, BevelStyle style) = 0;
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
	virtual ChildGet GetChild(size_t no, Size available_size) = 0;             // Throws is there is no child
	virtual const ChildGet GetChild(size_t no, Size available_size) const = 0; // Ditto
	virtual Widget& GetChild(size_t no) = 0;                                   // Ditto
	virtual const Widget& GetChild(size_t no) const = 0;                       // Ditto

	virtual std::string_view GetType() const = 0;

	virtual Size UpdateNaturalSize() = 0; // Also returns natural size
	virtual Size GetNaturalSize() const;
	virtual Size GetSize(Size available_size) const;
	virtual void Draw(DrawApi& api, Rect allowed_area) const;

	virtual Widget& SetStretch(bool x, bool y);
	virtual bool GetStretchX() const;
	virtual bool GetStretchY() const;

	virtual void SetReceivingEvents(uint32_t events);
	virtual uint32_t GetReceivingEvents() const;
	// virtual void OnMouseClick(MouseClickGesture gesture, Position mouse_pos);

  protected:
	Size m_natural_size = {};
	uint32_t m_receiving_events = 0;
	bool m_stretch_x : 1; // Most widgets should implement these
	bool m_stretch_y : 1;
	bool m_natural_size_updated : 1;
};


class Wrapper : public Widget
{
  public:
	size_t GetChildrenNo() const override; // Always returns 1
	Widget& SetChild(std::unique_ptr<Widget> widget);

	template <typename T, typename... ARGS> T& SetChild(ARGS&&... args)
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

	Size UpdateNaturalSize() override;

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

	template <typename T, typename... ARGS> T& AddChild(ARGS&&... args)
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

	std::string_view GetType() const override;

	Size UpdateNaturalSize() override;

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


class Button : public Wrapper
{
  public:
	// using MouseClickCallback = void(Button& self, MouseClickGesture gesture, Position mouse_pos);

	Button(std::string text);
	std::string_view GetType() const override;
	void Draw(DrawApi& api, Rect allowed_area) const override;

	// void OnMouseClick(MouseClickGesture gesture, Position mouse_pos) override;
	// void SetMouseClickCallback(std::function<MouseClickCallback> callback);

	std::string m_text; // TODO, create a label

  protected:
	// std::function<MouseClickCallback> m_mouse_click_callback;
};


class ScreenFriend;
class Screen
{
  public:
	void Initialise(uint32_t r_mask, uint32_t g_mask, uint32_t b_mask);
	void Deinitialise() noexcept;
	void Update(Size size, uint32_t* out);

	Wrapper& GetRoot();

  private:
	friend ScreenFriend; // :)

	Size m_size;
	uint32_t* m_out;
	uint32_t m_dummy;
	uint32_t m_palette[7];

	class Root final : public Wrapper
	{
	  public:
		Root() = default;
		std::string_view GetType() const override;
	};

	Root* m_root; // A pointer, so it can survive a memset and being in a C struct
};

} // namespace yuika
#endif
