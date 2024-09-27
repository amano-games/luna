#include "pd-utils.h"
#include "sys-io.h"
// TODO: Remove this
#include <string.h>

void
extract_value(char *line, char *key, char *dest, size_t max_len)
{
	char *value = strchr(line, '='); // Find the '=' character
	if(value) {
		value++;                           // Move past the '='
		strncpy(dest, value, max_len - 1); // Copy the value into the destination
		dest[max_len - 1] = '\0';          // Ensure null-termination
		// Remove trailing newline if present
		size_t len = strlen(dest);
		if(len > 0 && dest[len - 1] == '\n') {
			dest[len - 1] = '\0';
		}
	}
}

void
extract_int_value(char *line, char *key, int *dest)
{
	// char *value = strchr(line, '='); // Find the '=' character
	// if(value) {
	// 	*dest = 0;
	// 	// atoi(value + 1); // Convert the value after '=' to an integer
	// }
}

void
pdxinfo_parse(struct pdxinfo *info, struct alloc *scratch)
{
	void *f = sys_file_open(str8_lit("pdxinfo"), SYS_FILE_R);
	if(f != NULL) {
		struct sys_file_stats stats = sys_fstats(str8_lit("pdxinfo"));
		char *buffer                = scratch->allocf(scratch->ctx, stats.size);
		sys_file_read(f, (void *)buffer, stats.size);

		char *line = strtok(buffer, "\n");
		while(line) {
			if(strncmp(line, "name=", 5) == 0) {
				extract_value(line, "name", info->name, sizeof(info->name));
			} else if(strncmp(line, "author=", 7) == 0) {
				extract_value(line, "author", info->author, sizeof(info->author));
			} else if(strncmp(line, "description=", 12) == 0) {
				extract_value(line, "description", info->description, sizeof(info->description));
			} else if(strncmp(line, "bundleID=", 9) == 0) {
				extract_value(line, "bundleID", info->bundle_id, sizeof(info->bundle_id));
			} else if(strncmp(line, "version=", 8) == 0) {
				extract_value(line, "version", info->version, sizeof(info->version));
			} else if(strncmp(line, "buildNumber=", 12) == 0) {
				extract_int_value(line, "buildNumber", &info->build_number);
			} else if(strncmp(line, "imagePath=", 10) == 0) {
				extract_value(line, "imagePath", info->image_path, sizeof(info->image_path));
			}

			line = strtok(NULL, "\n"); // Move to the next line
		}
	}
}
