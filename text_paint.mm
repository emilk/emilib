// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014-12 for Ghostel
//   Cleaned up as separate library 2016-02

#include "text_paint.hpp"

#include <map>

#include <loguru.hpp>

#import <CoreText/CoreText.h>
#import <Foundation/Foundation.h> // NSString

#ifndef TARGET_OS_IPHONE
#	error TARGET_OS_IPHONE not defined
#endif

#ifndef TARGET_OS_MAC
#	error TARGET_OS_MAC not defined
#endif

#if TARGET_OS_IPHONE
	#import <UIKit/UIKit.h>
	#import <UIKit/UIGraphics.h>
#else
	#import <AppKit/NSGraphicsContext.h>
	#import <AppKit/NSStringDrawing.h>
	#import <AppKit/NSAttributedString.h>
	#import <AppKit/NSParagraphStyle.h>
	#import <AppKit/NSText.h>
#endif

// ----------------------------------------------------------------------------

size_t utf8_count_chars(const char* utf8, size_t num_bytes)
{
	CHECK_LE_F(num_bytes, strlen(utf8));
	size_t count = 0;
	for (size_t i=0; i<num_bytes; ++i) {
		if ((*utf8 & 0b11000000) != 0b10000000) {
			count += 1;
		}
		utf8++;
	}
	return count;
}

NSString* utf8_to_NSString(const char* utf8)
{
	return [NSString stringWithUTF8String: utf8];
}

using namespace text_paint;

CGContextRef current_context()
{
#if TARGET_OS_IPHONE
	CGContextRef context = UIGraphicsGetCurrentContext();
#else
    NSGraphicsContext* nsGraphicsContext = [NSGraphicsContext currentContext];
    CGContextRef       context           = (CGContextRef)[nsGraphicsContext graphicsPort];
#endif
	CHECK_NOTNULL_F(context);
	return context;
}

CTTextAlignment get_alignment(const TextInfo& ti) {
	return (ti.alignment == TextAlign::LEFT  ? kCTTextAlignmentLeft  :
	        ti.alignment == TextAlign::RIGHT ? kCTTextAlignmentRight
	                                         : kCTTextAlignmentCenter);
}

id get_color(const RGBAf& color)
{
#if TARGET_OS_IPHONE
	return (__bridge id)[UIColor colorWithRed: color.r
	                       green: color.g
	                        blue: color.b
	                       alpha: color.a ].CGColor;
#else
	return (__bridge id)CGColorCreateGenericRGB(color.r, color.g, color.b, color.a);
#endif
}

NSDictionary* get_attributes(const TextInfo& ti, bool ignore_text_align)
{
	NSString* font_name = utf8_to_NSString(ti.font.c_str());
	CTFontRef font = CTFontCreateWithName((CFStringRef)font_name, ti.font_size, nullptr);
	id font_id = (__bridge id)font;

	NSNumber* underline = [NSNumber numberWithInt:kCTUnderlineStyleNone];

	NSDictionary* attributes;

	if (ignore_text_align || ti.alignment == TextAlign::LEFT) {
		attributes = [NSDictionary dictionaryWithObjectsAndKeys:
			font_id,                   (id)kCTFontAttributeName,
			underline,                 (id)kCTUnderlineStyleAttributeName,
			nil];
	} else {
		CTTextAlignment alignment = get_alignment(ti);
		CTParagraphStyleSetting alignmentSetting;
		alignmentSetting.spec = kCTParagraphStyleSpecifierAlignment;
		alignmentSetting.valueSize = sizeof(alignment);
		alignmentSetting.value = &alignment;
		CTParagraphStyleRef paragraphRef = CTParagraphStyleCreate(&alignmentSetting, 1);

		attributes = [NSDictionary dictionaryWithObjectsAndKeys:
			font_id,                   (id)kCTFontAttributeName,
			underline,                 (id)kCTUnderlineStyleAttributeName,
			(__bridge id)paragraphRef, (id)kCTParagraphStyleAttributeName,
			nil];

		CFRelease(paragraphRef);
	}

	CFRelease(font);

	return attributes;
}

NSAttributedString* get_attr_string(const TextInfo& ti, const AttributeString& attrib_str, bool ignore_text_align)
{
	NSString* str = utf8_to_NSString(attrib_str.utf8.c_str());
	auto attributes = get_attributes(ti, ignore_text_align);
	auto ns_attrib_str = [[NSMutableAttributedString alloc] initWithString:str attributes:attributes];

	// NSMutableAttributedString assumes ranges of characters, not over bytes:
	size_t start_bytes = 0;
	size_t start_chars = 0;
	for (const auto& part : attrib_str.colors) {
		id color = get_color(part.color);
		auto length_chars = utf8_count_chars(attrib_str.utf8.c_str() + start_bytes, part.length_bytes);
		[ns_attrib_str addAttribute:(id)kCTForegroundColorAttributeName value:color range:NSMakeRange(start_chars, length_chars)];
		start_bytes += part.length_bytes;
		start_chars += length_chars;
	}

	for (const auto& part : attrib_str.fonts) {
		NSString* font_name = utf8_to_NSString(part.font.c_str());
		CTFontRef font = CTFontCreateWithName((CFStringRef)font_name, ti.font_size, nullptr);
		id font_id = (__bridge id)font;

		size_t begin_chars = utf8_count_chars(attrib_str.utf8.c_str(), part.begin);
		size_t end_chars = utf8_count_chars(attrib_str.utf8.c_str(), part.end);
		[ns_attrib_str addAttribute:(id)kCTFontAttributeName value:font_id range:NSMakeRange(begin_chars, end_chars - begin_chars)];
	}

	return ns_attrib_str;
}

Vec2 text_paint::text_size(const TextInfo& ti, const AttributeString& attrib_str)
{
	auto max_size = CGSizeMake(ti.max_size.x, ti.max_size.y);
	bool ignore_text_align = true; // Can't specify TextAlign when figuring out the size.
	auto ns_attrib_str = get_attr_string(ti, attrib_str, ignore_text_align);
	CGRect rect = [ns_attrib_str boundingRectWithSize: max_size
	                                          options: NSStringDrawingOptions(NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading)
	                                          context: nil];
	return {(float)rect.size.width, (float)rect.size.height};
}

void draw_text(const CGContextRef&    context,
               const Vec2&            pos,
               const TextInfo&        ti,
               const AttributeString& str)
{
	bool ignore_text_align = false;
	auto ns_attrib_str = get_attr_string(ti, str, ignore_text_align);

#if 0
	// Flip the Y axis:
	CGContextSetTextMatrix(context, CGAffineTransformIdentity);
	auto screen_height = CGBitmapContextGetHeight(context);
	CGContextTranslateCTM(context, 0, screen_height);
	CGContextScaleCTM(context, 1.0, -1.0);
#endif

	// Create a path which bounds the area where you will be drawing text.
	// The path need not be rectangular.
	CGMutablePathRef path = CGPathCreateMutable();

	// In this simple example, initialize a rectangular path.
	CGRect bounds = CGRectMake(pos.x, pos.y, ti.max_size.x, ti.max_size.y);
	CGPathAddRect(path, nullptr, bounds );

	// Create the framesetter with the attributed string.
	CTFramesetterRef framesetter =
         CTFramesetterCreateWithAttributedString((CFAttributedStringRef)ns_attrib_str);

	// Create a frame.
	CTFrameRef frame = CTFramesetterCreateFrame(framesetter,
          CFRangeMake(0, 0), path, nullptr);

	// Draw the specified frame in the given context.
	CTFrameDraw(frame, context);
}

void text_paint::draw_text(uint8_t* bytes, size_t width, size_t height, bool rgba,
                           const Vec2& pos, const TextInfo& ti, const AttributeString& str)
{
	CHECK_F(std::ceil(pos.x + ti.max_size.x) <= width && std::ceil(pos.y + ti.max_size.y) <= height,
	    "The target must be large enough to fit draw area (pos + max_size)");

	CGColorSpaceRef colorSpace;
	CGContextRef context;

	if (rgba) {
		colorSpace = CGColorSpaceCreateDeviceRGB();
		context = CGBitmapContextCreate(bytes, width, height, 8, 4 * width, colorSpace, kCGImageAlphaPremultipliedLast);
	} else {
		colorSpace = CGColorSpaceCreateDeviceGray();
		context = CGBitmapContextCreate(bytes, width, height, 8, width, colorSpace, kCGImageAlphaOnly);
	}
	CGColorSpaceRelease(colorSpace);
	CGContextSetGrayFillColor(context, 1.0f, 1.0);
	draw_text(context, pos, ti, str);
	CGContextRelease(context);
}

bool text_paint::test()
{
	// Create the text to draw:
	AttributeString str;
	str.append("Hello ", text_paint::RGBAf{0,0,0,1}); // Black
	str.append("World!", text_paint::RGBAf{1,1,1,1}); // White

	// Set up how we draw the text:
	text_paint::TextInfo text_info;
	text_info.font       = "Noteworthy-Light";
	text_info.font_size  =  44;
	text_info.alignment  = TextAlign::CENTER;
	text_info.max_size.x = 100; // Break to this width.

	// Calculate the size of the text:
	const auto size_pixels_float = text_paint::text_size(text_info, str);

	// We must set max_size before calling draw_text:
	text_info.max_size = size_pixels_float;

	// Allocate render target:
	const auto    buff_width  = (size_t)std::ceil(size_pixels_float.x);
	const auto    buff_height = (size_t)std::ceil(size_pixels_float.y);
	const bool    rgba        = true;
	const uint8_t bg_color    = 0; // Transparent (if rgba=true) or black (if rgba=false)
	std::vector<uint8_t> bytes(buff_width * buff_height * (rgba ? 4 : 1), bg_color);

	// Do actual painting of text:
	text_paint::draw_text(bytes.data(), buff_width, buff_height, rgba, {0,0}, text_info, str);

	// Check that we did paint something:
	return !std::all_of(bytes.begin(), bytes.end(), [&](auto value) { return value == bg_color; });
}
