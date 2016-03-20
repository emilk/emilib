#pragma once

#include <memory>
#include <utility>

#if __APPLE__
    #include "TargetConditionals.h"
#endif

// ----------------------------------------------------------------------------

#ifndef TARGET_OS_IPHONE
#	error TARGET_OS_IPHONE not defined
#endif

#ifndef TARGET_OS_MAC
#	error TARGET_OS_MAC not defined
#endif

#if TARGET_OS_IPHONE

	#define GLLIB_GLES                 1
	#define GLLIB_OPENGL_VERSION       200
	#define GLLIB_TRILLINEAR_FILTERING 1

#else // !TARGET_OS_IPHONE

	#define GLLIB_GLES                 0
	#define GLLIB_OPENGL_VERSION       320 // 210, 320, 330, 410, ...
	#define GLLIB_TRILLINEAR_FILTERING 1

#endif // !TARGET_OS_IPHONE

// ----------------------------------------------------------------------------

namespace gl {

class Program;
typedef std::unique_ptr<Program> Program_UP;
typedef std::shared_ptr<Program> Program_SP;

class Texture;
typedef std::unique_ptr<Texture> Texture_UP;
typedef std::shared_ptr<Texture> Texture_SP;

struct VertComp;
class VertexFormat;

class VBO;
typedef std::unique_ptr<VBO> VBO_UP;
typedef std::shared_ptr<VBO> VBO_SP;

class VAO;
typedef std::unique_ptr<VAO> VAO_UP;
typedef std::shared_ptr<VAO> VAO_SP;

class MeshPainter;
typedef std::unique_ptr<MeshPainter> MeshPainter_UP;
typedef std::shared_ptr<MeshPainter> MeshPainter_SP;

template<typename Vertex>
class TriangleStrip;

class FBO;
typedef std::unique_ptr<FBO> FBO_UP;
typedef std::shared_ptr<FBO> FBO_SP;

// FOr VBO:s and the like
enum class Usage
{
	WRITE_ONCE_READ_MANY,
	WRITE_MANY_READ_MANY,
	WRITE_ONCE_READ_ONCE,
};

struct Size
{
	unsigned x, y;
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
	Alpha8,  // One byte
	Red8,    // One byte
	RGB24,   // Three bytes
	RGBA32,  // Four bytes
	BGRA32,  // GL_BGRA - Four bytes
	AlphaHF, // 16-bit half-float, alpha channel only.
	RGBAHF,  // RGBA Half-float
	RGBAf,   // 32bit float RGBA

	// Avilable as render-target:
	//RGBA16F_EXT                                  0x881A
	//RGB16F_EXT                                   0x881B
	//RG16F_EXT                                    0x822F
	//R16F_EXT
};

// byte size per pixel
constexpr int format_size(ImageFormat format) {
	return (format == ImageFormat::Alpha8 ? 1 : 4);
}

constexpr bool is_half(ImageFormat f)
{
	return f==ImageFormat::AlphaHF || f==ImageFormat::RGBAHF;
}

// ------------------------------------------------

enum class TexFilter {
	Nearest,
	Linear,
	Mipmapped,
	DontCare,    // Best based on size
};

enum class WrapMode {
	Repeat,
	Mirror,
	Clamp,
	DontCare,
};

struct TexParams {
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

	friend bool operator<(const TexParams& a, const TexParams& b) {
		if (a.filter != b.filter) { return a.filter < b.filter; }
		if (a.wrap   != b.wrap)   { return a.wrap   < b.wrap; }
		return false; // Same
	}
};

// ------------------------------------------------

void check_for_gl_error(const char* file, int line);

#ifndef NDEBUG
	#define CHECK_FOR_GL_ERROR gl::check_for_gl_error(__FILE__, __LINE__)
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

#define NAME_PAINT_GROUP(name) gl::PaintGrouper LOGURU_ANONYMOUS_VARIABLE(paint_scope_)(name)

#define NAME_PAINT_FUNCTION() NAME_PAINT_GROUP(__PRETTY_FUNCTION__)

// ------------------------------------------------

} // gl
