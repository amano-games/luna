#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "sys-types.h"
#include "str.h"
#include "str.c"

#include "audio/adpcm.c"
#include "./wav/wav.h"
#include "./wav/wav.c"

#include "./tex/tex.c"
#include "./tex/tex.h"

#define IMG_EXT ".png"
#define AUD_EXT ".wav"
// #define RAW_EXT ".raw"
#define ANI_EXT ".lunass"

void
fcopy(const str8 in_path, const str8 out_path)
{
	FILE *f1 = fopen((char *)in_path.str, "rb");
	FILE *f2 = fopen((char *)out_path.str, "wb");
	char buffer[BUFSIZ];
	size_t n;

	while((n = fread(buffer, sizeof(char), sizeof(buffer), f1)) > 0) {
		if(fwrite(buffer, n, sizeof(char), f2) != 1) {
			fprintf(stderr, "Failed to copy file \n");
		}
	}

	printf("[cpy] %s -> %s\n", in_path.str, out_path.str);
}

void
gen_tex_recursive(const str8 in_dir, const str8 out_dir)
{
	DIR *dir;
	struct dirent *entry;
	struct stat statbuf;
	char in_path_buff[FILENAME_MAX];
	char out_path_buff[FILENAME_MAX];
	str8 in_path  = str8_array_fixed(in_path_buff);
	str8 out_path = str8_array_fixed(out_path_buff);

	dir = opendir((char *)in_dir.str);
	if(dir == NULL) {
		fprintf(stderr, "Cannot open directory: %s\n", in_dir.str);
		return;
	}

	while((entry = readdir(dir)) != NULL) {
		stbsp_snprintf((char *)in_path.str, in_path.size, "%s/%s", in_dir.str, entry->d_name);
		stbsp_snprintf((char *)out_path.str, out_path.size, "%s/%s", out_dir.str, entry->d_name);

		if(stat((char *)in_path.str, &statbuf) == -1) {
			perror("Error getting file information");
			continue;
		}

		if(S_ISDIR(statbuf.st_mode)) {
			if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
				mkdir((char *)out_path.str, statbuf.st_mode);
				gen_tex_recursive(in_path, out_path);
			}
		} else {
			if(strstr(entry->d_name, IMG_EXT) != NULL) {
				handle_texture(in_path, out_path);
			} else if(strstr(entry->d_name, ANI_EXT) != NULL) {
				fcopy(in_path, out_path);
			} else if(strstr(entry->d_name, AUD_EXT)) {
				i32 res = handle_wav(in_path, out_path);
			} else {
				fcopy(in_path, out_path);
			}
		}
	}
	closedir(dir);
}

int
main(int argc, char *argv[])
{
	if(argc != 3) {
		printf("Usage: %s <in_path> <destination_path>\n", argv[0]);
		return 1;
	}

	fprintf(stderr, "Processing assets...\n");
	mkdir(argv[2], 0755);
	gen_tex_recursive(str8_cstr(argv[1]), str8_cstr(argv[2]));

	return EXIT_SUCCESS;
}
