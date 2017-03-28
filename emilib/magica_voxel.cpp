// By Emil Ernerfeldt 2017
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
// Based on https://voxel.codeplex.com/wikipage?title=Sample%20Codes
// See also https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt

#include "magica_voxel.hpp"

#include <cstdio>

#include <loguru.hpp>

namespace emilib {
namespace magica_voxel {

static const unsigned int kDefaultPalette[256] = {0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff,
	0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff,
	0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff, 0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff,
	0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff,
	0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff, 0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff,
	0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc,
	0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc, 0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc,
	0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc,
	0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc, 0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc,
	0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99,
	0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99, 0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99,
	0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699,
	0xffcc6699, 0xff996699, 0xff666699, 0xff336699, 0xff006699, 0xffff3399, 0xffcc3399, 0xff993399,
	0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099,
	0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66, 0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66,
	0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966,
	0xff669966, 0xff339966, 0xff009966, 0xffff6666, 0xffcc6666, 0xff996666, 0xff666666, 0xff336666,
	0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066,
	0xffcc0066, 0xff990066, 0xff660066, 0xff330066, 0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33,
	0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33,
	0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933, 0xff669933, 0xff339933, 0xff009933, 0xffff6633,
	0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333,
	0xff663333, 0xff333333, 0xff003333, 0xffff0033, 0xffcc0033, 0xff990033, 0xff660033, 0xff330033,
	0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00,
	0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00, 0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900,
	0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600,
	0xff006600, 0xffff3300, 0xffcc3300, 0xff993300, 0xff663300, 0xff333300, 0xff003300, 0xffff0000,
	0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa,
	0xff000088, 0xff000077, 0xff000055, 0xff000044, 0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00,
	0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100,
	0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000, 0xff880000, 0xff770000, 0xff550000, 0xff440000,
	0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777,
	0xff555555, 0xff444444, 0xff222222, 0xff111111};

// magic number
int as_id(int a, int b, int c, int d)
{
	return (a) | (b << 8) | (c << 16) | (d << 24);
}

struct Chunk
{
	int  id;
	int  content_size;
	int  children_size;
	long end;
};

int read_int(FILE* fp)
{
	int v = 0;
	fread(&v, 4, 1, fp);
	return v;
}

void read_chunk(FILE* fp, Chunk* chunk)
{
	chunk->id = read_int(fp);
	chunk->content_size = read_int(fp);
	chunk->children_size = read_int(fp);

	// end of chunk : used for skipping the whole chunk
	chunk->end = ftell(fp) + chunk->content_size + chunk->children_size;

	const char* c = (const char*)(&chunk->id);
	LOG_F(INFO, "%c%c%c%c: %d %d",
		c[0], c[1], c[2], c[3], chunk->content_size, chunk->children_size);
}

std::vector<Model> load(const char* path)
{
	FILE* fp = fopen(path, "rb");
	if (!fp) {
		LOG_F(ERROR, "failed to open '%s'", path);
		return {};
	}

	const int MV_VERSION = 150;

	const int ID_VOX  = as_id('V', 'O', 'X', ' ');
	const int ID_MAIN = as_id('M', 'A', 'I', 'N');
	const int ID_SIZE = as_id('S', 'I', 'Z', 'E');
	const int ID_XYZI = as_id('X', 'Y', 'Z', 'I');
	const int ID_RGBA = as_id('R', 'G', 'B', 'A');
	const int ID_PACK = as_id('P', 'A', 'C', 'K');

	int magic = read_int(fp);
	if (magic != ID_VOX) {
		LOG_F(ERROR, "magic number does not match");
		fclose(fp);
		return {};
	}

	const int version = read_int(fp);
	if (version != MV_VERSION) {
		LOG_F(ERROR, "version does not match, expected %d, got %d", MV_VERSION, version);
		fclose(fp);
		return {};
	}

	// main chunk
	Chunk main_chunk;
	read_chunk(fp, &main_chunk);
	if (main_chunk.id != ID_MAIN) {
		LOG_F(ERROR, "main chunk is not found");
		fclose(fp);
		return {};
	}

	// skip content of main chunk
	fseek(fp, main_chunk.content_size, SEEK_CUR);

	bool is_custom_palette = false;

	Model model;
	model.version = version;

	// read children chunks
	while (ftell(fp) < main_chunk.end) {
		// read chunk header
		Chunk sub;
		read_chunk(fp, &sub);

		if (sub.id == ID_PACK) {
			int num_models = read_int(fp);
			LOG_F(INFO, "%d models in %s", num_models, path);
			CHECK_EQ_F(num_models, 1); // TODO
		} else if (sub.id == ID_SIZE) {
			model.size[0] = read_int(fp);
			model.size[1] = read_int(fp);
			model.size[2] = read_int(fp);
		} else if (sub.id == ID_XYZI) {
			int num_voxels = read_int(fp);
			if (num_voxels < 0) {
				LOG_F(ERROR, "negative number of voxels");
				fclose(fp);
				return {};
			}

			// voxels
			if (num_voxels > 0) {
				model.voxels.resize(num_voxels);
				fread(model.voxels.data(), sizeof(Voxel), num_voxels, fp);
			}
		} else if (sub.id == ID_RGBA) {
			// last color is not used, so we only need to read 255 colors
			is_custom_palette = true;
			fread(model.palette + 1, sizeof(RGBA), 255, fp);

			// NOTICE : skip the last reserved color
			RGBA reserved;
			fread(&reserved, sizeof(RGBA), 1, fp);
		} else {
			char chunk_id[5] = {
				static_cast<char>(sub.id & 0xff),
				static_cast<char>((sub.id >> 8) & 0xff),
				static_cast<char>((sub.id >> 16) & 0xff),
				static_cast<char>((sub.id >> 24) & 0xff),
				'\0',
			};
			LOG_F(WARNING, "Unknown chunk: %s", chunk_id);
		}

		// skip unread bytes of current chunk or the whole unused chunk
		fseek(fp, sub.end, SEEK_SET);
	}

	if (!is_custom_palette) {
		static_assert(sizeof(model.palette) == sizeof(kDefaultPalette), "");
		memcpy(model.palette, kDefaultPalette, sizeof(model.palette));
	}

	// print model info
	LOG_F(INFO, "Model: %dx%dx%d = %lu voxels", model.size[0], model.size[1], model.size[2], model.voxels.size());

	fclose(fp);
	return {model};
}

} // namespace magica_voxel
} // namespace emilib
