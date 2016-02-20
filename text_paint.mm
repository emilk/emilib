/*
Made by Emil Ernerfeldt.
Created for Ghostel 2014-12
Cleaned up as separate library 2016-02
*/

#include "text_paint.hpp"

#include <map>

#include <loguru.hpp>

#import <Foundation/Foundation.h> // NSString

#ifndef TARGET_OS_IPHONE
	#error TARGET_OS_IPHONE must be defined as (0 or 1œ)
#endif

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

//#if !TARGET_OS_IPHONE
#if 1

#if TARGET_OS_IPHONE
	#import <UIKit/UIKit.h>
	#import <UIKit/UIGraphics.h>
	#import <UIKit/UIStringDrawing.h> // for sizeWithFont: in NSString(UIStringDrawing)
#else // !TARGET_OS_IPHONE
	#import <AppKit/NSGraphicsContext.h>
	#import <AppKit/NSStringDrawing.h>
	#import <AppKit/NSAttributedString.h>
	#import <AppKit/NSParagraphStyle.h>
	#import <AppKit/NSText.h>
#endif // !TARGET_OS_IPHONE

#import <CoreText/CoreText.h>

#if TARGET_OS_IPHONE

UIFont* create_font(const char* family, float size)
{
	LOG_F(INFO, "create_font: '%s', size: %f", family, size);

	//return [UIFont fontWithName:@"Verdana" size:fontSize];
	UIFont* ret = [UIFont fontWithName:utf8_to_NSString(family) size:size];
	if (!ret) {
		LOG_F(WARNING, "Font not supported: '%s'", family);
		ret = [UIFont systemFontOfSize:size];
	}
	return ret;
}

UIFont* get_font(const TextInfo& ti)
{
	static FontMap    s_fonts;
	static std::mutex s_mutex;

	auto&& family = ti.font;
	auto   size   = ti.font_size;

	// Buffer fonts, since '[UIFont fontWithName ...' is very slow
	using NameSize = std::pair<std::string, float>;
	using FontMap  = std::map<NameSize,     UIFont*>;
	NameSize desc{family, size};

	std::lock_guard<std::mutex> lock(s_mutex);

	UIFont* font = s_fonts[desc];

	if (font) {
		return font;
	} else {
		font = create_font(family.c_str(), size);
		s_fonts[desc] = font;
		return font;
	}
}

#endif // TARGET_OS_IPHONE

CGContextRef current_context()
{
#if TARGET_OS_IPHONE
	CGContextRef context = UIGraphicsGetCurrentContext();
#else
    NSGraphicsContext* nsGraphicsContext = [NSGraphicsContext currentContext];
    CGContextRef       context           = (CGContextRef)[nsGraphicsContext graphicsPort];
#endif
	return context;
}

CTTextAlignment get_alignment(const TextInfo& ti) {
	return (ti.alignment == TextAlign::LEFT  ? kCTTextAlignmentLeft :
			  ti.alignment == TextAlign::RIGHT ? kCTTextAlignmentRight :
			  kCTTextAlignmentCenter );
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
#if TARGET_OS_IPHONE
	//UIFont *font_id = [UIFont fontWithName:font_name size:ti.font_size]; // TODO get_font
	auto font_id = get_font(ti);
#else
	NSString* font_name = utf8_to_NSString(ti.font.c_str());
	CTFontRef font = CTFontCreateWithName((CFStringRef)font_name, ti.font_size, nullptr);
	id font_id = (__bridge id)font;
#endif

	//auto color = get_color(ti.color);

	NSNumber *underline = [NSNumber numberWithInt:kCTUnderlineStyleNone];

	NSDictionary *attributes;

	if (ti.alignment != TextAlign::LEFT) {
		CTTextAlignment alignment = get_alignment(ti);
		CTParagraphStyleSetting alignmentSetting;
		alignmentSetting.spec = kCTParagraphStyleSpecifierAlignment;
		alignmentSetting.valueSize = sizeof(alignment);
		alignmentSetting.value = &alignment;
		CTParagraphStyleRef paragraphRef = CTParagraphStyleCreate(&alignmentSetting, 1);

		// pack it into attributes dictionary
		attributes = [NSDictionary dictionaryWithObjectsAndKeys:
											 font_id,                   (id)kCTFontAttributeName,
											 //color,                     (id)kCTForegroundColorAttributeName,
											 underline,                 (id)kCTUnderlineStyleAttributeName,
											 (__bridge id)paragraphRef, (id)kCTParagraphStyleAttributeName,
											 nil];

		CFRelease(paragraphRef);
	} else {
		attributes = [NSDictionary dictionaryWithObjectsAndKeys:
											 font_id,                   (id)kCTFontAttributeName,
											 //color,                     (id)kCTForegroundColorAttributeName,
											 underline,                 (id)kCTUnderlineStyleAttributeName,
											 nil];
	}

#if !TARGET_OS_IPHONE
	CFRelease(font);
#endif

	return attributes;
}

NSAttributedString* get_attr_string(const TextInfo& ti, const ColoredString& colored_str)
{
	NSString* str = utf8_to_NSString(colored_str.utf8.c_str());
	auto attributes = get_attributes(ti);
#if 1
	auto attrib_str = [[NSMutableAttributedString alloc] initWithString:str attributes:attributes];
#else
	NSMutableParagraphStyle *paragraphStyle = [[NSMutableParagraphStyle alloc]init] ;
	[paragraphStyle setAlignment:NSTextAlignmentRight];

	auto attrib_str = [[NSMutableAttributedString alloc]initWithString:str];
	[attrib_str setAttributes: attributes range: NSMakeRange(0, [str length])];
	[attrib_str addAttribute:NSParagraphStyleAttributeName value:paragraphStyle range:NSMakeRange(0, [str length])];
#endif

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
#if TARGET_OS_IPHONE
	auto attr_str = get_attr_string(ti, colored_str);
	CGRect rect = [attr_str boundingRectWithSize: max_size
	                                     options: NSStringDrawingOptions(NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading)
	                                     context: nil];
#else
	auto attributes = get_attributes(ti);
	NSString* str = utf8_to_NSString(colored_str.utf8.c_str());
	auto rect = [str boundingRectWithSize: max_size
                                 options: NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading
										//options: 0
										attributes: attributes];
#endif
	return {(float)rect.size.width, (float)rect.size.height};
}

void draw_text(const CGContextRef&  context,
               const Vec2&          pos,
               const TextInfo&      ti,
               const ColoredString& str)
{
	auto attr_str = get_attr_string(ti, str);

#if 1
	// flip the coordinate system
	CGContextSetTextMatrix(context, CGAffineTransformIdentity);
	auto SCREEN_HEIGHT = CGBitmapContextGetHeight(context);
	CGContextTranslateCTM(context, 0, SCREEN_HEIGHT);
	CGContextScaleCTM(context, 1.0, -1.0);
#endif

#if 1
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
#else
	// draw
	CTLineRef line = CTLineCreateWithAttributedString(
		(CFAttributedStringRef)attr_str);
	CGContextSetTextPosition(context, pos.x, pos.y);
	CTLineDraw(line, context);
	CFRelease(line);
#endif
}

#else

#include <map>
#include <string>

#import <CoreGraphics/CGGeometry.h>
#import <Foundation/NSString.h>
#import <UIKit/UIFont.h>
#import <UIKit/UIGraphics.h>
#import <UIKit/UIStringDrawing.h> // for sizeWithFont: in NSString(UIStringDrawing)
//#import <CoreGraphics/CGGeometry.h>
//#import <Foundation/NSString.h>
//#import <UIKit/UIFont.h>
#import <UIKit/UIGraphics.h>
//#import <UIKit/UIStringDrawing.h>
#import <UIKit/UIKit.h>

using namespace math;
using namespace util;

using byte = unsigned char;

NSTextAlignment get_alignment(const TextInfo& ti) {
	return (ti.alignment == TextAlign::LEFT  ? NSTextAlignmentLeft :
			  ti.alignment == TextAlign::RIGHT ? NSTextAlignmentRight :
			  NSTextAlignmentCenter );
}

NSDictionary* get_attributes(const TextInfo& ti)
{
	UIFont* font = get_font(ti);
	auto color = [UIColor colorWithRed: ti.color.r
										  green: ti.color.g
											blue: ti.color.b
										  alpha: ti.color.a];

	NSMutableParagraphStyle *style = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
	style.lineBreakMode = NSLineBreakByWordWrapping;
	style.alignment     = get_alignment(ti);

	NSDictionary *attributes = @{
		NSFontAttributeName:            font,
		NSForegroundColorAttributeName: color,
		NSParagraphStyleAttributeName:  style
	};

	return attributes;
}

#if 1
Vec2 text_paint::text_size(const TextInfo& ti, const char* str)
{
	NSString* nsstr = utf8_to_NSString(str);
	CGSize maxSize = CGSizeMake(ti.max_size.x, ti.max_size.y);

#if 0
	// To create a font, we need a CGContext - so we fake one! :)
	static const int width = 4*1024, height = 2*1024; // FIXME: size?
	static std::vector<byte> s_buff(width*height);

	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
	CGContextRef context = CGBitmapContextCreate(&s_buff[0], width, height, 8, width, colorSpace, kCGImageAlphaNone);
	CGColorSpaceRelease(colorSpace);

	UIGraphicsPushContext(context);

	UIFont* font = get_font(ti);
	NSLineBreakMode lineBreak = NSLineBreakByWordWrapping;

	CGSize size = [nsstr sizeWithFont:font constrainedToSize:maxSize lineBreakMode:lineBreak];

	UIGraphicsPopContext();
	CGContextRelease(context);
	return {size.width, size.height};
#else
	auto attributes = get_attributes(ti);
	auto rect = [nsstr boundingRectWithSize: maxSize
											  options: NSStringDrawingUsesLineFragmentOrigin
										  attributes: attributes
											  context: nil];
	return {(float)rect.size.width, (float)rect.size.height};
#endif

}
#endif

void draw_text(const CGContextRef& context, const Vec2& pos, const TextInfo& ti, NSString* str)
{
	UIGraphicsPushContext(context);

	auto SCREEN_HEIGHT = CGBitmapContextGetHeight(context);

	/*
	// flip the coordinate system
	CGContextSetTextMatrix(context, CGAffineTransformIdentity);
	CGContextTranslateCTM(context, 0, SCREEN_HEIGHT);
	CGContextScaleCTM(context, 1.0, -1.0);
	 */

	CGRect rect = CGRectMake(pos.x, SCREEN_HEIGHT - pos.y - 1,
									 ti.max_size.x, ti.max_size.y);

	if (math::is_finite(ti.max_size.y)) {
		rect.origin.y -= ti.max_size.y;
	} else {
		rect.origin.y -= ti.font_size;
	}

	auto attributes = get_attributes(ti);
	[str drawInRect:rect withAttributes: attributes];

	UIGraphicsPopContext();
}

void draw_text(const CGContextRef& context, const Vec2& pos, const TextInfo& ti, const char* str)
{
	draw_text(ti, pos, context, utf8_to_NSString(str));
}

#endif

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
