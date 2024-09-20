#pragma once

#include "mem.h"

struct pdxinfo {
	char name[50];
	char author[50];
	char description[100];
	char bundle_id[100];
	char version[20];
	int build_number;
	char image_path[100];
};

void pdxinfo_parse(struct pdxinfo *pdxinfo, struct alloc *scratch);
