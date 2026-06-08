/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#ifndef UI_HPP
#define UI_HPP

#include "shared.hpp"

namespace Ui
{

class Wrapper;
class Container;

class Widget
{
  public:
	virtual int GetChildrenNo() const = 0;

	// 'available_size' should be parent size, in case this child needs to stretch
	virtual Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
	                         Size* natural_size = nullptr) = 0;
	virtual const Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
	                               Size* natural_size = nullptr) const = 0;

	virtual const char* GetType() const = 0;

	virtual Size UpdateLayout() = 0; // Also returns natural size

	virtual Size GetNaturalSize() const
	{
		return m_natural_size;
	}

	virtual const char* GetPrintableInformation() const
	{
		return nullptr;
	}

	virtual void Draw(Rect allowed_area) const
	{
		(void)allowed_area;
	}

	virtual DrawAPI* GetDrawApi() const
	{
		// Is not that bad to traverse upwards
		return (m_parent != nullptr) ? m_parent->GetDrawApi() : nullptr;
	}

	virtual ~Widget() = default;

	// clang-format off
	virtual Widget* SetFont(Font font) { (void)font; return this; }
	// clang-format on

	virtual Widget* SetStretch(bool x, bool y)
	{
		m_stretch_x = x, m_stretch_y = y;
		return this;
	}

	virtual bool GetStretchedX()
	{
		return m_stretch_x;
	}

	virtual bool GetStretchedY()
	{
		return m_stretch_y;
	}

  protected:
	Widget(Container* container_parent, Wrapper* wrapper_parent);

	virtual void MakeItDirty()
	{
		m_dirty = true;
		m_natural_size = {};
		if (m_parent != nullptr)
			m_parent->MakeItDirty(); // Traverses upward
	}

	Widget* m_parent;
	bool m_dirty;
	Size m_natural_size;

	bool m_stretch_x; // Most widgets should implement these
	bool m_stretch_y;
};

class Container : public Widget
{
  public:
	virtual int AppendChild(Widget* widget) = 0;

  protected:
	Container(Container* container_parent, Wrapper* wrapper_parent);
};

class Wrapper : public Widget
{
  public:
	int GetChildrenNo() const override
	{
		return 1;
	}

	virtual Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
	                         Size* natural_size = nullptr) override;
	virtual const Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
	                               Size* natural_size = nullptr) const override;
	virtual void SetChild(Widget* widget);

  protected:
	Wrapper(Container* container_parent, Wrapper* wrapper_parent);
	Widget* m_content;
};


class Window : public Wrapper
{
  public:
	static Window* Create(DrawAPI* draw_api);
	~Window();

	const char* GetType() const override;

	Size UpdateLayout() override;
	void Draw(Rect allowed_area) const override;
	DrawAPI* GetDrawApi() const override;

  private:
	Window(DrawAPI* draw_api);
	DrawAPI* m_draw_api;
};


class BoxFriend;
class Box : public Container
{
  public:
	static constexpr int MAX_CHILDREN = 32;

	enum class Direction
	{
		Horizontal,
		Vertical
	};

	static Box* Create(Container* parent, Direction direction);
	static Box* Create(Wrapper* parent, Direction direction);
	~Box();

	int GetChildrenNo() const override;
	Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr, Size* natural_size = nullptr) override;
	const Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
	                       Size* natural_size = nullptr) const override;
	int AppendChild(Widget* widget) override;
	const char* GetType() const override;
	const char* GetPrintableInformation() const override;

	Size UpdateLayout() override;

	Box* SetStretch(bool x, bool y) override;

  protected:
	Box(Container* container, Wrapper* wrapper, Direction direction);

  private:
	Direction m_direction;
	int m_children_no;
	Widget* m_children[MAX_CHILDREN];
	int m_stretched_childs;

	friend BoxFriend; // :)
};


class HBox : public Box
{
  public:
	static HBox* Create(Container* parent);
	static HBox* Create(Wrapper* parent);

  protected:
	HBox(Container* container, Wrapper* wrapper) : Box(container, wrapper, Direction::Horizontal) {}
};

class VBox : public Box
{
  public:
	static VBox* Create(Container* parent);
	static VBox* Create(Wrapper* parent);

  protected:
	VBox(Container* container, Wrapper* wrapper) : Box(container, wrapper, Direction::Vertical) {}
};


class Text : public Widget
{
  public:
	static Text* Create(Container* parent, const char* text = nullptr);
	static Text* Create(Wrapper* parent, const char* text = nullptr);

	int GetChildrenNo() const override;
	Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr, Size* natural_size = nullptr) override;
	const Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
	                       Size* natural_size = nullptr) const override;
	const char* GetType() const override;
	const char* GetPrintableInformation() const override;

	Size UpdateLayout() override;
	void Draw(Rect allowed_area) const override;

	Text* SetFont(Font font) override;

  protected:
	Text(Container* container, Wrapper* wrapper, const char* text);

  private:
	const char* m_text;
	Font m_font;
};


class Button : public Wrapper
{
  public:
	static Button* Create(Container* parent, const char* text = nullptr);
	static Button* Create(Wrapper* parent, const char* text = nullptr);
	~Button();

	const char* GetType() const override;

	Size UpdateLayout() override;
	void Draw(Rect allowed_area) const override;

	Button* SetStretch(bool x, bool y) override;

  protected:
	Button(Container* container, Wrapper* wrapper, const char* text);
};

} // namespace Ui

#endif
