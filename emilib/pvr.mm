// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// HISTORY:
//   Created in 2014 for Ghostel

#include <loguru.hpp>

#include <emilib/gl_lib.hpp>

#import <GLKit/GLKit.h>

namespace gl {

static NSString* string_to_ns_string(const std::string& utf8)
{
	return [[NSString alloc] initWithBytes:utf8.c_str() length:utf8.length() encoding:NSUTF8StringEncoding];
}

gl::Texture load_pvr(const char* path, gl::TexParams params)
{
	CHECK_FOR_GL_ERROR;

	//EAGLContext context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	//[EAGLContext setCurrentContext:context];
//NSOpenGLContext* context = (__bridge NSOpenGLContext*)SDL_GL_GetCurrentContext();
	//[context makeCurrentContext];

	CHECK_FOR_GL_ERROR;

	NSError* error;
	NSString* path_ns = string_to_ns_string(path);

#if 1
	NSDictionary* options = nullptr;
#elif 0
	NSDictionary* options = [NSDictionary dictionaryWithObjectsAndKeys:
									  [NSNumber numberWithBool:YES], GLKTextureLoaderApplyPremultiplication,
									  nil];
#else
	NSDictionary* options = [NSDictionary dictionaryWithObjectsAndKeys:
									  [NSNumber numberWithBool:YES], GLKTextureLoaderOriginBottomLeft,
									  nil];
#endif

	GLKTextureInfo* tex_info = [GLKTextureLoader textureWithContentsOfFile:path_ns options:options error:&error];

	CHECK_FOR_GL_ERROR;

	if (error) {
		/*
		GLKTextureLoaderErrorFileOrURLNotFound             =  0,
		GLKTextureLoaderErrorInvalidNSData                 =  1,
		GLKTextureLoaderErrorInvalidCGImage                =  2,
		GLKTextureLoaderErrorUnknownPathType               =  3,
		GLKTextureLoaderErrorUnknownFileType               =  4,
		GLKTextureLoaderErrorPVRAtlasUnsupported           =  5,
		GLKTextureLoaderErrorCubeMapInvalidNumFiles        =  6,
		GLKTextureLoaderErrorCompressedTextureUpload       =  7,
		GLKTextureLoaderErrorUncompressedTextureUpload     =  8,
		GLKTextureLoaderErrorUnsupportedCubeMapDimensions  =  9,
		GLKTextureLoaderErrorUnsupportedBitDepth           = 10,
		GLKTextureLoaderErrorUnsupportedPVRFormat          = 11,
		GLKTextureLoaderErrorDataPreprocessingFailure      = 12
		GLKTextureLoaderErrorMipmapUnsupported             = 13,
		GLKTextureLoaderErrorUnsupportedOrientation        = 14,
		GLKTextureLoaderErrorReorientationFailure          = 15,
		GLKTextureLoaderErrorAlphaPremultiplicationFailure = 16,
		GLKTextureLoaderErrorInvalidEAGLContext            = 17
		GLKTextureLoaderErrorIncompatibleFormatSRGB        = 18
		 */
		NSString* ns_string = [error localizedDescription];
		ABORT_F("Failed to load pvr at '%s': %s", path, [ns_string cStringUsingEncoding:NSUTF8StringEncoding]);
	}

	params.filter = tex_info.containsMipmaps ? gl::TexFilter::Mipmapped : gl::TexFilter::Linear;

	gl::Size size{tex_info.width, tex_info.height};
	gl::Texture texture(tex_info.name, size, params, gl::ImageFormat::RGBA32, loguru::filename(path));
	texture.set_bits_per_pixel(4);
	return texture;
}

} // namespace gl
