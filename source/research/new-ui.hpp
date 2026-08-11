/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

namespace Ui
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

struct Colour
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

static constexpr Colour BLACK = {0xFF, 0x00, 0x00, 0x00};
static constexpr Colour WHITE = {0xFF, 0xFF, 0xFF, 0xFF};
static constexpr Colour RED = {0xFF, 0xFF, 0x00, 0x00};
static constexpr Colour GREEN = {0xFF, 0x00, 0xFF, 0x00};
static constexpr Colour BLUE = {0xFF, 0x00, 0x00, 0xFF};

static constexpr Colour BKG_COLOUR = {0xFF, 0xE6, 0x28, 0x28};

static constexpr Colour BEVEL_LIGHT_COLOUR = WHITE;
static constexpr Colour BEVEL_SHADOW_COLOUR = BLACK;
static constexpr Colour BEVEL_MID_COLOUR = {0xFF, 0x73, 0x14, 0x14}; // BKG_COLOUR / 2

class DrawApi
{
  public:
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
	// TODO?, this suggestion is typical 90s OOP:
	// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c120-use-class-hierarchies-to-represent-concepts-with-inherent-hierarchical-structure-only

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
	virtual void Draw(DrawApi& api, Rect allowed_area) const = 0;

	virtual Widget& SetStretch(bool x, bool y);
	virtual bool GetStretchX() const;
	virtual bool GetStretchY() const;

  protected:
	Size m_natural_size = {};
	bool m_stretch_x : 1; // Most widgets should implement these
	bool m_stretch_y : 1;
};


class Wrapper : public Widget
{
  public:
	virtual size_t GetChildrenNo() const override; // Always returns 1
	Ui::Widget& SetChild(std::unique_ptr<Widget> widget);

	template <typename T, typename... ARGS> T& SetChild(ARGS&&... args)
	{
		auto widget = std::make_unique<T>(std::forward<ARGS>(args)...);
		auto& ret = *widget; // Manoeuvre to return T
		SetChild(std::unique_ptr<Widget>(std::move(widget)));
		return ret;
	}

	virtual ChildGet GetChild(size_t no, Size available_size) override;
	virtual const ChildGet GetChild(size_t no, Size available_size) const override;
	virtual Widget& GetChild(size_t no) override;
	virtual const Widget& GetChild(size_t no) const override;

	virtual Size UpdateNaturalSize() override;
	virtual void Draw(DrawApi& api, Rect allowed_area) const override;

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

	virtual size_t GetChildrenNo() const override;
	virtual Widget& AddChild(std::unique_ptr<Widget> widget) override;

	template <typename T, typename... ARGS> T& AddChild(ARGS&&... args)
	{
		auto widget = std::make_unique<T>(std::forward<ARGS>(args)...);
		auto& ret = *widget;
		AddChild(std::unique_ptr<Widget>(std::move(widget)));
		return ret;
	}

	virtual ChildGet GetChild(size_t no, Size available_size) override;
	virtual const ChildGet GetChild(size_t no, Size available_size) const override;
	virtual Widget& GetChild(size_t no) override;
	virtual const Widget& GetChild(size_t no) const override;

	virtual std::string_view GetType() const override;

	virtual Size UpdateNaturalSize() override;
	virtual void Draw(DrawApi& api, Rect allowed_area) const override;

  protected:
	friend BoxFriend; // :)

	Direction m_direction = Direction::Horizontal;
	std::vector<std::unique_ptr<Widget>> m_children;
	size_t m_stretch_childs = 0;
	Size m_non_stretch_size = {};
};


class HBox final : public Box
{
  public:
	HBox();
};

class VBox final : public Box
{
  public:
	VBox();
};


class Button : public Wrapper
{
  public:
	Button(std::string text);
	virtual std::string_view GetType() const override;
	virtual void Draw(DrawApi& api, Rect allowed_area) const override;
};


class ScreenFriend;
class Screen
{
	// Up to here my love for Modern C++ lasted

  public:
	void Initialise();
	void Deinitialise() noexcept;
	void Update(Position mouse_pos, Size size, int stride, void* out);

	Wrapper& GetRoot();

  private:
	friend ScreenFriend; // :)

	uint8_t* m_out;
	int m_out_stride;
	Size m_size;
	Position m_mouse;

	uint8_t m_dummy[4];

	class Root final : public Wrapper
	{
	  public:
		Root() = default;
		std::string_view GetType() const override;
	};

	Root* m_root; // A pointer, so it can survive a memset and being in a struct

	// Not sorry:
	// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c90-rely-on-constructors-and-assignment-operators-not-memset-and-memcpy
};

} // namespace Ui
