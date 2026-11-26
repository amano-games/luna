#include "loc-gen.h"
#include "base/dbg.h"
#include "base/hash.h"
#include "base/marena.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "sys/sys-log.h"
#include "sys/sys.h"

#define LOC_GEN_LANGS_MAX 8
#define LOC_GEN_COLS_MAX  (LOC_GEN_LANGS_MAX + 1)

static b32
loc_gen_unescape(struct alloc alloc, str8 cell, str8 *out)
{
	u8 *buf   = alloc_arr(alloc, buf, cell.size + 1);
	usize len = 0;
	for(usize i = 0; i < cell.size; ++i) {
		u8 c = cell.str[i];
		if(c != '\\') {
			buf[len++] = c;
			continue;
		}
		if(i + 1 >= cell.size) { return false; }
		u8 e = cell.str[++i];
		if(e == 'n') {
			buf[len++] = '\n';
		} else if(e == '\\') {
			buf[len++] = '\\';
		} else {
			return false;
		}
	}
	buf[len] = 0;
	*out     = (str8){buf, len};
	return true;
}

i32
loc_gen_handle(str8 in_path, str8 out_path, struct alloc scratch)
{
	i32 res                = -1;
	struct alloc sys_alloc = sys_allocator();
	usize mem_size         = MMEGABYTE(8);
	void *mem              = mem_alloc_size(sys_alloc, mem_size);

	dbg_assert(mem != NULL);

	struct marena marena = {0};
	marena_ini_local(&marena, mem, mem_size);
	struct alloc alloc            = marena_allocator(&marena);
	struct sys_full_file_res file = sys_load_full_file(alloc, in_path);
	str8 src                      = {file.data, file.size};

	if(file.data == NULL) {
		log_error("loc-gen", "can't read %.*s", str8_spread(in_path));
		goto cleanup;
	}

	struct str8_list keys                    = {0};
	struct str8_list vals[LOC_GEN_LANGS_MAX] = {0};
	str8 langs[LOC_GEN_LANGS_MAX]            = {0};
	usize lang_count                         = 0;
	usize errors                             = 0;
	b32 has_header                           = false;
	str8_list_push(alloc, &keys, str8_lit("NONE"));

	struct str8_list lines = str8_split(alloc, src, (u8 *)"\n", 1, str_split_flag_keep_empties);
	usize line_no          = 0;

	for(struct str8_node *line_node = lines.first; line_node; line_node = line_node->next) {
		str8 line = line_node->str;
		line_no++;

		if(line.size > 0 && line.str[line.size - 1] == '\r') { line.size--; }
		if(line.size == 0) { continue; }

		struct str8_list cells = str8_split(alloc, line, (u8 *)"\t", 1, str_split_flag_keep_empties);
		// NOTE: sometimes empty language keys can cause the \t
		// to be at the end of the line, push an empty cell in that case
		if(line.str[line.size - 1] == '\t') { str8_list_push(alloc, &cells, str8_lit("")); }

		usize cell_count = cells.node_count;

		if(!has_header) {
			has_header = true;

			if(
				cell_count < 2 ||
				cell_count > LOC_GEN_COLS_MAX ||
				!str8_match(cells.first->str, str8_lit("key"), 0)) {
				log_error("loc-gen", "%.*s:%zu invalid header", str8_spread(in_path), line_no);
				goto cleanup;
			}

			lang_count             = cell_count - 1;
			struct str8_node *cell = cells.first->next;
			for(usize l = 0; l < lang_count; ++l, cell = cell->next) {
				langs[l] = cell->str;
				if(langs[l].size == 0 || langs[l].size >= LOC_LANG_ID_SIZE) {
					log_error("loc-gen", "%.*s:%zu invalid language id '%.*s'", str8_spread(in_path), line_no, str8_spread(langs[l]));
					goto cleanup;
				}
				str8_list_push(alloc, &vals[l], str8_lit(""));
			}
			continue;
		}

		if(cell_count != lang_count + 1) {
			log_error("loc-gen", "%.*s:%zu has %zu cells, expected %zu", str8_spread(in_path), line_no, cell_count, lang_count + 1);
			errors++;
			continue;
		}

		str8 key = cells.first->str;
		str8_list_push(alloc, &keys, key);
		struct str8_node *cell = cells.first->next;

		for(usize l = 0; l < lang_count; ++l, cell = cell->next) {
			str8 val = {0};
			if(!loc_gen_unescape(alloc, cell->str, &val)) {
				log_error("loc-gen", "%.*s:%zu %.*s bad escape in '%.*s' (only \\n and \\\\)", str8_spread(in_path), line_no, str8_spread(langs[l]), str8_spread(cell->str));

				errors++;
				val = cell->str;
			}
			str8_list_push(alloc, &vals[l], val);
		}

		if(vals[0].last->str.size == 0) {
			vals[0].last->str = key;
			vals[0].total_size += key.size;
			log_warn("loc-gen", "%.*s:%zu %.*s has no %.*s text, using the key", str8_spread(in_path), line_no, str8_spread(key), str8_spread(langs[0]));
		}
	}

	if(!has_header) {
		log_error("loc-gen", "%.*s is empty", str8_spread(in_path));
		goto cleanup;
	}

	if(errors > 0) {
		log_error("loc-gen", "%.*s: %zu errors, nothing written", str8_spread(in_path), errors);
		goto cleanup;
	}

	usize key_count = keys.node_count;

	for(usize l = 1; l < lang_count; ++l) {
		usize missing         = 0;
		struct str8_node *key = keys.first->next;
		struct str8_node *def = vals[0].first->next;

		for(struct str8_node *val = vals[l].first->next; val; val = val->next, def = def->next, key = key->next) {

			if(val->str.size > 0) { continue; }

			val->str = def->str;
			vals[l].total_size += def->str.size;
			log_warn("loc-gen", "%.*s missing %.*s, using %.*s", str8_spread(langs[l]), str8_spread(key->str), str8_spread(langs[0]));
			missing++;
		}

		if(missing > 0) { log_warn("loc-gen", "%.*s: %zu/%zu keys missing", str8_spread(langs[l]), missing, key_count - 1); }
	}

	str8 names = str8_list_join(alloc, &keys, &(struct str_join){.sep = str8_lit("\n"), .post = str8_lit("\n")});

	struct loc_header header = {
		.magic      = LOC_MAGIC,
		.keys_hash  = hash_fnv1a_str8(names),
		.version    = LOC_VERSION,
		.lang_count = (u32)lang_count,
		.key_count  = (u32)key_count,
	};

	struct loc_lang_entry entries[LOC_GEN_LANGS_MAX] = {0};
	u32 *offsets[LOC_GEN_LANGS_MAX]                  = {0};
	u32 entry_offset                                 = (u32)(sizeof(header) + sizeof(struct loc_lang_entry) * lang_count);

	// NOTE: for each lang section calculate the offset of each value row
	// and the total size of the value pool
	for(usize i = 0; i < lang_count; ++i) {
		// NOTE: Store key_count + 1 so that last key is an empty used to always be able
		// to calculate offset by offset[i+1] - offset[i]
		offsets[i] = alloc_arr(alloc, offsets[i], key_count + 1);
		u32 pool   = 0;
		usize k    = 0;

		for(struct str8_node *val = vals[i].first; val; val = val->next) {
			offsets[i][k++] = pool;
			pool += (u32)val->str.size + 1;
		}

		// NOTE: Last key is empty
		offsets[i][key_count] = pool;
		mcpy(entries[i].id, langs[i].str, langs[i].size);
		entries[i].offset = entry_offset;
		entries[i].size   = (u32)(sizeof(u32) * (key_count + 1)) + pool;
		entry_offset += entries[i].size;
	}

	str8 out_file_path = str8_fmt_push(alloc, "%.*s", str8_spread(str8_chop_last_dot(out_path)));
	sys_file out_file  = sys_file_open_w(out_file_path);

	if(!sys_file_is_valid(out_file)) {
		log_error("loc-gen", "can't open file %.*s for writing!", str8_spread(out_file_path));
		goto cleanup;
	}

	u8 nul = 0;
	sys_file_w(out_file, &header, sizeof(header));
	sys_file_w(out_file, entries, sizeof(struct loc_lang_entry) * lang_count);

	for(usize i = 0; i < lang_count; ++i) {
		// NOTE: all the keys offsets
		sys_file_w(out_file, offsets[i], sizeof(u32) * (key_count + 1));
		for(struct str8_node *val = vals[i].first; val; val = val->next) {
			if(val->str.size > 0) { sys_file_w(out_file, val->str.str, (u32)val->str.size); }
			sys_file_w(out_file, &nul, 1);
		}
	}
	sys_file_close(out_file);

	log_info("loc-gen", "%.*s -> %.*s (%zu keys, %zu langs)", str8_spread(in_path), str8_spread(out_file_path), key_count - 1, lang_count);
	res = 1;

cleanup:
	sys_free(mem);
	return res;
}
