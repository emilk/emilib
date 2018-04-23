// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014 for Ghostel
//   Moved into emilib in 2016

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <emilib/gl_lib_fwd.hpp>

namespace emilib {

class DelayedDirWatcher;
using DelayedDirWatcher_UP = std::unique_ptr<DelayedDirWatcher>;

/// Load the file at shader_dir/name.shader with includes relative to shader_dir.
gl::ProgramSource load_shader_file(const std::string& shader_dir, const std::string& name);

/**
 * ShaderMngr is a high-level library of emilib, meaning it is NOT a stand-alone library.
 * Instead, it depends on many other parts of emilib.
 *
 * ShaderMngr's job is to encapsulate loading, memoization and reloading of shader files.
 *
 * In particular, you can put your gl shaders in .shader files which ShaderMngr can load.
 * If you modify the .shader file, ShaderMngr can detect it and automatically reload it for you.
 * This is very useful for quickly trying new things without having to restart your app.
 *
 * Example of a simple .shader file:
 * 	vertex_shader: """
 * 		vs_in vec2 a_pos;
 * 		vs_in vec2 a_tc;
 *
 * 		vs_out vec2 v_tc;
 *
 * 		uniform mat4 u_mvp;
 *
 * 		void main()
 * 		{
 * 			gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
 * 			v_tc = a_tc;
 * 		}
 * 	"""
 *
 * 	fragment_shader: """
 * 		fs_in vec2 v_tc;
 *
 * 		uniform sampler2D u_sampler;
 * 		uniform mediump vec4 u_color;
 *
 * 		void main()
 * 		{
 * 			lowp vec4 color = texture2D(u_sampler, v_tc);
 * 			color *= u_color;
 * 			out_FragColor = color;
 * 		}
 * 	"""
 *
 * Additionally, a .shader file can include raw GLSL code with this (on top of the .shader file):
 *
 * 	includes: [ "first.glsl", "second.glsl" ]
 *
 * This will inject the contents of the files "first.glsl" and "second.glsl" into the top of both
 * the fragment and vertex shader code. This is useful for including a library of GLSL functions.
 * All includes are relative to the shader_dir given to ShaderMngr.
 */
class ShaderMngr
{
public:
	/// Look for .shader files in shader_dir
	explicit ShaderMngr(const std::string& shader_dir);
	~ShaderMngr();

	/// call this periodically (e.g. every frame) to detect and reload modified shader files.
	void update();

	/// Construct shader from source.
	// gl::Program* get(const gl::ProgramSource& program_source);

	/// Construct shader from source.
	// gl::Program* get_source(const std::string& name, const std::string& vs, const std::string& fs) const;

	/// Fixed function emulation using gl::FF combinations. Memoizes.
	gl::Program* get_ff(int flags);

	/// Construct shader from shader_dir/name.shader. Memoizes.
	gl::Program* get_file(const std::string& name);

	/// Reload all .shader files. You shouldn't need to call this if you are using `update()`.
	/// update() will hot-reload any changed files on-the-fly.
	void reload_all();

	/// Recursively load all shaders in shader_dir/sub_folder
	void prefetch_all(const std::string& sub_folder = "");

	/// Recursively list all images in shader_dir/sub_folder
	std::vector<std::string> all_shader_paths(const std::string& sub_folder = "") const;

private:
	// using SourceMap = std::unordered_map<gl::ProgramSource, gl::Program_UP>;
	using FFMap     = std::unordered_map<int,               gl::Program_UP>;
	using FileMap   = std::unordered_map<std::string,       gl::Program_UP>;

	std::string          _shader_dir;
	DelayedDirWatcher_UP _dir_watcher;
	// SourceMap            _source_map;
	FFMap                _ff_map;
	FileMap              _file_map;
};

} // namespace emilib
