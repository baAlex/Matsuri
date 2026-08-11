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

class DrawApi
{
  public:
	static constexpr uint32_t BLACK = 0xFF000000;
	static constexpr uint32_t WHITE = 0xFFFFFFFF;
	static constexpr uint32_t RED = 0xFFFF0000;
	static constexpr uint32_t GREEN = 0xFF00FF00;
	static constexpr uint32_t BLUE = 0xFF0000FF;

	static constexpr uint32_t BKG_COLOUR = 0xFFE62828;

	static constexpr uint32_t BEVEL_LIGHT_COLOUR = WHITE;
	static constexpr uint32_t BEVEL_SHADOW_COLOUR = BLACK;
	static constexpr uint32_t BEVEL_MID_COLOUR = 0xFF731414; // BKG_COLOUR / 2

	virtual void DrawRectangle(uint32_t colour, Rect rect) = 0;

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
	virtual ~Widget() = default;

	virtual size_t GetChildrenNo() const = 0;
	virtual Widget& GetChild(size_t no, Size available_size, Delta* layout_delta = nullptr,
	                         Size* child_size = nullptr) = 0;
	virtual const Widget& GetChild(size_t no, Size available_size, Delta* layout_delta = nullptr,
	                               Size* child_size = nullptr) const = 0;

	virtual const char* GetType() const = 0;

	virtual Size UpdateNaturalSize() = 0; // Also returns natural size
	virtual Size GetNaturalSize() const;
	virtual Size GetSize(Size available_size) const;
	virtual void Draw(DrawApi& api, Rect allowed_area) const = 0;

	virtual Widget& SetStretch(bool x, bool y);
	virtual bool GetStretchX() const;
	virtual bool GetStretchY() const;

  protected:
	Widget();

	Size m_natural_size;
	bool m_stretch_x; // Most widgets should implement these
	bool m_stretch_y;
};


class Wrapper : public Widget
{
  public:
	Wrapper();
	virtual ~Wrapper() noexcept override;

	virtual size_t GetChildrenNo() const override; // Always returns 1
	virtual Widget& SetChild(Widget* widget);

	virtual Widget& GetChild(size_t no, Size available_size, Delta* layout_delta = nullptr,
	                         Size* child_size = nullptr) override;
	virtual const Widget& GetChild(size_t no, Size available_size, Delta* layout_delta = nullptr,
	                               Size* child_size = nullptr) const override;

	virtual Size UpdateNaturalSize() override;
	virtual void Draw(DrawApi& api, Rect allowed_area) const override;

  protected:
	Widget* m_content;
};


class Container : public Widget
{
  public:
	virtual Widget& AddChild(Widget* widget) = 0;

  protected:
	Container() = default;
};


class BoxFriend;
class Box : public Container
{
  public:
	static constexpr size_t MAX_CHILDREN = 32;

	enum class Direction
	{
		Horizontal,
		Vertical
	};

	Box(Direction direction);
	virtual ~Box() noexcept override;

	virtual size_t GetChildrenNo() const override;
	virtual Widget& AddChild(Widget* widget) override;

	virtual Widget& GetChild(size_t no, Size available_size, Delta* layout_delta = nullptr,
	                         Size* child_size = nullptr) override;
	virtual const Widget& GetChild(size_t no, Size available_size, Delta* layout_delta = nullptr,
	                               Size* child_size = nullptr) const override;

	virtual const char* GetType() const override;

	virtual Size UpdateNaturalSize() override;
	virtual void Draw(DrawApi& api, Rect allowed_area) const override;

  protected:
	friend BoxFriend; // :)

	Direction m_direction;
	std::vector<Widget*> m_children;
	size_t m_stretch_childs;
	Size m_non_stretch_size;
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
	Button(const char* text);
	virtual const char* GetType() const override;
	virtual void Draw(DrawApi& api, Rect allowed_area) const override;
};


class ScreenFriend;
class Screen
{
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
		const char* GetType() const override;
	};

	Root* m_root; // A pointer, so it can survive a memset and being in a struct
};

} // namespace Ui
