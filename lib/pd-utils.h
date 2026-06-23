#pragma once

#include "base/mem.h"

struct pdxinfo {
	str8 name;
	str8 author;
	str8 description;
	str8 bundle_id;
	str8 version;
	int build_number;
	str8 image_path;
};

struct pdxinfo pdxinfo_parse(struct alloc alloc, struct alloc scratch);
