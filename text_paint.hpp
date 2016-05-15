/*
Made by Emil Ernerfeldt.
Created for Ghostel 2014-12
Cleaned up as separate library 2016-02

Library for drawing colored, multiline strings on iOS and OSX.

Requires Foundation and CoreText on both iOS and OSX.
Requires UIKit on iOS.
Requires AppKit on OSX.
*/

#pragma once

#include <limits>
#include <string>
#include <vector>

namespace text_paint {

struct Vec2 { float x, y; };
struct RGBAf { float r, g, b, a; };

enum class TextAlign
{
	LEFT,
	CENTER,
	RIGHT
};

// Multiline text where ranges can be colored differently.
struct ColoredString
{
	struct ColorRange
	{
		RGBAf  color;
		size_t length_bytes; // Use this color for this many bytes of utf8.
	};

	std::string utf8;
	std::vector<ColorRange> colors;

	void append(const std::string& str, RGBAf color = {1, 1, 1, 1})
	{
		auto pre_size = utf8.size();
		utf8 += str;
		colors.push_back(ColorRange{color, utf8.size() - pre_size});
	}

	ColoredString() {}
	ColoredString(const std::string& str, RGBAf color = {1, 1, 1, 1})
	{
		append(str, color);
	}
};

struct TextInfo
{
	std::string font      = "Noteworthy-Light";
	float       font_size = 22;
	TextAlign   alignment = TextAlign::LEFT;

	// Use max_size.x to set a max width for wrapping the text to.
	Vec2        max_size  = {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
};

/* Returns how much space the given text will take up.
   If max_size.x is set, it will use it as the width to break the text to.
   Use the results as max_size when calling draw_text.
   To figure out the minimum size of the draw target you should round up the returned size. */
Vec2 text_size(const TextInfo& ti, const ColoredString& str);

/*
This function does not care about retina, i.e. pixel==point.
If `rgba`, the given buffer should be width * height * 4 bytes.
If `!rgba`, the given buffer should be width * height bytes.
max_size must be less than width/height of buffer.
The text will be drawn inside a rectangle starting at pos and ending at pos + ti.max_size.
The output image will be be written top-left to bottom-right, row by row.
*/
void draw_text(uint8_t* bytes, size_t width, size_t height, bool rgba,
               const Vec2& pos, const TextInfo& ti, const ColoredString& str);

// Should return true, unless something is broken.
bool test();

} // namespace text_paint
