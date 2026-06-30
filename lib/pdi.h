#pragma once

#include "base/types.h"
#include "sys/sys-io.h"
#include "base/dbg.h"

struct pdi_header {
	char magic[12]; // Ident Playdate IMG
	u32 flags;      // flags: If > 0, the data in this file is compressed
};

struct pdi_img_header {
	u32 size;  // Size of image data section when decompressed
	u32 w;     // Image width(in pixels)
	u32 h;     // Image height (in pixels)
	u32 magic; // Unknown/reserved? Seen as 0
};

struct pdi_img_cell {
	u16 clip_w; // 0 	uint16 	Cell clip width (in pixels)
	u16 clip_h; // 2 	uint16 	Cell clip height (in pixels)
	u16 stride; // 4 	uint16 	Cell stride (bytes per image row)
	u16 clip_l; // 6 	uint16 	Cell clip left (in pixels)
	u16 clip_r; // 8 	uint16 	Cell clip right (in pixels)
	u16 clip_t; // 10 	uint16 	Cell clip top (in pixels)
	u16 clip_b; // 12 	uint16 	Cell clip bottom (in pixels)
	u16 flags;  // 14 	uint16 	Cell bitflags If > 0, cell uses transparency
};

struct pdi {
	struct pdi_header header;
	struct pdi_img_header img_header;
	struct pdi_img_cell cell;
	ssize data_size;
	u8 *data;
};

b32
pdi_write(struct pdi pdi, str8 path)
{
	b32 res           = false;
	void *f           = NULL;
	b32 is_compressed = (pdi.header.flags & 0x80000000) > 0;

	if(is_compressed) {
		dbg_not_implemeneted("PDI with compression not supported");
	}

	f = sys_file_open_w(path);
	dbg_check_warn(f, "pdi", "failed to open file to write %.*s", (int)path.size, path.str);

	sys_file_w(f, &pdi.header, sizeof(pdi.header));
	if(is_compressed) {
		sys_file_w(f, &pdi.img_header, sizeof(pdi.img_header));
	} else {
		sys_file_w(f, &pdi.cell, sizeof(pdi.cell));
	}

	ssize data_size = pdi.data_size;
	sys_file_w(f, pdi.data, data_size);

	res = true;

error:;
	if(f != NULL) {
		sys_file_close(f);
	}
	return res;
}
