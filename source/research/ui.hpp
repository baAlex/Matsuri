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

#include "misc.hpp"

namespace Ui
{

class DrawAPI
{
  public:
	virtual void Draw3dBevel(Rect rect) = 0;
	virtual void DrawText(Position pos, const char* text) = 0;
};


class Wrapper;
class Container;

class Widget
{
  public:
	virtual int GetChildrenNo() const = 0;
	virtual Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) = 0;
	virtual const Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) const = 0;
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

	virtual void Draw(Position pos, DrawAPI* api) const
	{
		(void)pos;
		(void)api;
	}

  protected:
	virtual void MakeItDirty()
	{
		m_dirty = true;
		m_natural_size = {};
		if (m_parent != nullptr)
			m_parent->MakeItDirty(); // Traverses upward
	}

	Widget(Container* container_parent, Wrapper* wrapper_parent);
	Widget* m_parent;
	bool m_dirty;
	Size m_natural_size;
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
	virtual void SetChild(Widget* widget) = 0;

	int GetChildrenNo() const override
	{
		return 1;
	}

  protected:
	Wrapper(Container* container_parent, Wrapper* wrapper_parent);
};


class Window : public Wrapper
{
  public:
	static Window* Create();

	Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) override;
	const Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) const override;
	void SetChild(Widget* widget) override;
	const char* GetType() const override;

	Size UpdateLayout() override;
	void Draw(Position pos, DrawAPI* api) const override;

  private:
	Window();
	Widget* m_content;
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

	int GetChildrenNo() const override;
	Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) override;
	const Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) const override;
	int AppendChild(Widget* widget) override;
	const char* GetType() const override;
	const char* GetPrintableInformation() const override;

	Size UpdateLayout() override;

  protected:
	Box(Container* container, Wrapper* wrapper, Direction direction);

  private:
	Direction m_direction;
	int m_children_no;
	Widget* m_children[MAX_CHILDREN];

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
	Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) override;
	const Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) const override;
	const char* GetType() const override;
	const char* GetPrintableInformation() const override;

	Size UpdateLayout() override;
	void Draw(Position pos, DrawAPI* api) const override;

  protected:
	Text(Container* container, Wrapper* wrapper, const char* text);

  private:
	const char* m_text;
};


class Button : public Wrapper
{
  public:
	static Button* Create(Container* parent, const char* text = nullptr);
	static Button* Create(Wrapper* parent, const char* text = nullptr);

	Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) override;
	const Widget* GetChild(int no, Delta* layout_delta = nullptr, Size* natural_size = nullptr) const override;
	void SetChild(Widget* widget) override;
	const char* GetType() const override;

	Size UpdateLayout() override;
	void Draw(Position pos, DrawAPI* api) const override;

  protected:
	Button(Container* container, Wrapper* wrapper, const char* text);

  private:
	Widget* m_content;
};

} // namespace Ui

#endif
