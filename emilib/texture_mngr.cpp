// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "texture_mngr.hpp"

#include <map>

#include <emilib/dir_watcher.hpp>
#include <emilib/file_system.hpp>
#include <emilib/gl_lib.hpp>
#include <emilib/irange.hpp>
#include <emilib/mem_map.hpp>
#include <emilib/pvr.hpp>
#include <loguru.hpp>

namespace emilib {

bool ends_with(const std::string& str, const std::string& ending)
{
	return str.size()>=ending.size() && str.substr(str.size()-ending.size()) == ending;
}

ImageData load_image_rgba(const ImageLoader& image_loader, const char* path, size_t* out_width, size_t* out_height)
{
	struct Color { uint8_t r,g,b,a; };

	int w, h, comp;
	auto image_data = image_loader(path, &w, &h, &comp, 4);

	auto pixels = (Color*)image_data.get();

	CHECK_GE_F(w, 0);
	CHECK_GE_F(h, 0);

	// Fix issues with stbi_load:
	if (comp == 1) {
		// input was greyscale - set alpha:
		for (Color& pixel : emilib::it_range(pixels, pixels + w * h)) {
			pixel.a = pixel.r;
		}
	} else {
		for (Color& pixel : emilib::it_range(pixels, pixels + w * h)) {
			if (pixel.a == 0) {
				pixel = Color{0,0,0,0};
			}
		}
	}

	*out_width = w;
	*out_height = h;
	return image_data;
}

// ------------------------------------------------

gl::Texture load_uncompressed_pvr(const char* path, gl::TexParams params, std::string debug_name)
{
	ERROR_CONTEXT("path", path);

	emilib::MemMap mem_map(path);
	return gl::load_uncompressed_pvr_from_memory(mem_map.data(), mem_map.size(), params, std::move(debug_name));
}

// ------------------------------------------------

// file_path, Texture
std::pair<std::string, gl::Texture> load_texture(
	const ImageLoader& image_loader, const std::string& gfx_dir, std::string name, gl::TexParams params)
{
	if (fs::file_ending(name) == "") {
		name += ".png";
	}

	ERROR_CONTEXT("texture_name", name.c_str());

#if TARGET_OS_IPHONE
	auto pvr_path = gfx_dir + name.substr(0, name.size()-4) + ".pvr";
	if (fs::file_exists(pvr_path.c_str())) {
		return {pvr_path, gl::load_pvr(pvr_path.c_str(), params)};
	}
#endif

	auto uncompressed_pvr_path = gfx_dir + name.substr(0, name.size()-4) + "_uncompressed.pvr";
	if (fs::file_exists(uncompressed_pvr_path.c_str())) {
		return {uncompressed_pvr_path, load_uncompressed_pvr(uncompressed_pvr_path.c_str(), params, name)};
	}

#if TARGET_OS_IPHONE
	LOG_F(WARNING, "Loading non-pvr image file '%s'", name.c_str());
#endif
	const auto img_path = gfx_dir + name;
	size_t w,h;
	ImageData data = load_image_rgba(image_loader, img_path.c_str(), &w, &h);
	return {img_path, gl::Texture{name, params, gl::ImageFormat::RGBA32, {(unsigned)w, (unsigned)h}, data.get()}};
}

// --------------------------------------------------------------------

TextureMngr::TextureMngr(const std::string& gfx_dir, ImageLoader image_loader)
	: _gfx_dir(gfx_dir)
	, _image_loader(std::move(image_loader))
{
}

TextureMngr::~TextureMngr() = default;

void TextureMngr::update()
{
	if (!_dir_watcher) {
		_dir_watcher = std::make_unique<emilib::DelayedDirWatcher>(_gfx_dir);
	}

	for (const std::string& abs_path : _dir_watcher->poll_files()) {
		reload(abs_path);
	}
}

void TextureMngr::reload(const std::string& abs_path)
{
	for (auto&& p : _file_map) {
		auto&& tex_info = p.second;
		if (tex_info.abs_path == abs_path) {
			if (!tex_info.texture->has_data()) {
				LOG_F(1, "Skipped hot-reload of '%s': not loaded", tex_info.name.c_str());
				return;
			}

			LOG_F(INFO, "Hot-reloading texture '%s'", tex_info.name.c_str());
			std::tie(tex_info.abs_path, *tex_info.texture) =
				load_texture(_image_loader, _gfx_dir, tex_info.name, tex_info.texture->params());
		}
	}

	LOG_F(1, "Skipped hot-reload of '%s': not found", abs_path.c_str());
}

TextureMngr::TexInfo* TextureMngr::prefetch_tex_info(const std::string& name, const gl::TexParams& params)
{
	if (_recorder) {
		_recorder(name);
	}

	auto& tex_info = _file_map[name];
	if (!tex_info.texture) {
		tex_info.texture = std::make_shared<gl::Texture>();
		tex_info.name = name;
	}
	tex_info.used = true;
	return &tex_info;
}

gl::Texture_SP TextureMngr::prefetch_retain(const std::string& name, const gl::TexParams& params)
{
	return prefetch_tex_info(name, params)->texture;
}

gl::Texture_SP TextureMngr::get_retain(const std::string& name, const gl::TexParams& params)
{
	auto tex_info = prefetch_tex_info(name, params);

	if (tex_info->texture->has_data()) {
		tex_info->texture->set_params(params);
	} else {
		// LOG_IF_F(WARNING, !_is_evicting,
		// 	"Hot-loading texture '%s' - you should mark this during eviction", name.c_str());
		std::tie(tex_info->abs_path, *tex_info->texture) =
			load_texture(_image_loader, _gfx_dir, name, params);
	}

	return tex_info->texture;
}

gl::Texture* TextureMngr::prefetch(const std::string& name, const gl::TexParams& params)
{
	return prefetch_retain(name, params).get();
}

gl::Texture* TextureMngr::get(const std::string& name, const gl::TexParams& params)
{
	return get_retain(name, params).get();
}

#if 0
gl::Texture* TextureMngr::store(Texture_UP&& tex_up)
{
	auto tex = tex_up.release();
	auto name = tex->name();
	_file_map[name] = tex;
	return tex;
}
#endif

gl::Texture create_black()
{
	unsigned width = 8;
	unsigned height = 8;
	std::vector<uint8_t> img(width * height * 4);
	for (size_t i = 0; i < img.size(); i += 4)
	{
		img[0] = 0;
		img[1] = 0;
		img[2] = 0;
		img[3] = 255;
	}
	gl::TexParams params(gl::TexFilter::Nearest, gl::WrapMode::DontCare);
	return gl::Texture{"black", params, gl::ImageFormat::RGBA32, {width, height}, img.data()};
}

const gl::Texture* TextureMngr::black() const
{
	static gl::Texture s_black = create_black();
	return &s_black;
}

gl::Texture create_white()
{
	unsigned width = 8;
	unsigned height = 8;
	std::vector<uint8_t> img(width * height * 4, 255);
	gl::TexParams params(gl::TexFilter::Nearest, gl::WrapMode::DontCare);
	return gl::Texture{"white", params, gl::ImageFormat::RGBA32, {width, height}, img.data()};
}

const gl::Texture* TextureMngr::white() const
{
	static gl::Texture s_white = create_white();
	return &s_white;
}

size_t TextureMngr::memory_usage(unsigned* out_tex_count) const
{
	size_t bytes = 0;
	unsigned count = 0;
	for (auto&& p: _file_map) {
		if (p.second.texture->has_id()) {
			bytes += p.second.texture->memory_usage();
			count += 1;
		}
	}
	if (out_tex_count) {
		*out_tex_count = count;
	}
	return bytes;
}

void TextureMngr::print_memory_usage(const char* prefix) const
{
	LOG_SCOPE_F(INFO, "Texture Memory Usage");
	size_t bytes = 0;
	size_t bytes_compressed = 0;
	unsigned count = 0;

	std::map<std::string, unsigned> prefix_size;

	for (auto&& p: _file_map) {
		const auto& texture = p.second.texture;
		if (texture->has_id()) {
			const auto texture_size_bytes = texture->memory_usage();

			bytes += texture_size_bytes;
			if (texture->bits_per_pixel() < 8) {
				bytes_compressed += texture_size_bytes;
			}
			count += 1;

			auto path = p.first;
			auto slash = path.find('/');
			while (slash != std::string::npos) {
				prefix_size[path.substr(0, slash)] += texture_size_bytes;
				slash = path.find('/', slash+1);
			}
		}
	}

	const float MiB = 1024 * 1024;
	LOG_F(INFO, "%s%5.1f MiB in %3u textures (%5.1f MiB compressed)",
		prefix, bytes / MiB, count, bytes_compressed / MiB);

	for (auto&& p : prefix_size) {
		LOG_F(INFO, "%-20s %5.1f MiB", p.first.c_str(), p.second / MiB);
	}
}

void TextureMngr::prepare_eviction()
{
	CHECK_F(!_is_evicting);
	_is_evicting = true;
	for (auto&& p: _file_map) {
		p.second.used = false;
	}
	// print_memory_usage("Before loading:  ");
}

void TextureMngr::finalize_eviction()
{
	// print_memory_usage("Before eviction: ");

	CHECK_F(_is_evicting);
	_is_evicting = false;

	unsigned num_evicted = 0;
	for (auto&& p: _file_map) {
		auto& tex_info = p.second;
		if (!tex_info.used && tex_info.texture.unique() && tex_info.texture->has_id()) {
			num_evicted += 1;
			tex_info.texture->free();
		}
	}

	// print_memory_usage("Before loading:  ");

	unsigned num_loaded  = 0;
	for (auto&& p: _file_map) {
		auto& tex_info = p.second;
		if (tex_info.used) {
			if (!tex_info.texture->has_data()) {
				num_loaded += 1;
				std::tie(tex_info.abs_path, *tex_info.texture) =
					load_texture(_image_loader, _gfx_dir, tex_info.name, tex_info.texture->params());
			}
		}
	}

	// print_memory_usage("After loading:   ");
}

void TextureMngr::start_recording(Recorder recorder)
{
	CHECK_F(!_recorder, "Already recording");
	_recorder = std::move(recorder);
}

void TextureMngr::stop_recording()
{
	CHECK_F(!!_recorder, "Not recording");
	_recorder = {};
}

} // namespace emilib
