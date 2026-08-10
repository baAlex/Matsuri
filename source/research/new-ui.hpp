/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <stdint.h>

class Ui
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

	void Initialise();
	void Deinitialise();
	void Update(Position mouse_pos, Size size, int stride, void* out);

	void DrawRectangle(uint32_t colour, Rect rect);

	enum class BevelStyle
	{
		Inset,
		Outset
	};

	void Draw3dBevel(Rect rect, BevelStyle style);

	class Wrapper;
	class Container;

	Wrapper* GetRoot();


	class Widget
	{
	  public:
		virtual void Deinitialise();

		virtual int GetChildrenNo() const = 0;
		virtual Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
		                         Size* child_size = nullptr) = 0;
		virtual const Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
		                               Size* child_size = nullptr) const = 0;

		virtual const char* GetType() const = 0;

		virtual Size UpdateNaturalSize() = 0; // Also returns natural size
		virtual Size GetNaturalSize() const;
		virtual Size GetSize(Size available_size) const;
		virtual void Draw(Ui* ui, Rect allowed_area) const = 0;

		virtual Widget* SetStretch(bool x, bool y);
		virtual bool GetStretchX() const;
		virtual bool GetStretchY() const;

	  protected:
		Widget* Initialise(Container* container_parent, Wrapper* wrapper_parent);
		virtual void MakeItDirty();

		Widget* m_parent;
		Size m_natural_size;

		bool m_dirty : 1;
		bool m_stretch_x : 1; // Most widgets should implement these
		bool m_stretch_y : 1;
	};


	class Wrapper : public Widget
	{
	  public:
		virtual void Deinitialise() override;

		virtual int GetChildrenNo() const override; // Always returns 1
		virtual void SetChild(Widget* widget);

		virtual Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
		                         Size* child_size = nullptr) override;
		virtual const Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
		                               Size* child_size = nullptr) const override;

		virtual Size UpdateNaturalSize() override;
		virtual void Draw(Ui* ui, Rect allowed_area) const override;

		virtual Wrapper* SetStretch(bool x, bool y) override;

	  protected:
		Wrapper* Initialise(Container* container_parent, Wrapper* wrapper_parent, Widget* child = nullptr);
		Widget* m_content;
	};


	class Container : public Widget
	{
	  public:
		virtual int AppendChild(Widget* widget) = 0;
		virtual Container* SetStretch(bool x, bool y) override;

	  protected:
		Container* Initialise(Container* container_parent, Wrapper* wrapper_parent);
	};


	class BoxFriend;
	class Box : public Container
	{
	  public:
		static constexpr int MAX_CHILDREN = 32;
		virtual void Deinitialise() override;

		enum class Direction
		{
			Horizontal,
			Vertical
		};

		static Box* Create(Container* parent, Direction direction);
		static Box* Create(Wrapper* parent, Direction direction);

		virtual int GetChildrenNo() const override;
		virtual int AppendChild(Widget* widget) override;

		virtual Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
		                         Size* child_size = nullptr) override;
		virtual const Widget* GetChild(int no, Size available_size, Delta* layout_delta = nullptr,
		                               Size* child_size = nullptr) const override;

		virtual const char* GetType() const override;

		virtual Size UpdateNaturalSize() override;
		virtual void Draw(Ui* ui, Rect allowed_area) const override;

	  protected:
		Box* Initialise(Container* container, Wrapper* wrapper, Direction direction);

	  private:
		friend BoxFriend; // :)
		Direction m_direction;
		int m_children_no;
		Widget* m_children[MAX_CHILDREN];
		int m_stretch_childs;
		Size m_non_stretch_size;
	};


	class HBox final : public Box
	{
	  public:
		static Box* Create(Container* parent);
		static Box* Create(Wrapper* parent);
	};

	class VBox final : public Box
	{
	  public:
		static Box* Create(Container* parent);
		static Box* Create(Wrapper* parent);
	};


	class Button : public Wrapper
	{
	  public:
		static Button* Create(Container* parent, const char* text = nullptr);
		static Button* Create(Wrapper* parent, const char* text = nullptr);

		virtual const char* GetType() const override;
		virtual void Draw(Ui* ui, Rect allowed_area) const override;

	  protected:
		Button* Initialise(Container* container, Wrapper* wrapper, const char* text);
	};


	// ----


  private:
	uint8_t* m_out;
	int m_out_stride;
	Size m_size;
	Position m_mouse;

	uint8_t m_dummy[4];

	class Root final : public Wrapper
	{
	  public:
		void Initialise();
		const char* GetType() const override;
	};

	Root* m_root; // A pointer, so it can survive a memset and being in a struct
};
