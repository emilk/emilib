// By Emil Ernerfeldt 2012-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "gl_lib.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

#define LOGURU_WITH_STREAMS 1
#include <loguru.hpp>

#include "gl_lib_opengl.hpp"

#ifndef EMILIB_GL_GLES
	#error EMILIB_GL_GLES not defined
#endif
#ifndef EMILIB_GL_OPENGL_VERSION
	#error EMILIB_GL_OPENGL_VERSION not defined
#endif

namespace emilib {
namespace gl {

// ----------------------------------------------------------------------------

void check_for_gl_error(const char* file, int line)
{
	//CHECK(has_gl_context()); // TODO?

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		const char* err_str;
		switch (error)
		{
			case GL_NO_ERROR:           err_str = "GL_NO_ERROR";           break;
			case GL_INVALID_ENUM:       err_str = "GL_INVALID_ENUM";       break;
			case GL_INVALID_VALUE:      err_str = "GL_INVALID_VALUE";      break;
			case GL_INVALID_OPERATION:  err_str = "GL_INVALID_OPERATION";  break;
			//case GL_STACK_OVERFLOW:   err_str = "GL_STACK_OVERFLOW";     break;
			//case GL_STACK_UNDERFLOW:  err_str = "GL_STACK_UNDERFLOW";    break;
			case GL_OUT_OF_MEMORY:      err_str = "GL_OUT_OF_MEMORY";      break;
			//case GL_TABLE_TOO_LARGE:  err_str = "GL_TABLE_TOO_LARGE";   break;
			default:                    err_str = "GL_NO_FREAKING_IDEA";
		}

		//glInsertEventMarkerEXT(0, "com.apple.GPUTools.event.debug-frame"); // Frame capture what went wrong

		loguru::log_and_abort(1, "", file, line, "GL error: %s at %s:%d", err_str, file, line);
	}
}

// Error callback
void on_gl_error(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
	const GLchar* message, const void* user_param)
{
	(void)source;
	(void)type;
	(void)id;
	(void)severity;
	(void)length;
	(void)user_param;
	LOG_F(WARNING, "GL debug: %s", message);
}

// ----------------------------------------------------------------------------

void init_glew()
{
#if !EMILIB_GL_GLES
	#ifndef GLEW_OK
	#	error "No GLEW!"
	#endif

	static bool s_initialized = false;
	if (s_initialized) { return; }
	s_initialized = true;

	LOG_SCOPE_FUNCTION(INFO);
	CHECK_FOR_GL_ERROR;

	glewExperimental = true;
	GLenum glewErr = glewInit();
	CHECK_F(glewErr == GLEW_OK, "Failed to initialize GLEW: %s", glewGetErrorString(glewErr));
	glGetError();  // glew sometimes produces faux GL_INVALID_ENUM
	CHECK_FOR_GL_ERROR;

	CHECK_NOTNULL_F(glCreateProgram);

	char gl_version_string[64];
	snprintf(gl_version_string, sizeof(gl_version_string) - 1,
		"GL_VERSION_%d_%d", EMILIB_GL_OPENGL_VERSION / 100, (EMILIB_GL_OPENGL_VERSION / 10) % 10);
	CHECK_F(glewIsSupported(gl_version_string), "Not supported: %s", gl_version_string);

	// CHECK_F(glewIsSupported("GL_VERSION_3_2"));

	if (glDebugMessageCallbackARB) {
		LOG_F(INFO, "ARB_debug_output supported");
		glDebugMessageCallbackARB(on_gl_error, nullptr);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	} else {
		LOG_F(INFO, "ARB_debug_output not supported");
	}
#endif // !EMILIB_GL_GLES

	CHECK_FOR_GL_ERROR;
}

// ----------------------------------------------------------------------------

PaintGrouper::PaintGrouper(const char* name)
{
	CHECK_FOR_GL_ERROR;

#if TARGET_OS_IPHONE
	glPushGroupMarkerEXT(0, name);
	CHECK_FOR_GL_ERROR;
#else
	(void)name;
#endif
}

PaintGrouper::PaintGrouper(const std::string& name) : PaintGrouper(name.c_str())
{
}

PaintGrouper::~PaintGrouper()
{
	CHECK_FOR_GL_ERROR;
#if TARGET_OS_IPHONE
	glPopGroupMarkerEXT();
	CHECK_FOR_GL_ERROR;
#endif
}

// ----------------------------------------------------------------------------

#if TARGET_OS_IPHONE
#   define GL_RED GL_RED_EXT
#   define GL_TEXTURE_MAX_LEVEL  GL_TEXTURE_MAX_LEVEL_APPLE
//#   define GL_TEXTURE_BASE_LEVEL GL_TEXTURE_BASE_LEVEL_APPLE
#endif

inline constexpr bool is_power_of_two(size_t k)
{
	return (k & (k-1))==0;
}

inline constexpr bool is_power_of_two(Size size)
{
	return is_power_of_two(size.x) && is_power_of_two(size.y);
}

bool supports_mipmaps_for(Size size)
{
#if EMILIB_GL_GLES
	return is_power_of_two(size.x) && is_power_of_two(size.y);
#else
	(void)size;
	return true;
#endif
}

int max_texture_size()
{
	static int s_max_size = [](){
		int size;
		CHECK_FOR_GL_ERROR;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
		CHECK_FOR_GL_ERROR;
		return size;
	}();
	return s_max_size;
}

Texture::Texture()
{
}

Texture::Texture(
	GLuint      id,
	Size        size,
	TexParams   params_arg,
	ImageFormat format,
	std::string debug_name)
	: _size(size)
	, _format(format)
	, _debug_name(std::move(debug_name))
	, _params(params_arg)
{
	EMILIB_GL_PAINT_FUNCTION();
	CHECK_FOR_GL_ERROR;

	_id = id;

	init(nullptr);
}

Texture::Texture(
	std::string debug_name,
	TexParams   params_arg,
	ImageFormat format,
	Size        size,
	const void* data)
	: _size(size)
	, _format(format)
	, _debug_name(std::move(debug_name))
	, _params(params_arg)
{
	EMILIB_GL_PAINT_FUNCTION();
	CHECK_FOR_GL_ERROR;

	glGenTextures(1, &_id);

	init(data);
}

void Texture::init(const void* data_ptr)
{
	// ------------------------------------------------
	// Check params

#if EMILIB_GL_GLES
	if (is_half(_format)) {
		// Just in case:
		// FIXME: needed?
		_params.wrap = std::make_pair(WrapMode::Clamp, WrapMode::Clamp);
		//_params.filter = TexFilter::Nearest;
		//_params.filter = TexFilter::Linear;
	}
#endif

	// ------------------------------------------------

	CHECK_FOR_GL_ERROR;

	_params_dirty = true;
	bind();

	// ------------------------------------------------

	if (data_ptr) {
		set_data(data_ptr);
	}

	CHECK_FOR_GL_ERROR;

	set_debug_name(_debug_name);

	CHECK_FOR_GL_ERROR;
}

void Texture::set_params(const TexParams& params)
{
	if (params == _params) { return; }
	_params = params;
	_params_dirty = true;
}

Texture::~Texture()
{
	free();
}

void Texture::swap(Texture& other)
{
	std::swap(_size,       other._size);
	std::swap(_format,     other._format);
	std::swap(_params,     other._params);
	std::swap(_debug_name, other._debug_name);
	std::swap(_id,         other._id);
	std::swap(_has_data,   other._has_data);
	std::swap(_bpp,        other._bpp);
}

void Texture::free()
{
	if (_id != 0) {
		EMILIB_GL_PAINT_FUNCTION();
		glDeleteTextures(1, &_id);
		_id = 0;
		_has_data = false;
	}
}

void Texture::set_debug_name(const std::string& debug_name)
{
	_debug_name = debug_name;

#if TARGET_OS_IPHONE
	if (_id) {
		glLabelObjectEXT(GL_TEXTURE, _id, 0, _debug_name.c_str());
	}
#endif
}

void Texture::set_data(const void* data_ptr, Size size, ImageFormat format)
{
	_size = size;
	_format = format;
	set_data(data_ptr);
}

void Texture::generate_mipmaps()
{
	CHECK_FOR_GL_ERROR;
	bind();
	glGenerateMipmap(GL_TEXTURE_2D);
	CHECK_FOR_GL_ERROR;
}

void Texture::set_data(const void* data_ptr)
{
	set_mip_data(data_ptr, _size, 0);

	CHECK_FOR_GL_ERROR;

	if (_params.filter == TexFilter::Mipmapped) {
		generate_mipmaps();
	}
}

void Texture::set_mip_data(const void* data_ptr, Size size, int mip_level)
{
	ERROR_CONTEXT("Texture name",   _debug_name.c_str());
	ERROR_CONTEXT("Texture width",  size.x);
	ERROR_CONTEXT("Texture height", size.y);
	ERROR_CONTEXT("mip_level",      mip_level);
	EMILIB_GL_PAINT_FUNCTION();
	bind();

	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // FIXME

	CHECK_FOR_GL_ERROR;

	GLenum src_format;
	GLint  dst_format;
	GLenum element_format;

	switch (_format)
	{
		case ImageFormat::Alpha8:
			src_format = dst_format = GL_RED;
			element_format = GL_UNSIGNED_BYTE;
			break;

		case ImageFormat::Red8:
			src_format = dst_format = GL_RED;
			element_format = GL_UNSIGNED_BYTE;
			break;

		case ImageFormat::RedF32:
			src_format = dst_format = GL_RED;
			element_format = GL_FLOAT;
			break;

		case ImageFormat::RGB24:
			src_format = dst_format = GL_RGB;
			element_format = GL_UNSIGNED_BYTE;
			break;

		case ImageFormat::RGBA32:
			src_format = dst_format = GL_RGBA;
			element_format = GL_UNSIGNED_BYTE;
			break;

		case ImageFormat::BGRA32:
			src_format = GL_BGRA;
			dst_format = GL_RGBA;
			element_format = GL_UNSIGNED_BYTE;
			break;

		case ImageFormat::AlphaHF:
			//src_format = GL_R16F_EXT; // WRONG
			//src_format = GL_RGBA16F_EXT; // WRONG
			//src_format = GL_LUMINANCE; // Doesn't work with FBO
			src_format = GL_ALPHA; // The number of components
			dst_format = src_format;
#if TARGET_OS_IPHONE
			element_format = GL_HALF_FLOAT_OES;
#else
			element_format = GL_HALF_FLOAT;
#endif
			// TODO: GL_EXT_texture_rg ?
			break;

		case ImageFormat::RGBAHF:
			src_format = GL_RGBA; // The number of components
			dst_format = src_format;
#if TARGET_OS_IPHONE
			element_format = GL_HALF_FLOAT_OES;
#else
			element_format = GL_HALF_FLOAT;
#endif
			break;

		case ImageFormat::Depth16:
			dst_format     = GL_DEPTH_COMPONENT16;
			src_format     = GL_DEPTH_COMPONENT;
			element_format = GL_FLOAT;
			break;

		case ImageFormat::Depth24:
			dst_format     = GL_DEPTH_COMPONENT24;
			src_format     = GL_DEPTH_COMPONENT;
			element_format = GL_FLOAT;
			break;

		case ImageFormat::Depth32:
			dst_format     = GL_DEPTH_COMPONENT32;
			src_format     = GL_DEPTH_COMPONENT;
			element_format = GL_FLOAT;
			break;

		default:
			ABORT_F("Unknown image format");
	}

	CHECK_FOR_GL_ERROR;

	CHECK_LE_F(size.x, max_texture_size(), "%s too large (%d x %d), max is %d", _debug_name.c_str(), size.x, size.y, max_texture_size());
	CHECK_LE_F(size.y, max_texture_size(), "%s too large (%d x %d), max is %d", _debug_name.c_str(), size.x, size.y, max_texture_size());

	glTexImage2D(GL_TEXTURE_2D, mip_level, dst_format,
					 (GLsizei)size.x, (GLsizei)size.y, 0,
					 src_format, element_format, data_ptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip_level);

	if (src_format == GL_DEPTH_COMPONENT)
	{
		// For use with PCF (percentage-close filtering) in a shader:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	CHECK_FOR_GL_ERROR;

	_has_data = true;
}

bool Texture::is_power_of_two() const
{
	return gl::is_power_of_two(_size);
}

void Texture::bind(int tu) const
{
	CHECK_NE_F(_id, 0, "Texture not loaded: '%s'", _debug_name.c_str());
	EMILIB_GL_PAINT_FUNCTION();
	glActiveTexture(GL_TEXTURE0 + tu);
	glBindTexture(GL_TEXTURE_2D, _id);

	if (_params_dirty) {
		set_filtering(_params.filter);
		set_wrap_mode(_params.wrap.first, _params.wrap.second);
		_params_dirty = false;
	}
}

void Texture::unbind(int tu) const
{
	glActiveTexture(GL_TEXTURE0 + tu);
	glBindTexture(GL_TEXTURE_2D, _id);
}

void Texture::set_wrap_mode(WrapMode s, WrapMode t) const
{
	EMILIB_GL_PAINT_FUNCTION();

	// How to map WrapModes to GL.
	auto translate_mode = [](WrapMode mode)
	{
		if (mode == WrapMode::Mirror) {
			return GL_MIRRORED_REPEAT;
		} else if (mode == WrapMode::Repeat) {
			return GL_REPEAT;
		} else {
			return GL_CLAMP_TO_EDGE;
		}
	};

#if EMILIB_GL_GLES
	// See Section 3.8.7 Texture Completeness in OpenGl ES 2.0 spec.
	if (!is_power_of_two()) {
		assert(s==WrapMode::Clamp && t==WrapMode::Clamp);
	}
#endif

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, translate_mode(s));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, translate_mode(t));
}

void Texture::set_filtering(TexFilter filter) const
{
	EMILIB_GL_PAINT_FUNCTION();

	if (filter==TexFilter::DontCare) {
#if EMILIB_GL_GLES
		if (is_power_of_two()) {
			filter = TexFilter::Mipmapped;
		} else {
			filter = TexFilter::Linear;
		}
#else
		filter = TexFilter::Mipmapped;
#endif
	}

#if EMILIB_GL_GLES
	if (!is_power_of_two()) {
		if (filter == TexFilter::Mipmapped) {
			LOG_F(WARNING, "non-power-of-two mipmaps NOT SUPPORTED! Texture: %s", _debug_name.c_str());
			filter = TexFilter::Linear;
		}

		// See Section 3.8.7 Texture Completeness in OpenGl ES 2.0 spec.
		if (_params.wrap != std::make_pair(WrapMode::Clamp, WrapMode::Clamp)) {
			LOG_F(WARNING, "non-power-of-two textures must use clamping! Texture: %s", _debug_name.c_str());
			_params.wrap = std::make_pair(WrapMode::Clamp, WrapMode::Clamp);
		}
	}
#endif

	_params.filter = filter;

	if (filter == TexFilter::Nearest) {
		// LOG_F(WARNING, "'%s' Filter: Nearest", _debug_name.c_str());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	} else {
		if (filter == TexFilter::Mipmapped) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				EMILIB_GL_TRILLINEAR_FILTERING ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
		}
		else {
			// LOG_F(WARNING, "'%s' Filter: Linear", _debug_name.c_str());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

#if TARGET_OS_IPHONE
	const bool supports_anisotropic_filtering = true;
#elif EMILIB_GL_GLES
	const bool supports_anisotropic_filtering = false;
#else
	//const bool supports_anisotropic_filtering = GLEW_EXT_texture_filter_anisotropic;
	const bool supports_anisotropic_filtering = true; // FIXME
#endif

	if (supports_anisotropic_filtering && filter == TexFilter::Mipmapped) {
		CHECK_FOR_GL_ERROR;

		float max_anisotropy = 16;
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy);

		CHECK_FOR_GL_ERROR;
	}
}

// Image32 Texture::read_rgba() const
// {
// 	auto ret = Image32(_size, _debug_name.c_str());
// 	bind();
// 	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, ret.data());
// 	unbind();
// 	return std::move(ret);
// }

// Use to override, e.g. when you know the format is compressed
void Texture::set_bits_per_pixel(int bpp)
{
	_bpp = bpp;
}

int Texture::bits_per_pixel() const
{
	if (_bpp != 0) {
		return _bpp;
	} else {
		switch (_format) {
			case ImageFormat::Alpha8:  return  8;
			case ImageFormat::AlphaHF: return 16;
			case ImageFormat::BGRA32:  return 32;
			case ImageFormat::RedF32:  return 32;
			case ImageFormat::Red8:    return  8;
			case ImageFormat::RGB24:   return 24;
			case ImageFormat::RGBA32:  return 32;
			case ImageFormat::RGBAf:   return 4 * 32;
			case ImageFormat::RGBAHF:  return 4 * 16;
			case ImageFormat::Depth16: return 16;
			case ImageFormat::Depth24: return 24;
			case ImageFormat::Depth32: return 32;
			default: ABORT_F("Unknown image format: %d", (int)_format);
		}
	}
}

size_t Texture::memory_usage() const
{
	auto bpp = bits_per_pixel();
	auto bytes = _size.x * _size.y * bpp / 8;
	if (_params.filter == TexFilter::Mipmapped) {
		bytes = bytes * 4 / 3;
	}
	return bytes;
}

// ----------------------------------------------------------------------------

// Legacy PVR 2: http://cdn.imgtec.com/sdk-documentation/PVR+File+Format.Specification.Legacy.pdf
struct PvrHeader
{
	uint32_t header_length;
	uint32_t height;
	uint32_t width;
	uint32_t mipmap_count;
	uint32_t flags;
	uint32_t data_length;
	uint32_t bpp;
	uint32_t bitmask_red;
	uint32_t bitmask_green;
	uint32_t bitmask_blue;
	uint32_t bitmask_alpha;
	char     pvr_tag[4];
	uint32_t surface_count;
};

static_assert(sizeof(PvrHeader) == 52, "");

static const uint32_t kBGRA8888 = 0x1A;
static const uint32_t kA8       = 0x1B;

Texture load_uncompressed_pvr2_from_memory(
	const void* data, size_t num_bytes,
	TexParams params, std::string debug_name)
{
	PvrHeader* header = (PvrHeader*)data;
	if (header->header_length != 52 || strncmp(header->pvr_tag, "PVR!", 4) != 0) {
		LOG_F(ERROR, "Not a PVR 2 file: '%s'", debug_name.c_str());
		return {};
	}

	uint32_t flags = header->flags;
	uint32_t format_flag = flags & 0xFF;

	const uint8_t* data_start = (const uint8_t*)data + sizeof(PvrHeader);

	ImageFormat format;
	if (format_flag == kA8) {
		format = ImageFormat::Alpha8;
	} else if (format_flag == kBGRA8888) {
		format = ImageFormat::BGRA32;
	} else {
		LOG_F(ERROR, "PVR: kBGRA8888 (%x) expected, got %x", kBGRA8888, format_flag);
		return {};
	}

	Size size{header->width, header->height};

	CHECK_GT_F(header->mipmap_count, 0);

	if (params.filter == TexFilter::Mipmapped && !supports_mipmaps_for(size)) {
		params.filter = TexFilter::Linear;
	}

	if (params.filter == TexFilter::Nearest || params.filter == TexFilter::Linear) {
		return Texture{std::move(debug_name), params, format, size, data_start};
	} else if (header->mipmap_count == 1) {
		params.filter = TexFilter::Linear;
		return Texture{std::move(debug_name), params, format, size, data_start};
	} else {
		params.filter = TexFilter::Mipmapped;
		Texture tex{std::move(debug_name), params, format, size, nullptr};

		auto bytes_per_pixel = format_size(format);

		for (unsigned level=0; level<header->mipmap_count; ++level) {
			tex.set_mip_data(data_start, size, level);

			data_start += size.x * size.y * bytes_per_pixel;
			size.x = std::max(1, size.x / 2);
			size.y = std::max(1, size.y / 2);
		}
		return tex;
	}
}

// ----------------------------------------------------------------------------

std::string prefix_with_line_numbers(const char* str)
{
	std::string result;
	size_t line_nr = 1;

	while (*str)
	{
		auto line_nr_text = loguru::strprintf("%3lu  ", line_nr);
		result += line_nr_text.c_str();
		while (const char c = *str) {
			++str;
			result += c;
			if (c == '\n') {
				++line_nr;
				break;
			}
		}
	}

	return result;
}

unsigned load_shader(GLenum type, const char* source, const char* debug_name)
{
	CHECK_FOR_GL_ERROR;

	unsigned id = glCreateShader(type);

	const char* code = source;
	glShaderSource(id, 1, &code, nullptr);
	glCompileShader(id);

	CHECK_FOR_GL_ERROR;

	GLint status = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);

	CHECK_FOR_GL_ERROR;

	//std::string log = Trim( GetShaderLog(id) );

	if (status == 0) {
		CHECK_FOR_GL_ERROR;

		LOG_F(INFO, "-------------------------------------");
		LOG_S(INFO) << "\n" << prefix_with_line_numbers(source);
		LOG_F(INFO, "-------------------------------------");

		LOG_F(ERROR, "Failed to compile %s shader for program \"%s\".",
					(type==GL_VERTEX_SHADER ? "vertex" : "fragment"), debug_name);

		GLint log_length = -1;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);

		CHECK_FOR_GL_ERROR;

		if (log_length > 0) {
			std::vector<GLchar> log((unsigned)log_length);
			glGetShaderInfoLog(id, log_length, &log_length, log.data());
			LOG_F(ERROR, "Shader log:\n%s", log.data());
		}

		CHECK_FOR_GL_ERROR;

		glDeleteShader(id);

		throw std::runtime_error("Shader compilation error");
	}

	return id;
}

void print_link_log(unsigned prog, const char* debug_name)
{
	GLint log_length;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0) {
		std::vector<GLchar> log((unsigned)log_length+1);
		glGetProgramInfoLog(prog, log_length, &log_length, &log[0]);
		LOG_F(INFO, "Program '%s' link log:\n%s", debug_name, &log[0]);
	}
}

bool link_program(unsigned prog, const char* debug_name)
{
	CHECK_FOR_GL_ERROR;

	glLinkProgram(prog);

#if defined(DEBUG)
	print_link_log(prog, debug_name);
#endif

	GLint status;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		print_link_log(prog, debug_name);
		ABORT_F("Failed to link GL program");
	}

	return true;
}

bool validate_program(GLuint prog, const char* debug_name)
{
	CHECK_FOR_GL_ERROR;

	GLint log_length, status;

	glValidateProgram(prog);
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0) {
		std::vector<GLchar> log((size_t)log_length);
		glGetProgramInfoLog(prog, log_length, &log_length, log.data());
		LOG_F(INFO, "Program '%s' validate log:\n%s", debug_name, log.data());
	}

	glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
	if (status == 0) {
		ABORT_F("Program status is zero: %s", debug_name);
	}

	return true;
}

const char* type_to_string(GLenum type)
{
	switch (type)
	{
		case GL_FLOAT:                return "GL_FLOAT";
		case GL_FLOAT_VEC2:           return "GL_FLOAT_VEC2";
		case GL_FLOAT_VEC3:           return "GL_FLOAT_VEC3";
		case GL_FLOAT_VEC4:           return "GL_FLOAT_VEC4";
		case GL_FLOAT_MAT2:           return "GL_FLOAT_MAT2";
		case GL_FLOAT_MAT3:           return "GL_FLOAT_MAT3";
		case GL_FLOAT_MAT4:           return "GL_FLOAT_MAT4";
		case GL_INT:                  return "GL_INT";
		case GL_INT_VEC2:             return "GL_INT_VEC2";
		case GL_INT_VEC3:             return "GL_INT_VEC3";
		case GL_INT_VEC4:             return "GL_INT_VEC4";
		case GL_UNSIGNED_INT:         return "GL_UNSIGNED_INT";
		//case GL_DOUBLE_MAT4x3​:        return "GL_DOUBLE_MAT4x3​";
		case GL_SAMPLER_2D:           return "GL_SAMPLER_2D";  // TODO: more of these

#if !EMILIB_GL_GLES
		case GL_DOUBLE_MAT3x4:        return "GL_DOUBLE_MAT3x4";
		case GL_DOUBLE_MAT4x2:        return "GL_DOUBLE_MAT4x2";
		case GL_FLOAT_MAT2x3:         return "GL_FLOAT_MAT2x3";
		case GL_FLOAT_MAT2x4:         return "GL_FLOAT_MAT2x4";
		case GL_FLOAT_MAT3x2:         return "GL_FLOAT_MAT3x2";
		case GL_FLOAT_MAT3x4:         return "GL_FLOAT_MAT3x4";
		case GL_FLOAT_MAT4x2:         return "GL_FLOAT_MAT4x2";
		case GL_FLOAT_MAT4x3:         return "GL_FLOAT_MAT4x3";

		case GL_UNSIGNED_INT_VEC2:    return "GL_UNSIGNED_INT_VEC2";
		case GL_UNSIGNED_INT_VEC3:    return "GL_UNSIGNED_INT_VEC3";
		case GL_UNSIGNED_INT_VEC4:    return "GL_UNSIGNED_INT_VEC4";
		case GL_DOUBLE:               return "GL_DOUBLE";
		case GL_DOUBLE_VEC2:          return "GL_DOUBLE_VEC2";
		case GL_DOUBLE_VEC3:          return "GL_DOUBLE_VEC3";
		case GL_DOUBLE_VEC4:          return "GL_DOUBLE_VEC4";
		case GL_DOUBLE_MAT2:          return "GL_DOUBLE_MAT2";
		case GL_DOUBLE_MAT3:          return "GL_DOUBLE_MAT3";
		case GL_DOUBLE_MAT4:          return "GL_DOUBLE_MAT4";
		case GL_DOUBLE_MAT2x3:        return "GL_DOUBLE_MAT2x3";
		case GL_DOUBLE_MAT2x4:        return "GL_DOUBLE_MAT2x4";
		case GL_DOUBLE_MAT3x2:        return "GL_DOUBLE_MAT3x2";
#endif

		default:                      return "UNKNOWN";
	}
}

// ---------------------------------------------------------------

Program::Program(const std::string& vs, const std::string& fs, std::string debug_name_arg)
	: _debug_name(std::move(debug_name_arg))
{
	VLOG_SCOPE_F(1, "Compiling GLSL %s", _debug_name.c_str());

	CHECK_FOR_GL_ERROR;

	GLuint vs_id = load_shader(GL_VERTEX_SHADER,   vs.c_str(), _debug_name.c_str());
	GLuint fs_id = load_shader(GL_FRAGMENT_SHADER, fs.c_str(), _debug_name.c_str());

	_program = glCreateProgram();

#if TARGET_OS_IPHONE
	// For debugger:
	glLabelObjectEXT(GL_PROGRAM_OBJECT_EXT, _program, 0, _debug_name.c_str());
#endif

	glAttachShader(_program, vs_id);
	glAttachShader(_program, fs_id);

	//GLuint color_number = 0;
	//glBindFragDataLocation(_program, color_number, "out_frag_color");

	link_program(_program, _debug_name.c_str());

	/* too early to validate: uniforms haven't been bound yet.
	   Using two samplers of different type (sampler2D and sampler_cube)
	   will break the validation.
	 */
	//validate();

	CHECK_FOR_GL_ERROR;

	//debug_print();

#if 0
	LOG_F(INFO, "Shader: %s", _debug_name.c_str());
	LOG_F(INFO, "-------------------------------------");
	LOG_F(INFO, "%s", vs.c_str());
	LOG_F(INFO, "-------------------------------------");
	LOG_F(INFO, "%s", fs.c_str());
	LOG_F(INFO, "-------------------------------------");
#endif

	GLint num_attribs;
	glGetProgramiv(_program, GL_ACTIVE_ATTRIBUTES, &num_attribs);

	for (int i=0; i<num_attribs; ++i) {
		GLint size;
		GLenum type;
		GLchar name[1024];
		glGetActiveAttrib(_program, i, sizeof(name), NULL, &size, &type, name);
		int location = glGetAttribLocation(_program, name);
		CHECK_NE_F(location, -1, "Attribute '%s' not found in shader '%s'", name, _debug_name.c_str());
		VLOG_F(1, "Attribute %d: %10s, %d x %s, location: %d", i, name, size, type_to_string(type), location);
		_attributes.emplace_back(Attribute{name, size, type, location});
	}

	GLint num_uniforms;
	glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &num_uniforms);

	for (int i=0; i<num_uniforms; ++i) {
		GLint size;
		GLenum type;
		GLchar name[1024];
		glGetActiveUniform(_program, i, sizeof(name), NULL, &size, &type, name);
		int location = glGetUniformLocation(_program, name);
		CHECK_NE_F(location, -1, "Uniform '%s' not found in shader '%s'", name, _debug_name.c_str());
		VLOG_F(1, "Uniform %d: %10s, %d x %s, location: %d", i, name, size, type_to_string(type), location);
		_uniforms.emplace_back(Uniform{name, size, type, location});
	}
}

Program::Program(Program&& other)
{
	this->swap(other);
}

Program& Program::operator=(Program&& other)
{
	this->swap(other);
	return *this;
}

void Program::swap(Program& other)
{
	std::swap(_program,    other._program);
	std::swap(_debug_name, other._debug_name);
	std::swap(_uniforms,   other._uniforms);
	std::swap(_attributes, other._attributes);
}

Program::~Program()
{
	glDeleteProgram(_program);
}

void Program::validate() const
{
// #ifndef NDEBUG
	validate_program(_program, _debug_name.c_str());
// #endif
}

void Program::bind() const
{
	glUseProgram(_program);
}

void Program::unbind() const
{
	#if EMILIB_GL_OPENGL_VERSION < 300
		glUseProgram(0);
	#endif
}

int Program::get_uniform_loc(const std::string& uniform_name) const
{
	for (const auto& uniform : _uniforms) {
		if (uniform.name == uniform_name) {
			return uniform.location;
		}
	}
	ABORT_F("Failed to find location for uniform '%s' in program '%s'",
			uniform_name.c_str(), _debug_name.c_str());
}

int Program::get_attribute_loc(const std::string& attrib_name) const
{
	for (const auto& attrib : _attributes) {
		if (attrib.name == attrib_name) {
			return attrib.location;
		}
	}
	ABORT_F("Failed to find location for attribute '%s' in program '%s'",
			attrib_name.c_str(), _debug_name.c_str());
}

bool Program::has_uniform(const std::string& uniform_name) const
{
	for (const auto& uniform : _uniforms) {
		if (uniform.name == uniform_name) {
			return true;
		}
	}
	return false;
}

bool Program::has_attribute(const std::string& attrib_name) const
{
	for (const auto& attrib : _attributes) {
		if (attrib.name == attrib_name) {
			return true;
		}
	}
	return false;
}

template<> void Program::set_uniform(int loc, const int& v) const
{
	CHECK_FOR_GL_ERROR;
	glUniform1i(loc, v);
	CHECK_FOR_GL_ERROR;
}

template<> void Program::set_uniform(int loc, const float& v) const
{
	CHECK_FOR_GL_ERROR;
	glUniform1f(loc, v);
	CHECK_FOR_GL_ERROR;
}

template<> void Program::set_uniform(int loc, const double& v) const
{
	CHECK_FOR_GL_ERROR;
	glUniform1f(loc, (float)v);
	CHECK_FOR_GL_ERROR;
}

// ----------------------------------------------------------------------------

Program compile_program(const std::string& vs, const std::string& fs, const std::string& debug_name)
{

#if EMILIB_GL_GLES

	const auto common_prefix = R"(
	// enable dFdx, dFdy, fwidth:
	#extension GL_OES_standard_derivatives : enable

	precision highp float;

	#define GLES
	)";

	const auto vs_prefix = R"(
	#define vs_in attribute
	#define vs_out varying
	)";

	const auto fs_prefix = R"(
	#define fs_in varying
	#define out_FragColor gl_FragColor
	)";

#elif EMILIB_GL_OPENGL_VERSION < 300

	const auto common_prefix = R"(#version 120

	#define lowp
	#define mediump
	#define highp
	#define precision
	)";

	const auto vs_prefix = R"(
	#define vs_in attribute
	#define vs_out varying
	)";

	const auto fs_prefix = R"(
	#define fs_in varying
	#define out_FragColor gl_FragColor
	)";

#else

	const auto common_prefix = R"(#version 150

	#define lowp
	#define mediump
	#define highp
	#define precision

	#define texture2D   texture
	#define textureCube texture
	)";

	const auto vs_prefix = R"(
	#define vs_in in
	#define vs_out out
	)";

	const auto fs_prefix = R"(
	#define fs_in in
	out vec4 out_FragColor;
	)";

#endif

	char line[1024];
	snprintf(line, sizeof(line) - 1, "\n#line 1 /* %s */\n", debug_name.c_str());

	return Program(
		(std::string)common_prefix + vs_prefix + line + vs,
		(std::string)common_prefix + fs_prefix + line + fs,
		debug_name);
}

Program compile_program(const ProgramSource& program_source)
{
	return compile_program(program_source.vs, program_source.fs, program_source.debug_name);
}

// ----------------------------------------------------------------------------

struct AttribInfo
{
	// prec can be e.g. "mediump"
	AttribInfo(const std::string& type_, const std::string& name_, const std::string& prec_="")
		: type(type_), name(name_), prec(prec_) {}

	std::string type, name, prec;

	std::string precision_type() const
	{
		if (prec == "") {
			return type;
		} else {
			return prec + " " + type;
		}
	}
};

ProgramSource create_ff(int flags)
{
	const auto Dims = (flags & FF::dim3  ?  3 :  2);

	const std::string vec_name = (Dims==2 ? "vec2" : "vec3");

	std::vector<AttribInfo> vars;

	if (flags & FF::Texture) {
		vars.push_back(AttribInfo("vec2", "tc", "mediump"));
	}

	if (flags & FF::a_color) {
		vars.push_back(AttribInfo("vec4", "color", "lowp"));
	}

	// ------------------------------------------------
	// Vertex shader

	std::string vs;

	vs += "vs_in " + vec_name + " a_pos;\n"; // All need vertices

	for (auto&& a : vars) {
		vs += "vs_in " + a.precision_type() + " a_" + a.name + ";\n"; // e.g.  varying lowp vec4 v_color;
	}

	vs += "\n";

	for (auto&& a : vars) {
		vs += "vs_out " + a.precision_type() + " v_" + a.name + ";\n"; // e.g.  out lowp vec4 v_color;
	}

	vs += "\n";
	vs += "uniform mat4 u_mvp;\n";
	vs += "\n";
	vs += "void main() {\n";
	if (Dims==2) {
		vs += "    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);\n";
	} else {
		vs += "    gl_Position = u_mvp * vec4(a_pos, 1.0);\n";
	}

	for (auto&& a : vars)
		vs += "    v_" + a.name + " = a_" + a.name + ";\n"; // e.g.  v_color = a_color;

	vs += "}\n"; // End of vs main

	// ------------------------------------------------

	std::string fs = "";

	if (flags & FF::u_color) {
		fs += "uniform vec4 u_color;\n";
	}

	if (flags & FF::Texture) {
		fs += "uniform sampler2D u_sampler;\n";
	}

	fs += "\n";

	for (auto&& a : vars) {
		fs += "fs_in " + a.precision_type() + " v_" + a.name + ";\n"; // e.g.  out lowp vec4 v_color;
	}

	fs += "\n";
	fs += "void main() {\n";

	if (flags & FF::u_color) {
		fs += "    lowp vec4 color = u_color;\n";
	} else {
		fs += "    lowp vec4 color = vec4(1,1,1,1);\n";
	}

	if (flags & FF::Texture) {
		fs += "    lowp vec4 tex = texture2D(u_sampler, v_tc);\n";
		fs += "    color *= tex;\n";
	}

	if (flags & FF::a_color) {
		fs += "    color *= v_color;\n";
	}

	fs += "    out_FragColor = color;\n";

	fs += "}"; // End of fs main

	// ------------------------------------------------

	std::string debug_name = "FF(";
	if (flags & FF::u_color) { debug_name += "u_color, "; }
	if (flags & FF::Texture) { debug_name += "Texture, "; }
	if (flags & FF::a_color) { debug_name += "a_color, "; }
	if (flags != 0) {
		debug_name.resize(debug_name.size() - 2);
	}
	debug_name += ")";

	return {debug_name, vs, fs};
}

Program compile_ff_program(int flags)
{
	const auto program_source = create_ff(flags);
	return compile_program(program_source.vs, program_source.fs, program_source.debug_name);
}

// ----------------------------------------------------------------------------

size_t VertComp::sizeBytes() const
{
	if (type == GL_BYTE)           { return num_comps * sizeof(int8_t);   }
	if (type == GL_UNSIGNED_BYTE)  { return num_comps * sizeof(uint8_t);  }
	if (type == GL_SHORT)          { return num_comps * sizeof(int16_t);  }
	if (type == GL_UNSIGNED_SHORT) { return num_comps * sizeof(uint16_t); }
	if (type == GL_FLOAT)          { return num_comps * sizeof(float);    }
	ABORT_F("Unknow type: %u", type);
}

VertComp VertComp::Float(const char* name)
{
	VertComp vc;
	vc.name      = name;
	vc.num_comps = 1;
	vc.type      = GL_FLOAT;
	vc.normalize = DONT_NORMALIZE;
	vc.offset    = 0;
	return vc;
}

VertComp VertComp::Vec2f(const char* name, Normalize normalize)
{
	VertComp vc;
	vc.name      = name;
	vc.num_comps = 2;
	vc.type      = GL_FLOAT;
	vc.normalize = normalize;
	vc.offset    = 0;
	return vc;
}

VertComp VertComp::Vec3f(const char* name, Normalize normalize)
{
	VertComp vc;
	vc.name      = name;
	vc.num_comps = 3;
	vc.type      = GL_FLOAT;
	vc.normalize = normalize;
	vc.offset    = 0;
	return vc;
}

VertComp VertComp::Vec4f(const char* name, Normalize normalize)
{
	VertComp vc;
	vc.name      = name;
	vc.num_comps = 4;
	vc.type      = GL_FLOAT;
	vc.normalize = normalize;
	vc.offset    = 0;
	return vc;
}

VertComp VertComp::RGBA32(const char* name)
{
	VertComp vc;
	vc.name      = name;
	vc.num_comps = 4;
	vc.type      = GL_UNSIGNED_BYTE;
	vc.normalize = NORMALIZE;
	vc.offset    = 0;
	return vc;
}

VertexFormat::VertexFormat(std::initializer_list<VertComp> components)
{
	_stride = 0;
	for (VertComp vc : components)
	{
		vc.offset = _stride;
		_stride += vc.sizeBytes();
		_comps.push_back(vc);
	}
}

// ----------------------------------------------------------------------------

void bind_prog_and_attributes(const VertexFormat& vf, const Program& program)
{
	program.bind();

	for (auto&& vc : vf) {
		const auto attrib_loc = program.get_attribute_loc(vc.name);

		CHECK_FOR_GL_ERROR;
		glEnableVertexAttribArray(attrib_loc);
		CHECK_FOR_GL_ERROR;
		glVertexAttribPointer(attrib_loc, vc.num_comps, vc.type, vc.normalize, (GLsizei)vf.stride(), (const void*)vc.offset);
		CHECK_FOR_GL_ERROR;
	}

	program.unbind();
}

// ----------------------------------------------------------------------------

VBO::VBO(Type type, Usage usage)
	: _type(type), _usage(usage)
{
	CHECK_FOR_GL_ERROR;
	glGenBuffers(1, &_id);
	CHECK_FOR_GL_ERROR;
}

VBO::~VBO()
{
	glDeleteBuffers(1, &_id);
}

void VBO::bind() const
{
	CHECK_FOR_GL_ERROR;
	glBindBuffer(_type == Vertex ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, _id);
	CHECK_FOR_GL_ERROR;
}


void VBO::unbind() const
{
	#if EMILIB_GL_OPENGL_VERSION < 300
		glBindBuffer(_type == Vertex ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, 0);
	#endif
}

void VBO::upload()
{
	if (!_dirty) {
		return;
	}

	if (_count > 0) {
		bind();

		glBufferData(_type == Vertex ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER,
					 (long)_buffer.size(), _buffer.data(),
					 _usage == Usage::WRITE_ONCE_READ_MANY ? GL_STATIC_DRAW  :
					 _usage == Usage::WRITE_MANY_READ_MANY ? GL_DYNAMIC_DRAW : GL_STREAM_DRAW);

		CHECK_FOR_GL_ERROR;
	}

	_dirty = false;
}

// ----------------------------------------------------------------------------

#if TARGET_OS_IPHONE

	VAO::VAO()
	{
		CHECK_FOR_GL_ERROR;
		glGenVertexArraysOES(1, &_id);
		CHECK_FOR_GL_ERROR;
	}

	VAO::~VAO()
	{
		glDeleteVertexArraysOES(1, &_id);
	}

	void VAO::bind()
	{
		glBindVertexArrayOES(_id);
	}

	void VAO::unbind()
	{
		glBindVertexArrayOES(0);
	}

#elif TARGET_OS_MAC && (300 <= EMILIB_GL_OPENGL_VERSION)

	VAO::VAO()
	{
		CHECK_FOR_GL_ERROR;
		CHECK_F(glGenVertexArrays != nullptr);
		CHECK_F(glBindVertexArray != nullptr);
		CHECK_F(glDeleteVertexArrays != nullptr);

		glGenVertexArrays(1, &_id);
		//glGenVertexArraysAPPLE(1, &_id);
		CHECK_FOR_GL_ERROR;
	}

	VAO::~VAO()
	{
		CHECK_FOR_GL_ERROR;
		glDeleteVertexArrays(1, &_id);
		//glDeleteVertexArraysAPPLE(1, &_id);
		CHECK_FOR_GL_ERROR;
	}

	void VAO::bind()
	{
		CHECK_FOR_GL_ERROR;
		glBindVertexArray(_id);
		//glBindVertexArrayAPPLE(_id);
		CHECK_FOR_GL_ERROR;
	}

	void VAO::unbind()
	{
		CHECK_FOR_GL_ERROR;
		//glBindVertexArray(0);
		CHECK_FOR_GL_ERROR;
	}

#else

	VAO::VAO()
	{
		CHECK_FOR_GL_ERROR;
		CHECK_F(glGenVertexArraysAPPLE != nullptr);
		CHECK_F(glDeleteVertexArraysAPPLE != nullptr);
		CHECK_F(glBindVertexArrayAPPLE != nullptr);
		glGenVertexArraysAPPLE(1, &_id);
		CHECK_FOR_GL_ERROR;
	}

	VAO::~VAO()
	{
		glDeleteVertexArraysAPPLE(1, &_id);
	}

	void VAO::bind()
	{
		glBindVertexArrayAPPLE(_id);
	}

	void VAO::unbind()
	{
		glBindVertexArrayAPPLE(0);
	}

#endif

// ----------------------------------------------------------------------------

MeshPainter::MeshPainter(Usage usage, VertexFormat vf)
	: _usage(usage), _vertices(VBO::Vertex, _usage), _vf(vf)
{
}

void MeshPainter::paint_strip(const Program& prog)
{
	paint(prog, GL_TRIANGLE_STRIP);
}

void MeshPainter::paint(const Program& prog, GLenum mode)
{
	VAO_UP& vao = _vao_from_prog[prog.id()];

	if (!vao) {
		vao.reset(new VAO());
		vao->bind();

		_vertices.bind();
		if (_indices) { _indices->bind(); }
		bind_prog_and_attributes(_vf, prog);

		vao->unbind();
	}

	vao->bind();

	_vertices.bind();
	_vertices.upload(); // If needed.

	if (_indices) {
		_indices->upload(); // If needed.
		_indices->bind();
		CHECK_FOR_GL_ERROR;
		glDrawElements(mode, (GLsizei)_indices->count(), GL_UNSIGNED_INT, (void*)0);
		_indices->unbind();
		CHECK_FOR_GL_ERROR;
	} else {
		CHECK_FOR_GL_ERROR;
		glDrawArrays(mode, 0, (GLsizei)_vertices.count());
		CHECK_FOR_GL_ERROR;
	}

	_vertices.unbind();
	vao->unbind();

	CHECK_FOR_GL_ERROR;
}

// ------------------------------------------------

static Rectangle s_current_vp{0, 0, 0, 0};

TempViewPort::TempViewPort(Rectangle bb)
	: _old_vp(s_current_vp)
{
	glViewport(bb.x, bb.y, bb.width, bb.height);
	s_current_vp = bb;
}

TempViewPort::TempViewPort(Size size)
	: TempViewPort({0, 0, (int)size.x, (int)size.y})
{
}

TempViewPort::~TempViewPort()
{
	glViewport(_old_vp.x, _old_vp.y, _old_vp.width, _old_vp.height);
	s_current_vp = _old_vp;
}

void TempViewPort::set_back_buffer(Rectangle bb)
{
	s_current_vp = bb;
	CHECK_FOR_GL_ERROR;
	glViewport(bb.x, bb.y, bb.width, bb.height);
	CHECK_FOR_GL_ERROR;
}

const Rectangle& TempViewPort::back_buffer()
{
	return s_current_vp;
}

Size TempViewPort::back_buffer_size()
{
	return {s_current_vp.width, s_current_vp.height};
}

// ----------------------------------------------------------------------------

FBO::Lock::Lock(const FBO* fbo) : _fbo(fbo)
{
	if (_fbo) {
		CHECK_FOR_GL_ERROR;

		int old;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old);
		_old = (GLuint)old;
		CHECK_FOR_GL_ERROR;

		glBindFramebuffer(GL_FRAMEBUFFER, fbo->id());
		CHECK_FOR_GL_ERROR;
	}
}

FBO::Lock::~Lock()
{
	if (_fbo) {
		CHECK_FOR_GL_ERROR;
		glBindFramebuffer(GL_FRAMEBUFFER, _old);
		CHECK_FOR_GL_ERROR;
	}
}

static const char* framebuffer_completion_to_string(GLenum err)
{
	switch (err)
	{
		case     GL_FRAMEBUFFER_COMPLETE:                      return "GL_FRAMEBUFFER_COMPLETE";
		case     GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:         return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
		// case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
		case     GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
		case     GL_FRAMEBUFFER_UNSUPPORTED:                   return "GL_FRAMEBUFFER_UNSUPPORTED";

		// case GL_FRAMEBUFFER_COMPLETE_EXT:                      return "GL_FRAMEBUFFER_COMPLETE_EXT";
		// case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:         return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT";
		// case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT";
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:         return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT";
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:            return "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT";
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:        return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT";
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:        return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT";
		// case GL_FRAMEBUFFER_UNSUPPORTED_EXT:                   return "GL_FRAMEBUFFER_UNSUPPORTED_EXT";

		default: return                                        "UNKNOWN";
	}
}

FBO::FBO(const std::string& debug_name, Size size, const Params& params)
	: _debug_name(debug_name), _size(size), _params(params)
{
	CHECK_FOR_GL_ERROR;

	glGenFramebuffers(1, &_fbo_id);
	CHECK_FOR_GL_ERROR;

	{
		FBO::Lock lock(this);
		CHECK_FOR_GL_ERROR;

		if (params.with_color) {
			CHECK_FOR_GL_ERROR;
			// glEnable(GL_TEXTURE_2D);
			_color_tex = Texture(debug_name + "_color", TexParams::clamped_linear(), params.color_format, size, nullptr);
			_color_tex.bind();
			if (!_color_tex.has_data()) {
				// We must init texture or we'll get a GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				_color_tex.set_data(nullptr);
			}
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _color_tex.id(), 0);
			CHECK_FOR_GL_ERROR;
		} else {
			glDrawBuffer(GL_NONE); // No color buffer is drawn to.
			glReadBuffer(GL_NONE); // No color buffer is read from.
		}

		CHECK_FOR_GL_ERROR;

#if !EMILIB_GL_GLES
		if (params.depth == Depth::kDepthRenderBuffer) {
			CHECK_FOR_GL_ERROR;
			glGenRenderbuffers(1, &_depth_rbo_id);
			glBindRenderbuffer(GL_RENDERBUFFER, _depth_rbo_id);

			GLuint depth_format;
			if (params.depth_format == ImageFormat::Depth16) {
				depth_format = GL_DEPTH_COMPONENT16;
			} else if (params.depth_format == ImageFormat::Depth24) {
				depth_format = GL_DEPTH_COMPONENT24;
			} else if (params.depth_format == ImageFormat::Depth32) {
				depth_format = GL_DEPTH_COMPONENT32;
			} else {
				ABORT_F("Expected a depth format.");
			}

			glRenderbufferStorage(GL_RENDERBUFFER, depth_format, width(), height());
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(
				GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_rbo_id);
			CHECK_FOR_GL_ERROR;
		} else if (params.depth == Depth::kDepthTexture) {
			_depth_tex = Texture(debug_name + "_depth", TexParams::clamped_linear(), params.depth_format, size, nullptr);
			_depth_tex.bind();
			if (!_depth_tex.has_data()) {
				// We must init texture or we'll get a GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				_depth_tex.set_data(nullptr);
			}
			CHECK_FOR_GL_ERROR;
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _depth_tex.id(), 0);
			CHECK_FOR_GL_ERROR;
		}
#endif // !EMILIB_GL_GLES
	}

	CHECK_FOR_GL_ERROR;

	{
		FBO::Lock lock(this);
		auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		CHECK_EQ_F(status, GL_FRAMEBUFFER_COMPLETE,
			"Framebuffer '%s' not complete after initialization: 0x%04X (%s)",
			debug_name.c_str(), status, framebuffer_completion_to_string(status));
	}

	CHECK_FOR_GL_ERROR;
}

FBO::~FBO()
{
#if !EMILIB_GL_GLES
	if (_depth_rbo_id != 0) {
		glDeleteRenderbuffers(1, &_depth_rbo_id);
	}
#endif // !EMILIB_GL_GLES
	glDeleteFramebuffers(1, &_fbo_id);
}

unsigned FBO::id() const
{
	return _fbo_id;
}

void FBO::generate_color_mipmap()
{
	CHECK_FOR_GL_ERROR;
	// glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _color_tex.id());
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	CHECK_FOR_GL_ERROR;
}

// ----------------------------------------------------------------------------

} // namespace gl
} // namespace emilib
