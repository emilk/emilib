// By Emil Ernerfeldt 2012-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <memory>
#include <string>
#include <utility>

#if __APPLE__
	#include "TargetConditionals.h"
#endif

// ----------------------------------------------------------------------------

#ifndef EMILIB_GL_GLES
	#if TARGET_OS_IPHONE
		#define EMILIB_GL_GLES 1
	#else
		#define EMILIB_GL_GLES 0
	#endif
#endif

#ifndef EMILIB_GL_OPENGL_VERSION
	#if EMILIB_GL_GLES
		#define EMILIB_GL_OPENGL_VERSION 200
	#elif TARGET_OS_MAC
		#define EMILIB_GL_OPENGL_VERSION 320 // 210, 320, 330, 410, ...
	#else
		#define EMILIB_GL_OPENGL_VERSION 210 // Works on Ubuntu 16.04
	#endif
#endif

#ifndef EMILIB_GL_TRILLINEAR_FILTERING
	#define EMILIB_GL_TRILLINEAR_FILTERING 1
#endif

// ----------------------------------------------------------------------------

namespace emilib {
namespace gl {

class Program;
using Program_UP = std::unique_ptr<Program>;
using Program_SP = std::shared_ptr<Program>;

struct ProgramSource
{
	std::string debug_name;
	std::string vs;
	std::string fs;
};

class Texture;
using Texture_UP = std::unique_ptr<Texture>;
using Texture_SP = std::shared_ptr<Texture>;

struct VertComp;
class VertexFormat;

class VBO;
using VBO_UP = std::unique_ptr<VBO>;
using VBO_SP = std::shared_ptr<VBO>;

class VAO;
using VAO_UP = std::unique_ptr<VAO>;
using VAO_SP = std::shared_ptr<VAO>;

class MeshPainter;
using MeshPainter_UP = std::unique_ptr<MeshPainter>;
using MeshPainter_SP = std::shared_ptr<MeshPainter>;

template<typename Vertex>
class TriangleStrip;

class FBO;
using FBO_UP = std::unique_ptr<FBO>;
using FBO_SP = std::shared_ptr<FBO>;

/// For VBO:s and the like
enum class Usage
{
	WRITE_ONCE_READ_MANY,
	WRITE_MANY_READ_MANY,
	WRITE_ONCE_READ_ONCE,
};

struct Size
{
	int x, y;
};

struct Rectangle
{
	int x, y;
	int width, height;
};

// ------------------------------------------------
// ImageFormat

enum class ImageFormat
{
	INVALID,
	Alpha8,  ///< One byte
	AlphaHF, ///< 16-bit half-float, alpha channel only.
	BGRA32,  ///< GL_BGRA - Four bytes
	Red8,    ///< One byte
	RedF32,  ///< 32bit float Red channel
	RGB24,   ///< Three bytes
	RGBA32,  ///< Four bytes
	RGBAf,   ///< 32bit float RGBA
	RGBAHF,  ///< RGBA Half-float

	Depth16,
	Depth24,
	Depth32,

	// Available as render-target:
	//RGBA16F_EXT                                  0x881A
	//RGB16F_EXT                                   0x881B
	//RG16F_EXT                                    0x822F
	//R16F_EXT
};

/// byte size per pixel
constexpr int format_size(ImageFormat format)
{
	return (format == ImageFormat::Alpha8 ? 1 : 4);
}

constexpr bool is_half(ImageFormat f)
{
	return f==ImageFormat::AlphaHF || f==ImageFormat::RGBAHF;
}

// ------------------------------------------------

enum class TexFilter
{
	Nearest,
	Linear,
	Mipmapped,
	DontCare,    ///< Best based on size
};

enum class WrapMode
{
	Repeat,
	Mirror,
	Clamp,
	DontCare,
};

struct TexParams
{
	TexFilter filter = TexFilter::Mipmapped;
	std::pair<WrapMode, WrapMode> wrap = std::make_pair(WrapMode::Clamp, WrapMode::Clamp);

	TexParams() {}
	TexParams(TexFilter f, WrapMode w) : filter(f), wrap(w,w) {}
	TexParams(TexFilter f, WrapMode u, WrapMode v) : filter(f), wrap(u,v) {}

	static TexParams clamped(TexFilter f=TexFilter::DontCare) { return TexParams(f, WrapMode::Clamp); }
	static TexParams repeated(TexFilter f=TexFilter::DontCare) { return TexParams(f, WrapMode::Repeat); }
	static TexParams clamped_nearest() { return clamped(TexFilter::Nearest); }
	static TexParams clamped_linear() { return clamped(TexFilter::Linear); }
	static TexParams clamped_mipmapped() { return clamped(TexFilter::Mipmapped); }
	static TexParams repeated_linear() { return repeated(TexFilter::Linear); }
	static TexParams repeated_mipmapped() { return repeated(TexFilter::Mipmapped); }
	static TexParams mipmapped(WrapMode u, WrapMode v) { return TexParams(TexFilter::Mipmapped, u, v); }

	friend bool operator<(const TexParams& a, const TexParams& b)
	{
		if (a.filter != b.filter) { return a.filter < b.filter; }
		if (a.wrap   != b.wrap)   { return a.wrap   < b.wrap; }
		return false; // Same
	}

	friend bool operator==(const TexParams& a, const TexParams& b)
	{
		return a.filter == b.filter && a.wrap == b.wrap;
	}
};

// ------------------------------------------------

void check_for_gl_error(const char* file, int line);

#undef CHECK_FOR_GL_ERROR
#ifndef NDEBUG
	#define CHECK_FOR_GL_ERROR ::emilib::gl::check_for_gl_error(__FILE__, __LINE__)
#else
	#define CHECK_FOR_GL_ERROR
#endif

// ----------------------------------------------------------------------------

class PaintGrouper
{
public:
	PaintGrouper(const char* name);
	PaintGrouper(const std::string& name);
	~PaintGrouper();
	PaintGrouper(PaintGrouper&) = delete;
	PaintGrouper& operator=(PaintGrouper&) = delete;
};

#define EMILIB_GL_PAINT_GROUP(name) ::emilib::gl::PaintGrouper LOGURU_ANONYMOUS_VARIABLE(paint_scope_)(name)

#define EMILIB_GL_PAINT_FUNCTION() EMILIB_GL_PAINT_GROUP(__PRETTY_FUNCTION__)

// ------------------------------------------------

} // gl
} // namespace emilib
