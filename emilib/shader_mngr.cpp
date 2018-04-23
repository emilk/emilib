// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "shader_mngr.hpp"

#include <map>
#include <unordered_map>

#include <configuru.hpp>
#include <loguru.hpp>

#include <emilib/dir_watcher.hpp>
#include <emilib/file_system.hpp>
#include <emilib/gl_lib.hpp>
#include <emilib/strprintf.hpp>

namespace emilib {

configuru::Config parse_cfg_file(const char* path)
{
	try {
		return configuru::parse_file(path, configuru::CFG);
	} catch (std::exception& e) {
		ABORT_F("Failed to load %s: %s", path, e.what());
	}
}

gl::ProgramSource load_shader_file(const std::string& shader_dir, const std::string& name)
{
	const auto path = shader_dir + name;
	ERROR_CONTEXT("load_shader_file", path.c_str());
	const auto root = parse_cfg_file(path.c_str());

	std::string prefix = "";

	if (root.has_key("includes")) {
		for (auto&& include_name : root["includes"].as_array()) {
			ERROR_CONTEXT("including", include_name.c_str());
			auto include_path = shader_dir + (std::string)include_name;
			auto contents = fs::read_text_file(include_path);
			auto line = strprintf("\n#line 1 // %s\n", include_name.c_str());
			prefix += line + contents;
		}
	}

	auto get = [&](const char* shader_name) {
		auto&& config = root[shader_name];
		auto code = (std::string)config;
		auto line = config.line();
		if (line == -1) { line = 1; };

		auto line_str = strprintf("\n#line %d // %s %s\n", line-1, name.c_str(), shader_name);
		return prefix + line_str + code;
	};

	const auto vs = get("vertex_shader");
	const auto fs = get("fragment_shader");
	root.check_dangling();
	return {name, vs, fs};
}

/*
gl::Program* ShaderMngr::get(const std::string& name) const
{
	static std::unordered_map<std::string, gl::ProgramImpl_UP> s_map;
	auto it = s_map.find(name);
	if (it==s_map.end()) {
		gl::ProgramImpl_UP val = load_program_from_file(name);
		it = s_map.insert(make_pair(name, std::move(val))).first;
	}

	return it->second.get();
}
 */

// ------------------------------------------------

ShaderMngr::ShaderMngr(const std::string& shader_dir)
	: _shader_dir(shader_dir)
{
}

ShaderMngr::~ShaderMngr() = default;

void ShaderMngr::update()
{
	if (!_dir_watcher) {
		_dir_watcher = std::make_unique<DelayedDirWatcher>(_shader_dir);
	}

	if (!_dir_watcher->poll_files().empty()) {
		// TODO: reload only affected shaders.
		reload_all();
	}
}

void ShaderMngr::reload_all()
{
	for (auto&& p : _file_map) {
		LOG_F(INFO, "Hot-reloading shader '%s'", p.first.c_str());

		try {
			// TODO: Hash lookup on shader code to prevent recompiling that which hasn't changed.
			*p.second = gl::compile_program(load_shader_file(_shader_dir, p.first));
		} catch (std::exception& e) {
			LOG_F(ERROR, "Hot-reload of shader failed: %s", e.what());
		}
	}
}

// gl::Program* ShaderMngr::get(const gl::ProgramSource& program_source)
// {
// 	auto it = _shader_map.find(tup);

// 	if (it == _shader_map.end()) {
// 		auto program = std::make_unique<gl::Program>(gl::compile_program(program_source));
// 		it = _shader_map.insert(make_pair(tup, std::move(program))).first;
// 	}

// 	return it->second.get();
// }

// gl::Program* ShaderMngr::get_source(const std::string& name, const std::string& vs, const std::string& fs) const
// {
// 	return get({name, vs, fs});
// }

gl::Program* ShaderMngr::get_ff(int flags)
{
	auto it = _ff_map.find(flags);

	if (it == _ff_map.end()) {
		auto program = std::make_unique<gl::Program>(gl::compile_ff_program(flags));
		it = _ff_map.insert(make_pair(flags, std::move(program))).first;
	}

	return it->second.get();
}

gl::Program* ShaderMngr::get_file(const std::string& name)
{
	auto it = _file_map.find(name);

	if (it == _file_map.end()) {
		const auto program_source = load_shader_file(_shader_dir, name);
		auto program = std::make_unique<gl::Program>(gl::compile_program(program_source));
		it = _file_map.insert(make_pair(name, std::move(program))).first;
	}

	return it->second.get();
}

void ShaderMngr::prefetch_all(const std::string& sub_folder)
{
	fs::walk_dir(_shader_dir + sub_folder, [=](const std::string& file_path) {
		if (fs::file_ending(file_path) == "shader") {
			get_file(fs::strip_path(_shader_dir, file_path));
		}
	});
}

/// Recursively list all images in shader_dir/sub_folder
std::vector<std::string> ShaderMngr::all_shader_paths(const std::string& sub_folder) const
{
	std::vector<std::string> paths;
	fs::walk_dir(_shader_dir + sub_folder, [&](const std::string& file_path) {
		if (fs::file_ending(file_path) == "shader") {
			paths.push_back(fs::strip_path(_shader_dir, file_path));
		}
	});
	return paths;
}

} // namespace emilib
