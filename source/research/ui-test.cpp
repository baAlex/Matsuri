/*

Copyright (c) 2026 Alexander Brandt

The contents of this file are subject to the terms of the
Common Development and Distribution License (the "License").
You may not use this file except in compliance with the License.

If a copy of the CDDL was not distributed with this file, You
can obtain one at https://opensource.org/license/CDDL-1.0.
*/

#include <assert.h>
#include <stdio.h>

#include "canvas.hpp"
#include "misc.hpp"

#include "ui.hpp"
using namespace Ui;


struct StackEntry
{
	const Widget* widget;
	int level;
	Position pos;
	Size size;
};

static void sPrint(const StackEntry* entry)
{
	for (int i = 0; i < entry->level; i += 1)
		printf("   ");

	if (entry->widget->GetPrintableInformation() != nullptr)
	{
		if (entry->pos.x > 1.0f / 255.0f && entry->pos.y > 1.0f / 255.0f)
		{
			printf("%s \"%s\", Size: [%.1f, %.1f], Pos: [%.1f, %.1f]\n", entry->widget->GetType(),
			       entry->widget->GetPrintableInformation(), entry->size.w, entry->size.h, entry->pos.x, entry->pos.y);
		}
		else
		{
			printf("%s \"%s\", Size: [%.1f, %.1f]\n", entry->widget->GetType(),
			       entry->widget->GetPrintableInformation(), entry->size.w, entry->size.h);
		}
	}
	else
	{
		if (entry->pos.x > 1.0f / 255.0f && entry->pos.y > 1.0f / 255.0f)
		{
			printf("%s, Size: [%.1f, %.1f], Pos: [%.1f, %.1f]\n", entry->widget->GetType(), entry->size.w,
			       entry->size.h, entry->pos.x, entry->pos.y);
		}
		else
		{
			printf("%s, Size: [%.1f, %.1f]\n", entry->widget->GetType(), entry->size.w, entry->size.h);
		}
	}
}

static void sRender(const Widget* root)
{
	constexpr int STACK_LEN = 64;
	StackEntry stack[STACK_LEN];
	int cursor = 0;

	stack[cursor++] = {root, 0, {}, root->GetNaturalSize()};

	while (cursor != 0)
	{
		cursor -= 1;
		const StackEntry* current = stack + cursor;

		// Draw
		current->widget->Draw(current->pos);
		sPrint(current);

		// Stack children
		const int children = current->widget->GetChildrenNo();
		if (cursor + children >= STACK_LEN)
		{
			printf("Too many widgets\n");
			return;
		}

		cursor += children;
		Position pos_accumulator = current->pos;

		for (int i = 0; i < children; i += 1)
		{
			Delta layout_delta = {}; // Generally widget size
			Size size = {};
			const Widget* child = current->widget->GetChild(i, &layout_delta, &size);

			stack[cursor - i - 1] = {child, current->level + 1, pos_accumulator, size};

			pos_accumulator.x += layout_delta.x;
			pos_accumulator.y += layout_delta.y;
		}
	}
}


struct TinyStackEntry
{
	Widget* widget;
	bool visited;
};

static void sUpdateLayout(Widget* root)
{
	constexpr int STACK_LEN = 64;
	TinyStackEntry stack[STACK_LEN];
	int cursor = 0;

	stack[cursor++] = {root, false};

	while (cursor != 0)
	{
		cursor -= 1;
		TinyStackEntry* current = stack + cursor;

		// Update
		if (current->visited == true)
		{
			current->widget->UpdateLayout();
			continue;
		}

		// Stack children
		const int children = current->widget->GetChildrenNo();
		if (cursor + children + 1 >= STACK_LEN)
		{
			printf("Too many widgets\n");
			return;
		}

		// Re-stack ourselves as visited
		stack[cursor++] = {current->widget, true};

		// Stack children
		for (int i = children - 1; i >= 0; i -= 1)
		{
			Widget* child = current->widget->GetChild(i);
			stack[cursor++] = {child, false};
		}
	}
}


class DrawAPIImplementation : public DrawAPI
{
	Canvas* m_canvas;

  public:
	DrawAPIImplementation(Canvas* canvas)
	{
		m_canvas = canvas;
	}

	void Draw3dBevel(Rect rect) override
	{
		m_canvas->Draw3dBevel(rect);
	}

	void DrawText(Position pos, const char* text) override
	{
		m_canvas->DrawText(pos, text, COLOUR16_RED);
	}

	Size GetTextSize(const char* text) const override
	{
		return m_canvas->GetTextSize(text);
	}
};


int main()
{
	auto canvas = Canvas::Create(640, 480, 22.0f);

	auto draw_api = DrawAPIImplementation(canvas);
	canvas->DrawRectangle({{0.0f, 0.0f}, {640.0f, 480.0f}}, COLOUR16_GREY);

	// Creating something somewhat complex, yet simple (I don't have many widgets yet)
	auto window = Window::Create(&draw_api);
	{
		auto vbox = VBox::Create(window);
		{
			auto titlebar = HBox::Create(vbox);
			{
				Button::Create(titlebar, "Microsoft Word - Document 1");
			}

			auto menu = HBox::Create(vbox);
			{
				Button::Create(menu, "File");
				Button::Create(menu, "Edit");
				Button::Create(menu, "View");
				Button::Create(menu, "Insert");
				Button::Create(menu, "Format");
				Button::Create(menu, "Tools");
				Button::Create(menu, "Table");
				Button::Create(menu, "Window");
				Button::Create(menu, "Help");
			}

			auto top_toolbar = HBox::Create(vbox);
			{
				Button::Create(top_toolbar, "0"); // New
				Button::Create(top_toolbar, "1"); // Open
				Button::Create(top_toolbar, "2"); // Save
				Text::Create(top_toolbar, "|");
				Button::Create(top_toolbar, "3"); // Print
				Button::Create(top_toolbar, "4"); // Search
				Button::Create(top_toolbar, "5"); // Spell
				Text::Create(top_toolbar, "|");
				Button::Create(top_toolbar, "6"); // Cut
				Button::Create(top_toolbar, "7"); // Copy
				Button::Create(top_toolbar, "8"); // Paste
				Button::Create(top_toolbar, "9"); // Format
				Text::Create(top_toolbar, "|");
				Button::Create(top_toolbar, "A"); // Undo
				Button::Create(top_toolbar, "B"); // Redo
			}

			auto bottom_toolbar = HBox::Create(vbox);
			{
				Button::Create(bottom_toolbar, "Normal");          // Style
				Button::Create(bottom_toolbar, "Times New Roman"); // Font
				Button::Create(bottom_toolbar, "10");              // Size
				Text::Create(bottom_toolbar, "|");
				Button::Create(bottom_toolbar, "C"); // Bold
				Button::Create(bottom_toolbar, "D"); // Cursive
				Button::Create(bottom_toolbar, "E"); // Underline
				Text::Create(bottom_toolbar, "|");
				Button::Create(bottom_toolbar, "F"); // Left
				Button::Create(bottom_toolbar, "0"); // Center
				Button::Create(bottom_toolbar, "1"); // Right
			}
		}
	}

	sUpdateLayout(window);
	sRender(window);

	SaveBMP16("test.bmp", 640, 480, canvas->GetBuffer());

	return 0;
}
