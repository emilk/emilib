/*
Made by Emil Ernerfeldt.
Created for Ghostel 2014-12
Cleaned up as separate library 2016-02
*/

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

NSDictionary* get_attributes(const TextInfo& ti)
{
	NSString* font_name = utf8_to_NSString(ti.font.c_str());
	CTFontRef font = CTFontCreateWithName((CFStringRef)font_name, ti.font_size, nullptr);
	id font_id = (__bridge id)font;

	NSNumber* underline = [NSNumber numberWithInt:kCTUnderlineStyleNone];

	NSDictionary* attributes;

	if (ti.alignment == TextAlign::LEFT) {
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

NSAttributedString* get_attr_string(const TextInfo& ti, const ColoredString& colored_str)
{
	NSString* str = utf8_to_NSString(colored_str.utf8.c_str());
	auto attributes = get_attributes(ti);
	auto attrib_str = [[NSMutableAttributedString alloc] initWithString:str attributes:attributes];

	// NSMutableAttributedString assumes ranges of characters, not over bytes:
	size_t start_bytes = 0;
	size_t start_chars = 0;
	for (auto& part : colored_str.colors) {
		id color = get_color(part.color);
		auto length_chars = utf8_count_chars(colored_str.utf8.c_str() + start_bytes, part.length_bytes);
		[attrib_str addAttribute:(id)kCTForegroundColorAttributeName value:color range:NSMakeRange(start_chars, length_chars)];
		start_bytes += part.length_bytes;
		start_chars += length_chars;
	}

	return attrib_str;
}

Vec2 text_paint::text_size(const TextInfo& ti, const ColoredString& colored_str)
{
	auto max_size = CGSizeMake(ti.max_size.x, ti.max_size.y);
	auto attr_str = get_attr_string(ti, colored_str);
	CGRect rect = [attr_str boundingRectWithSize: max_size
	                                     options: NSStringDrawingOptions(NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading)
	                                     context: nil];
	return {(float)rect.size.width, (float)rect.size.height};
}

void draw_text(const CGContextRef&  context,
               const Vec2&          pos,
               const TextInfo&      ti,
               const ColoredString& str)
{
	auto attr_str = get_attr_string(ti, str);

	// Flip the Y axis:
	CGContextSetTextMatrix(context, CGAffineTransformIdentity);
	auto screen_height = CGBitmapContextGetHeight(context);
	CGContextTranslateCTM(context, 0, screen_height);
	CGContextScaleCTM(context, 1.0, -1.0);

	// Create a path which bounds the area where you will be drawing text.
	// The path need not be rectangular.
	CGMutablePathRef path = CGPathCreateMutable();

	// In this simple example, initialize a rectangular path.
	CGRect bounds = CGRectMake(pos.x, pos.y, ti.max_size.x, ti.max_size.y);
	CGPathAddRect(path, nullptr, bounds );

	// Create the framesetter with the attributed string.
	CTFramesetterRef framesetter =
         CTFramesetterCreateWithAttributedString((CFAttributedStringRef)attr_str);

	// Create a frame.
	CTFrameRef frame = CTFramesetterCreateFrame(framesetter,
          CFRangeMake(0, 0), path, nullptr);

	// Draw the specified frame in the given context.
	CTFrameDraw(frame, context);
}

void text_paint::draw_text(uint8_t* bytes, size_t width, size_t height, bool rgba,
                           const Vec2& pos, const TextInfo& ti, const ColoredString& str)
{
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
