// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2012-10-07 for Ghostel
//   Cleaned up as separate library 2016-02

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <emilib/gl_lib_fwd.hpp>

namespace emilib {

class DelayedDirWatcher;
using DelayedDirWatcher_UP = std::unique_ptr<DelayedDirWatcher>;

using ImageData = std::unique_ptr<void, std::function<void(void*)>>;

/// Simple implementation would be:
///
/// 	emilib::ImageData load_image(const char* path, int* width, int* height, int* comp, int req_comp)
/// 	{
/// 		void* data = stbi_load(path, width, height, comp, req_comp);
/// 		LOG_IF_F(ERROR, !data, "Failed loading image '%s': %s", path, stbi_failure_reason());
/// 		return {data, stbi_image_free};
/// 	}
///
/// If you return nullptr, a placeholder error image will be generated and used.
using ImageLoader = std::function<ImageData(const char* path, int* width, int* height, int* comp, int req_comp)>;

/// Returns r, g, b, a uint8_t quadruplets, row by row, from top.
/// This fixes some issues with stbi_image vs alpha.
ImageData load_image_rgba(const ImageLoader& image_loader, const char* path, size_t* out_width, size_t* out_height);

/// Handles loading, unloading, memoization of textures.
/// If a file changes on disk, that file is hot-reloaded by a call to update().
class TextureMngr
{
public:
	using Recorder = std::function<void(const std::string& name)>;

	/// Look for textures relative to gfx_dir
	TextureMngr(const std::string& gfx_dir, ImageLoader image_loader);
	~TextureMngr();

	/// Call frequently (once a frame) for hot-reloading of textures.
	void update();

	/// While holding on to this shared_ptr handle, the texture won't get evicted.
	gl::Texture_SP prefetch_retain(const std::string& name, const gl::TexParams& params);
	gl::Texture_SP get_retain(const std::string& name, const gl::TexParams& params);
	gl::Texture_SP get_retain(const std::string& name) { return get_retain(name, default_params()); }

	/// Get a texture ready for use.
	gl::Texture* get(const std::string& name, const gl::TexParams& params);
	gl::Texture* get(const std::string& name) { return get(name, default_params()); }

	/// Get a handle to a texture which will be loaded by finalize_eviction.
	gl::Texture* prefetch(const std::string& name, const gl::TexParams& params);
	gl::Texture* prefetch(const std::string& name) { return prefetch(name, default_params()); }

	/// Recursively prefetch all textures in gfx_dir/sub_folder
	void prefetch_all(const std::string& sub_folder = "");

	/// Recursively list all images in gfx_dir/sub_folder
	std::vector<std::string> all_image_paths(const std::string& sub_folder = "") const;

	/// When we need to load a bunch of new things it is prudent to throw out the old.
	/// We do this in three steps:
	/// 1) call prepare_eviction
	/// 2) call prefetch (or get) on all textures you are planning to use
	/// 3) call finalize_eviction, which will, in order:
	///   * throw out all textures not marked in step 2.
	///   * load all textures prefetched in step 2
	void prepare_eviction();
	void finalize_eviction();

#if 0
	const Texture* store(Texture_UP&& tex);
#endif
	const gl::Texture* black() const;
	const gl::Texture* white() const;

	gl::TexParams default_params() const
	{
#if TARGET_OS_IPHONE
		return gl::TexParams::clamped_linear(); // No need for mipmaps
#else
		return gl::TexParams::clamped();
#endif
	}

	/// Bytes
	size_t memory_usage(unsigned* out_tex_count) const;

	void print_memory_usage(const char* prefix="") const;

	/// Let's say you want to load a bunch of resources and later reload the same ones.
	/// You can use a Recorder to record all things being loaded.
	void start_recording(Recorder recorder);
	void stop_recording();

private:
	struct TexInfo
	{
		std::string    name;
		std::string    abs_path;
		gl::Texture_SP texture;
		bool           used = false;
	};

	void reload(const std::string& name);
	TexInfo* prefetch_tex_info(const std::string& name, const gl::TexParams& params);

private:
	using FileMap = std::unordered_map<std::string, TexInfo>;

	std::string          _gfx_dir;
	ImageLoader          _image_loader;
	DelayedDirWatcher_UP _dir_watcher;
	FileMap              _file_map;
	bool                 _is_evicting = false;
	Recorder             _recorder;
};

} // namespace emilib
