#include "loc.h"
#include "base/dbg.h"
#include "base/hash.h"
#include "sys/sys-log.h"

static str8 loc_pool_get(struct loc_pool pool, u32 idx);
static b32 loc_section_copy(struct alloc alloc, void *data, usize size, u32 offset, u32 section_size, u32 count, struct loc_pool *out);
static b32 loc_fmt_arg_parse(str8 fmt, usize i, u32 *arg, usize *len);
static usize loc_fmt_w(u8 *dst, str8 fmt, str8 *args, usize count);

b32
loc_load_from_mem(struct loc *loc, struct alloc alloc, void *data, usize size)
{
	*loc      = (struct loc){.lang = -1};
	u8 *bytes = data;

	struct loc_header header = {0};
	if(data == NULL || size < sizeof(header)) {
		log_error("loc", "data too small");
		return false;
	}

	mcpy(&header, bytes, sizeof(header));

	if(header.magic != LOC_MAGIC || header.version != LOC_VERSION) {
		log_error("loc", "bad magic or version %u", header.version);
		return false;
	}

	if(header.lang_count == 0 || header.lang_count > LOC_LANGS_MAX || sizeof(header) + sizeof(struct loc_lang_entry) * header.lang_count > size) {
		log_error("loc", "bad language count %u", header.lang_count);
		return false;
	}

	loc->key_count  = header.key_count;
	loc->lang_count = header.lang_count;
	mcpy(loc->langs, bytes + sizeof(header), sizeof(struct loc_lang_entry) * header.lang_count);

	if(!loc_section_copy(alloc, data, size, header.keys_offset, header.keys_size, header.key_count, &loc->keys)) {
		log_error("loc", "bad keys section");
		return false;
	}

	loc->ht = ht_u32_new(ht_exp_from_count(header.key_count), alloc);

	for(u32 i = 0; i < header.key_count; ++i) {
		str8 key  = loc_pool_get(loc->keys, i);
		u32 value = ht_u32_insert(&loc->ht, hash_fnv1a_str8(key), i + 1);
		if(value != i + 1) {
			log_error("loc", "duplicate key or hash collision: %.*s", str8_spread(key));
			dbg_assert(0);
		}
	}

	return true;
}

b32
loc_lang_set_from_mem(struct loc *loc, struct alloc alloc, void *data, usize size, i32 lang)
{
	if(lang < 0 || (u32)lang >= loc->lang_count) {
		log_error("loc", "bad language index %d", lang);
		return false;
	}

	struct loc_lang_entry entry = loc->langs[lang];
	struct loc_pool strs        = {0};
	if(!loc_section_copy(alloc, data, size, entry.offset, entry.size, loc->key_count, &strs)) {
		log_error("loc", "bad section for %.*s", str8_spread(loc_lang_id(loc, lang)));
		return false;
	}

	loc->strs = strs;
	loc->lang = lang;
	return true;
}

i32
loc_lang_idx(struct loc *loc, str8 id)
{
	for(u32 i = 0; i < loc->lang_count; ++i) {
		if(str8_match(loc_lang_id(loc, (i32)i), id, 0)) { return (i32)i; }
	}
	return -1;
}

str8
loc_lang_id(struct loc *loc, i32 lang)
{
	if(lang < 0 || (u32)lang >= loc->lang_count) { return str8_zero(); }
	char *id   = loc->langs[lang].id;
	usize size = 0;
	while(size < LOC_LANG_ID_SIZE && id[size] != 0) { size++; }
	return (str8){(u8 *)id, size};
}

str8
loc_get(struct loc *loc, str8 key)
{
	if(loc->strs.offsets == NULL) { return key; }
	u32 value = ht_u32_get(&loc->ht, hash_fnv1a_str8(key));
	if(value == 0) { return key; }
	return loc_pool_get(loc->strs, value - 1);
}

str8
loc_fmt(struct alloc alloc, str8 fmt, str8 *args, usize count)
{
	usize size = loc_fmt_w(NULL, fmt, args, count);
	u8 *str    = alloc_arr(alloc, str, size + 1);
	if(str == NULL) { return str8_zero(); }
	loc_fmt_w(str, fmt, args, count);
	str[size] = 0;
	return (str8){str, size};
}

b32
loc_fmt_args_mask(str8 fmt, u32 *mask)
{
	*mask = 0;
	for(usize i = 0; i < fmt.size; ++i) {
		if(fmt.str[i] != '{') { continue; }
		if(i + 1 < fmt.size && fmt.str[i + 1] == '{') {
			i++;
			continue;
		}
		u32 arg   = 0;
		usize len = 0;
		if(!loc_fmt_arg_parse(fmt, i, &arg, &len)) { continue; }
		if(arg >= 32) { return false; }
		*mask |= 1u << arg;
		i += len - 1;
	}
	return true;
}

static str8
loc_pool_get(struct loc_pool pool, u32 idx)
{
	u32 start = pool.offsets[idx];
	u32 end   = pool.offsets[idx + 1];
	return (str8){pool.data + start, end - start - 1};
}

static b32
loc_section_copy(struct alloc alloc, void *data, usize size, u32 offset, u32 section_size, u32 count, struct loc_pool *out)
{
	u32 table_size = sizeof(u32) * (count + 1);
	if(offset > size || section_size > size - offset || section_size < table_size) { return false; }

	u8 *mem = alloc_size_aligned(alloc, section_size, alignof(u32), false);
	if(mem == NULL) { return false; }
	mcpy(mem, (u8 *)data + offset, section_size);

	struct loc_pool pool = {.offsets = (u32 *)mem, .data = mem + table_size};
	if(pool.offsets[count] > section_size - table_size) { return false; }
	for(u32 i = 0; i < count; ++i) {
		if(pool.offsets[i] >= pool.offsets[i + 1]) { return false; }
	}

	*out = pool;
	return true;
}

static b32
loc_fmt_arg_parse(str8 fmt, usize i, u32 *arg, usize *len)
{
	usize j = i + 1;
	u32 n   = 0;
	while(j < fmt.size && j - i <= 3 && char_is_digit(fmt.str[j], 10)) {
		n = n * 10 + (u32)(fmt.str[j] - '0');
		j++;
	}
	if(j == i + 1 || j >= fmt.size || fmt.str[j] != '}') { return false; }
	*arg = n;
	*len = j - i + 1;
	return true;
}

static usize
loc_fmt_w(u8 *dst, str8 fmt, str8 *args, usize count)
{
	usize len = 0;
	for(usize i = 0; i < fmt.size;) {
		u8 c      = fmt.str[i];
		u32 arg   = 0;
		usize arg_len = 0;
		if((c == '{' || c == '}') && i + 1 < fmt.size && fmt.str[i + 1] == c) {
			if(dst) { dst[len] = c; }
			len++;
			i += 2;
			continue;
		}
		if(c == '{' && loc_fmt_arg_parse(fmt, i, &arg, &arg_len) && arg < count) {
			str8 value = args[arg];
			if(dst && value.size > 0) { mcpy(dst + len, value.str, value.size); }
			len += value.size;
			i += arg_len;
			continue;
		}
		if(dst) { dst[len] = c; }
		len++;
		i++;
	}
	return len;
}
