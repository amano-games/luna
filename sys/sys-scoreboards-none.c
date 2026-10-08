#include "sys/sys-scoreboards.h"

#include "base/str.h"

static const str8 SYS_SCORES_ERR_UNAVAILABLE = str8_lit_comp("scoreboards unavailable");

int
sys_scores_queries_clear_queue(void)
{
	return 0;
}

int
sys_scores_mutations_clear_queue(void)
{
	return 0;
}

int
sys_score_add(
	str8 board_id,
	u32 value,
	sys_scores_req_callback callback,
	void *userdata)
{
	if(callback) {
		struct sys_scores_res res = {
			.type          = SYS_SCORE_RES_SCORES_ADD,
			.error_message = SYS_SCORES_ERR_UNAVAILABLE,
		};
		callback(0, res, userdata);
	}
	return -1;
}

int
sys_scores_get(
	str8 board_id,
	enum sys_scores_scope scope,
	sys_scores_req_callback callback,
	void *userdata,
	struct alloc alloc)
{
	if(callback) {
		struct sys_scores_res res = {
			.type          = SYS_SCORE_RES_SCORES_GET,
			.error_message = SYS_SCORES_ERR_UNAVAILABLE,
		};
		callback(0, res, userdata);
	}
	return -1;
}

int
sys_scores_personal_best_get(str8 board_id, sys_scores_req_callback callback, void *userdata)
{
	if(callback) {
		struct sys_scores_res res = {
			.type          = SYS_SCORE_RES_SCORES_PERSONAL_BEST_GET,
			.error_message = SYS_SCORES_ERR_UNAVAILABLE,
		};
		callback(0, res, userdata);
	}
	return -1;
}
