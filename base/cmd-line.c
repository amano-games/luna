#include "cmd-line.h"
#include "base/hash.h"
#include "base/link-list.h"
#include "base/utils.h"

static struct cmd_line_opt **
cmd_line_slot_from_str8(
	struct cmd_line *cmd_line,
	str8 string)
{
	struct cmd_line_opt **slot = 0;
	if(cmd_line->option_table_size != 0) {
		u64 hash   = hash_fnv1a_str8(string);
		u64 bucket = hash % cmd_line->option_table_size;
		slot       = &cmd_line->option_table[bucket];
	}
	return slot;
}

static struct cmd_line_opt *
cmd_line_opt_from_slot(struct cmd_line_opt **slot, str8 string)
{
	if(slot == NULL) { return NULL; }
	struct cmd_line_opt *res = NULL;
	for(struct cmd_line_opt *var = *slot; var; var = var->hash_next) {
		if(str8_match(string, var->name, 0)) {
			res = var;
			break;
		}
	}
	return res;
}

static void
cmd_line_push_opt(struct cmd_line_opt_list *list, struct cmd_line_opt *var)
{
	SLLQueuePush(list->first, list->last, var);
	list->count += 1;
}

static struct cmd_line_opt *
cmd_line_insert_opt(struct alloc alloc, struct cmd_line *cmd_line, str8 string, struct str8_list values)
{
	struct cmd_line_opt *var          = NULL;
	struct cmd_line_opt **slot        = cmd_line_slot_from_str8(cmd_line, string);
	struct cmd_line_opt *existing_var = cmd_line_opt_from_slot(slot, string);
	if(existing_var != 0) {
		var = existing_var;
	} else {
		var                  = alloc_struct_clr(alloc, var);
		var->hash_next       = *slot;
		var->hash            = hash_fnv1a_str8(string);
		var->name            = str8_cpy_push(alloc, string);
		var->values          = values;
		struct str_join join = {0};
		join.pre             = str8_lit("");
		join.sep             = str8_lit(",");
		join.post            = str8_lit("");
		var->value           = str8_list_join(alloc, &var->values, &join);
		*slot                = var;
		cmd_line_push_opt(&cmd_line->options, var);
	}
	return var;
}

// TODO: digest
static struct cmd_line
cmd_line_from_string_list(struct alloc alloc, struct str8_list arguments)
{
	struct cmd_line res   = {0};
	res.option_table_size = 64;
	res.option_table      = alloc_arr_clr(alloc, res.option_table, res.option_table_size);

	// Playdate has no command line at all, so an empty list must still yield a
	// usable cmd_line rather than a null-dereference on arguments.first.
	if(arguments.first == NULL) { return res; }

	res.exe_name = arguments.first->str;

	b32 after_passthrough_option = 0;

	for(struct str8_node *node = arguments.first->next, *next = 0; node != 0; node = next) {
		next = node->next;

		// look at --, -, or / (only on Windows) at the start of an
		// argument to determine if it's a flag option. all arguments after a
		// single "--" (with no trailing string on the command line will be
		// considered as passthrough input strings.
		b32 is_option    = 0;
		str8 option_name = node->str;
		if(!after_passthrough_option) {
			is_option = 1;
			if(str8_match(node->str, str8_lit("--"), 0)) {
				// The bare "--" is the marker itself, not an input. Any later
				// "--" arrives with after_passthrough_option already set and
				// falls through to the input path below.
				after_passthrough_option = 1;
				continue;
			} else if(str8_match(str8_prefix(node->str, 2), str8_lit("--"), 0)) {
				option_name = str8_skip(option_name, 2);
			} else if(str8_match(str8_prefix(node->str, 1), str8_lit("-"), 0)) {
				option_name = str8_skip(option_name, 1);
			} else if(OS_KIND_CURRENT == OS_KIND_WINDOWS &&
				str8_match(str8_prefix(node->str, 1), str8_lit("/"), 0)) {
				option_name = str8_skip(option_name, 1);
			} else {
				is_option = 0;
			}
		}

		// string is an option
		if(is_option) {
			//  unpack option prefix
			b32 has_values                 = 0;
			u64 value_signifier_position1  = str8_find_needle(option_name, 0, str8_lit(":"), 0);
			u64 value_signifier_position2  = str8_find_needle(option_name, 0, str8_lit("="), 0);
			u64 value_signifier_position   = MIN(value_signifier_position1, value_signifier_position2);
			str8 value_portion_this_string = str8_skip(option_name, value_signifier_position + 1);
			if(value_signifier_position < option_name.size) {
				has_values = 1;
			}
			option_name = str8_prefix(option_name, value_signifier_position);

			// parse option's values
			struct str8_list values = {0};
			if(has_values) {
				for(struct str8_node *n = node; n; n = n->next) {
					next        = n->next;
					str8 string = n->str;
					if(n == node) {
						string = value_portion_this_string;
					}
					u8 splits[]                            = {','};
					struct str8_list values_in_this_string = str8_split(alloc, string, splits, ARRLEN(splits), 0);
					for(struct str8_node *sub_val = values_in_this_string.first; sub_val; sub_val = sub_val->next) {
						str8_list_push(alloc, &values, sub_val->str);
					}
					if(!str8_match(str8_postfix(n->str, 1), str8_lit(","), 0) &&
						(n != node || value_portion_this_string.size != 0)) {
						break;
					}
				}
			}

			// store
			cmd_line_insert_opt(alloc, &res, option_name, values);
		}

		// default path - treat as a passthrough input
		else {
			str8_list_push(alloc, &res.inputs, node->str);
		}
	}

	// fill argc/argv
	res.argc = arguments.node_count;
	res.argv = alloc_arr_clr(alloc, res.argv, res.argc);
	{
		u64 idx = 0;
		for(struct str8_node *n = arguments.first; n != 0; n = n->next) {
			res.argv[idx] = (char *)str8_cpy_push(alloc, n->str).str;
			idx += 1;
		}
	}

	return res;
}

static struct cmd_line
cmd_line_from_argcv(struct alloc alloc, i32 argc, char **argv)
{
	struct str8_list arguments = {0};
	for(i32 i = 0; i < argc && argv != NULL; ++i) {
		str8_list_push(alloc, &arguments, str8_cstr(argv[i]));
	}
	return cmd_line_from_string_list(alloc, arguments);
}

static struct cmd_line_opt *
cmd_line_opt_from_str8(struct cmd_line *cmd_line, str8 name)
{
	return cmd_line_opt_from_slot(cmd_line_slot_from_str8(cmd_line, name), name);
}

static struct str8_list
cmd_line_str8_list(struct cmd_line *cmd_line, str8 name)
{
	struct str8_list result  = {0};
	struct cmd_line_opt *var = cmd_line_opt_from_str8(cmd_line, name);
	if(var != 0) {
		result = var->values;
	}
	return result;
}

static str8
cmd_line_str8(struct cmd_line *cmd_line, str8 name)
{
	str8 res                 = {0};
	struct cmd_line_opt *var = cmd_line_opt_from_str8(cmd_line, name);
	if(var != 0) {
		res = var->value;
	}
	return res;
}

static b32
cmd_line_has_flag(struct cmd_line *cmd_line, str8 name)
{
	struct cmd_line_opt *var = cmd_line_opt_from_str8(cmd_line, name);
	return (var != 0);
}

static b32
cmd_line_has_arg(struct cmd_line *cmd_line, str8 name)
{
	struct cmd_line_opt *var = cmd_line_opt_from_str8(cmd_line, name);
	return (var != 0 && var->values.node_count > 0);
}

static b32
cmd_line_i64(struct cmd_line *cmd_line, str8 name, i64 *out)
{
	b32 res  = false;
	str8 str = cmd_line_str8(cmd_line, name);
	if(str.size > 0) {
		*out = str8_to_i64(str, 10);
		res  = true;
	}
	return res;
}
