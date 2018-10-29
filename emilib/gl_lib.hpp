// By Emil Ernerfeldt 2012-2018
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Some code dating back to 2012 (at least).
//   Made into free-standing library for face_morpher in late 2015.
//   Adopted for PipeDreams 2016-02

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <loguru.hpp>

#include "gl_lib_fwd.hpp"

#ifndef EMILIB_GL_GLES
	#error EMILIB_GL_GLES not defined
#endif
#ifndef EMILIB_GL_OPENGL_VERSION
	#error EMILIB_GL_OPENGL_VERSION not defined
#endif

/// OpenGL wrapper classes
namespace emilib {
namespace gl {

using GLenum = unsigned int;
using GLuint = unsigned int;

// ----------------------------------------------------------------------------

void init_glew();

// ----------------------------------------------------------------------------

bool supports_mipmaps_for(Size size);

/** A texture can be in three states:
 *  No id, no data
 *  id, no data,
 *  id and data.
*/
class Texture
{
public:
	/// Will create an invalid texture!
	Texture();

	Texture(GLuint id, Size size, TexParams params_arg, ImageFormat format, std::string debug_name);
	Texture(std::string debug_name, TexParams params = {}, ImageFormat format = {}, Size size = {0, 0}, const void* data = nullptr);

	~Texture();

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(Texture&& other) { this->swap(other); }
	void operator=(Texture&& other) { this->swap(other); }

	void swap(Texture& other);

	/// Free allocated texture, if any. sets id = 0.ß
	void free();

	void set_data(const void* data, Size size, ImageFormat format);

	/// Note: data MUST be in the correct format.
	void set_data(const void* data);

	void set_mip_data(const void* data, Size size, int level);

	const ImageFormat& format() const { return _format; }

	const TexParams& params() const { return _params; }
	void set_params(const TexParams& params);

	bool has_data() const { return _has_data; }

	bool is_power_of_two() const;

	const std::string& debug_name() const { return _debug_name; }
	void set_debug_name(const std::string& debug_name);

	/// We must have an id
	void bind(int tu = 0) const;
	void unbind(int tu = 0) const;

	int width()  const { return _size.x;  }
	int height() const { return _size.y; }
	const Size& size() const { return _size; }

	/// Use to override when you know the format is compressed
	void set_bits_per_pixel(int bpp);
	int bits_per_pixel() const;
	size_t memory_usage() const; ///< in bytes

	/// 0 if not generated
	GLuint id() const { return _id; }
	bool has_id() const { return _id != 0; }

	void generate_mipmaps();

protected:
	void init(const void* data);

	void set_wrap_mode(WrapMode s, WrapMode t) const;
	void set_filtering(TexFilter filter) const;

private:
	Size        _size{0, 0};
	ImageFormat _format;
	std::string _debug_name;

	GLuint      _id       = 0;
	bool        _has_data = false;
	int         _bpp      = 0;

	// The effecto of set_params is deferred until bind() so we can set_params from non-render thread.
	mutable TexParams _params;
	mutable bool      _params_dirty = false;
};

// ----------------------------------------------------------------------------

/// Assumes legacy PVR (version 2).
/// Returns a default texture (has_data() == false) if the given memory does not contain an uncompressed PVR.
Texture load_uncompressed_pvr2_from_memory(
	const void* data, size_t num_bytes,
	TexParams params, std::string debug_name);

// ----------------------------------------------------------------------------

class Program
{
public:
	struct Uniform
	{
		std::string name;
		int         size; ///< Mostly 1, maybe non-1 for arrays?
		unsigned    type; ///< e.g. GL_FLOAT_VEC2
		int         location;
	};

	struct Attribute
	{
		std::string name;
		int         size; ///< Mostly 1, maybe non-1 for arrays?
		unsigned    type; ///< e.g. GL_FLOAT_VEC2
		int         location;
	};

	using Uniforms   = std::vector<Uniform>;
	using Attributes = std::vector<Attribute>;

	// ---------------------------------------------

	Program() {}

	/// Shader format must match the current OpenGl version
	Program(const std::string& vs, const std::string& fs, std::string debug_name);
	Program(Program&&);
	Program& operator=(Program&&);
	~Program();

	void swap(Program& other);

	const std::string& debug_name() const { return _debug_name; }
	unsigned           id()         const { return _program;    }

	/// For debugging: call after binding uniforms. Ignored if not debug build.
	void validate() const;

	void bind() const;
	void unbind() const; /// Does nothing on OpenGL 3 and later.

	int get_attribute_loc(const std::string& attrib_name) const;
	int get_uniform_loc(const std::string& attrib_name) const;

	bool has_attribute(const std::string& name) const;
	bool has_uniform(const std::string& name) const;

	template<typename T>
	void set_uniform(const std::string& name, const T& value) const
	{
		ERROR_CONTEXT("uniform", name.c_str());
		set_uniform(get_uniform_loc(name), value);
	}

	/// gl_lib does NOT implement this function! You have to do that yourself, e.g. for Vec2f, Mat4
	template<typename T>
	void set_uniform(int location, const T& value) const;

private:
	Program(Program&) = delete;
	Program& operator=(Program&) = delete;

	unsigned    _program = 0;
	std::string _debug_name;
	Uniforms    _uniforms;
	Attributes  _attributes;
};

/// Uses the same syntax for all OpenGL versions, including GL ES.
/// VertexShader:  vs_in/vs_out instead of attribute/varying.
/// PixelShader:   fs_in instead of varying, write to out_FragColor.
Program compile_program(const std::string& vs, const std::string& fs, const std::string& debug_name);
Program compile_program(const ProgramSource& program_source);

// ----------------------------------------------------------------------------

/// Functionality for mimicking fixed function with shaders easily
namespace FF
{
	/// Fixed function flags
	enum FF_Flags
	{
		Texture = (1<<0), ///< TODO: rename as u_sampler?
		a_color = (1<<1),
		u_color = (1<<2),
		dim3    = (1<<3),
	};
}

/// `flags` should be a combo if FF::FF_Flags
ProgramSource create_ff(int flags);

/// `flags` should be a combo if FF::FF_Flags
Program compile_ff_program(int flags);

// ----------------------------------------------------------------------------

enum Normalize { DONT_NORMALIZE, NORMALIZE };

struct VertComp
{
	std::string name;
	unsigned    num_comps; ///< 1 for scalars, 2 for Vec2f etc
	unsigned    type;      ///< e.g. GL_FLOAT
	Normalize   normalize; ///< If we normalize, values are rescaled to [0, 1]
	size_t      offset;    ///< Byte offset, filled in by VertexFormat::VertexFormat.

	size_t sizeBytes() const;

	VertComp() {}

	VertComp(const char* name_, unsigned num_comps_, unsigned type_, Normalize normalize_)
		: name(name_), num_comps(num_comps_), type(type_), normalize(normalize_) {}

	// Static constructor helpers:
	static VertComp Float(const char* name);
	static VertComp Vec2f(const char* name, Normalize normalize = DONT_NORMALIZE);
	static VertComp Vec3f(const char* name, Normalize normalize = DONT_NORMALIZE);
	static VertComp Vec4f(const char* name, Normalize normalize = DONT_NORMALIZE);
	static VertComp RGBA32(const char* name);
};

class VertexFormat
{
public:
	using CompIter = std::vector<VertComp>::const_iterator;

	VertexFormat(std::initializer_list<VertComp> components);

	auto begin() const { return _comps.begin(); }
	auto end()   const { return _comps.end();   }

	size_t stride() const { return _stride; }

private:
	size_t                _stride;
	std::vector<VertComp> _comps;
};

void bind_prog_and_attributes(const VertexFormat& vf, const Program& program);

// ----------------------------------------------------------------------------

class VBO
{
public:
	enum Type
	{
		Vertex, Index
	};

	VBO(Type type, Usage usage);
	~VBO();

	template<typename ElementType>
	const ElementType* data() const { return reinterpret_cast<const ElementType*>(_buffer.data()); }

	/// Will re-use memory if same size
	template<typename ElementType>
	ElementType* allocate(size_t count)
	{
		_count = count;
		_buffer.resize(count * sizeof(ElementType));
		_dirty = true;
		return reinterpret_cast<ElementType*>( _buffer.data() );
	}

	template<typename ElementType>
	void append(const ElementType* elements, size_t count)
	{
		_count += count;
		auto elements_ptr = reinterpret_cast<const char*>(elements);
		_buffer.insert(_buffer.end(), elements_ptr, elements_ptr + count * sizeof(ElementType));
		_dirty |= count > 0;
	}

	void clear()
	{
		_dirty = !empty();
		_count = 0;
		_buffer.clear();
	}

	void invalidate() { _dirty = true; }

	bool empty() const { return _count == 0; }
	size_t count() const { return _count; }
	size_t size_bytes() const { return _buffer.size(); }

	void upload();
	void bind() const;
	void unbind() const; /// Does nothing on OpenGL 3 and later.

private:
	unsigned          _id    = -1;
	Type              _type;
	Usage             _usage;
	std::vector<char> _buffer;
	size_t            _count =  0;
	bool              _dirty = true;
};

// ----------------------------------------------------------------------------

class VAO
{
public:
	VAO();
	~VAO();
	void bind();
	void unbind();

private:
	VAO(VAO&);
	VAO(VAO&&);
	VAO& operator=(VAO&);
	VAO& operator=(VAO&&);

	unsigned _id = -1;
};

// ----------------------------------------------------------------------------

class MeshPainter
{
public:
	MeshPainter(Usage usage, VertexFormat vf);

	// --------------------------

	const VertexFormat& vertex_format() const { return _vf; }
	VBO& vert_vbo() { return _vertices; }
	const VBO& vert_vbo() const { return _vertices; }

	/// Will re-use memory if same size
	template<typename Vertex>
	Vertex* allocate_vert(size_t count)
	{
		CHECK_EQ_F(sizeof(Vertex), _vf.stride(), "Unexpected vertex size");
		return _vertices.allocate<Vertex>(count);
	}

	size_t vertex_count() const
	{
		return _vertices.count();
	}

	template<typename Vertex>
	void set_verts(const std::vector<Vertex>& vertices)
	{
		std::copy_n(vertices.data(), vertices.size(), allocate_vert<Vertex>(vertices.size()));
	}

	/// TODO: 16 bit indices
	/// Will re-use memory if same size
	uint32_t* allocate_indices(size_t count)
	{
		if (!_indices) {
			_indices.reset(new VBO(VBO::Index, _usage));
		}
		return _indices->allocate<uint32_t>(count);
	}

	void set_indices(const std::vector<uint32_t>& indices)
	{
		std::copy_n(indices.data(), indices.size(), allocate_indices(indices.size()));
	}

	// --------------------------

	/// mode: GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, ...
	void paint(const Program& prog, GLenum mode);

	void paint_strip(const Program& prog);

	void invalidate_verts() { _vertices.invalidate(); }

private:
	Usage                              _usage;
	VBO                                _vertices;
	VBO_UP                             _indices;
	std::unordered_map<GLuint, VAO_UP> _vao_from_prog;
	VertexFormat                       _vf;
};

// ----------------------------------------------------------------------------

/// Like a typed, dynamic-expanding MeshPainter
/// TODO: inherit MeshPainter?
template<typename Vertex>
class TriangleStrip
{
public:
	TriangleStrip(Usage usage, VertexFormat vf) : _mesh_painter(usage, std::move(vf))
	{
		CHECK_EQ_F(sizeof(Vertex), _mesh_painter.vertex_format().stride());
	}

	bool empty() { return _mesh_painter.vert_vbo().empty(); }

	/// Number of vertices
	int count() const { return _mesh_painter.vert_vbo().count(); }

	int size_bytes() const { return _mesh_painter.vert_vbo().size_bytes(); }

	void clear() { _mesh_painter.vert_vbo().clear(); }

	void add_strip(const Vertex* ptr, size_t n)
	{
		if (n == 0) { return; }
		CHECK_GT_F(n, 2u);
		VBO& vbo = _mesh_painter.vert_vbo();
		if (!vbo.empty()) {
			// Connector by degenerate triangles:
			Vertex last_vertex = vbo.data<Vertex>()[vbo.count() - 1u];
			vbo.append<Vertex>(&last_vertex, 1u);
			vbo.append<Vertex>(ptr, 1u);
		}
		vbo.append<Vertex>(ptr, n);
	}

	void add_strip(const Vertex* start, const Vertex* end) { add_strip(start, end - start); }
	void add_strip(const std::vector<Vertex>& verts)       { add_strip(verts.data(), verts.size()); }
	void add_strip(std::initializer_list<Vertex> verts)    { add_strip(verts.begin(), verts.size()); }

	void paint_strip(const Program& prog) { _mesh_painter.paint_strip(prog); }

private:
	MeshPainter _mesh_painter;
};

// ------------------------------------------------

/// Will set a viewport and restore the old viewport on death.
class TempViewPort
{
public:
	explicit TempViewPort(Rectangle bb);
	explicit TempViewPort(Size size);
	explicit TempViewPort(int width, int height) : TempViewPort(Size{width, height}) {}
	~TempViewPort();

	/// Call when we acquire context or resize window.
	static void set_back_buffer(Rectangle bb);

	/// Call when we acquire context or resize window.
	static void set_back_buffer_size(Size size)
	{
		set_back_buffer(Rectangle{0, 0, (int)size.x, (int)size.y});
	}

	/// Call when we acquire context or resize window.
	static void set_back_buffer_size(int width, int height)
	{
		set_back_buffer(Rectangle{0, 0, width, height});
	}

	static const Rectangle& back_buffer();
	static Size back_buffer_size();

private:
	TempViewPort(TempViewPort&) = delete;
	TempViewPort& operator=(TempViewPort&) = delete;

	Rectangle _old_vp;
};

// ----------------------------------------------------------------------------

/// An off-screen buffer you can draw onto.
class FBO
{
public:
	enum Depth
	{
		kNone,
		kDepthRenderBuffer, ///< Fast
		kDepthTexture,      ///< If you need to sample it later
	};

	/// Bind/unbind FBO:
	class Lock
	{
	public:
		explicit Lock(const FBO* fbo);
		~Lock();

	private:
		Lock(const Lock&)=delete;
		Lock& operator=(const Lock&)=delete;

		GLuint     _old; // Bound before us
		const FBO* _fbo;
	};

	struct Params
	{
#if !EMILIB_GL_GLES // TODO: might be supported on newer GLES versions?
		Depth       depth        = Depth::kNone;
		ImageFormat depth_format = ImageFormat::Depth32;
#endif // !EMILIB_GL_GLES

		bool        with_color   = true;  ///< Turn off the color component if you don't need it.
		bool        color_mipmap = false; ///< You must also call generate_color_mipmap() after painting.
		ImageFormat color_format = ImageFormat::RGBA32;
	};

	// ------------------------------------------------

	FBO(const std::string& debug_name, Size size, const Params& params);
	~FBO();

	FBO(const FBO&) = delete;
	FBO(FBO&&) = delete;
	void operator=(const FBO&) = delete;
	void operator=(FBO&&) = delete;

	GLuint      id()     const;
	const Size& size()   const { return _size;   }
	int         width()  const { return _size.x; }
	int         height() const { return _size.y; }

	/// Call after painting if color_mipmap is set.
	void generate_color_mipmap();

	/// Iff params.with_color
	gl::Texture&       color_texture()         { return _color_tex;            }
	const gl::Texture& color_texture() const   { return _color_tex;            }
	gl::Texture        release_color_texture() { return std::move(_color_tex); }

	/// Iff params.depth == Depth::kDepthTexture
	gl::Texture&       depth_texture()         { return _depth_tex;            }
	const gl::Texture& depth_texture() const   { return _depth_tex;            }
	gl::Texture        release_depth_texture() { return std::move(_depth_tex); }

private:
	std::string _debug_name;
	Size        _size;
	Params      _params;
	GLuint      _fbo_id;
	gl::Texture _color_tex;
	gl::Texture _depth_tex;
#if !EMILIB_GL_GLES
	GLuint      _depth_rbo_id = 0;
#endif // !EMILIB_GL_GLES
};

// ----------------------------------------------------------------------------

} // namespace gl
} // namespace emilib
