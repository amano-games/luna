#pragma once

#include "base/str.h"
#include "base/types.h"

struct cmd_line_opt {
	struct cmd_line_opt *next;
	struct cmd_line_opt *hash_next;
	u64 hash;
	str8 name;
	struct str8_list values;
	str8 value;
};

struct cmd_line_opt_list {
	u64 count;
	struct cmd_line_opt *first;
	struct cmd_line_opt *last;
};

struct cmd_line {
	str8 exe_name;
	struct cmd_line_opt_list options;
	struct str8_list inputs;
	u64 option_table_size;
	struct cmd_line_opt **option_table;
	u64 argc;
	char **argv;
};

static struct cmd_line_opt **cmd_line_slot_from_str8(struct cmd_line *cmd_line, str8 string);
static struct cmd_line_opt *cmd_line_opt_from_slot(struct cmd_line_opt **slot, str8 string);
static void cmd_line_push_opt(struct cmd_line_opt_list *list, struct cmd_line_opt *var);
static struct cmd_line_opt *cmd_line_insert_opt(struct alloc alloc, struct cmd_line *cmd_line, str8 string, struct str8_list values);
static struct cmd_line cmd_line_from_string_list(struct alloc alloc, struct str8_list arguments);
static struct cmd_line cmd_line_from_argcv(struct alloc alloc, i32 argc, char **argv);

static struct cmd_line_opt *cmd_line_opt_from_str8(struct cmd_line *cmd_line, str8 name);
static struct str8_list cmd_line_str8_list(struct cmd_line *cmd_line, str8 name);
static str8 cmd_line_str8(struct cmd_line *cmd_line, str8 name);
static b32 cmd_line_has_flag(struct cmd_line *cmd_line, str8 name);
static b32 cmd_line_has_arg(struct cmd_line *cmd_line, str8 name);
static b32 cmd_line_i64(struct cmd_line *cmd_line, str8 name, i64 *out);
