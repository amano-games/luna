#include "sys/sys-scoreboards.h"

#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "base/utils.h"
#include "lib/rndm.h"
#include "sys/sys.h"

#define SYS_SCORES_MOCK_PLAYER_NAME "afk"

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
sys_score_add(str8 board_id, u32 value, sys_scores_req_callback callback, void *userdata)
{
	return 0;
}

int
sys_scores_get(
	str8 board_id,
	enum sys_scores_scope scope,
	sys_scores_req_callback callback,
	void *userdata,
	struct alloc alloc)
{
	i32 res          = -1;
	u32 top_score    = U32_MAX;
	u32 bottom_score = 100000;

	static const str8 names[] = {
		str8_lit_comp("Gravinger"),
		str8_lit_comp("Clabbosaurus"),
		str8_lit_comp("Markantes"),
		str8_lit_comp("kartacha"),
		str8_lit_comp("7305899826608137"),
		str8_lit_comp("Dr.Bagels"),
		str8_lit_comp("msal"),
		str8_lit_comp("4461469380168897"),
		str8_lit_comp("GentleEel"),
		str8_lit_comp("NicBran98"),
		str8_lit_comp("loosecanons"),
		str8_lit_comp("RoliRoler"),
		str8_lit_comp("fenelope"),
		str8_lit_comp("yoyogigames"),
		str8_lit_comp("fp.monkey"),
		str8_lit_comp("Simply_In"),
	};

	if(callback) {
		res                             = 0;
		i32 scores_count                = rndm_range_i32(NULL, 5, 10);
		u32 last_updated                = sys_epoch_2000(NULL);
		struct sys_scores_res score_res = {
			.type = SYS_SCORE_RES_SCORES_GET,
			.get  = {
				.board_id        = board_id,
				.last_updated    = last_updated,
				.player_included = true,
			},
		};
		struct sys_score_arr *entries = &score_res.get.entries;
		if(alloc.allocf != NULL) {
			entries->items = alloc_arr(alloc, entries->items, scores_count);
		}
		if(entries->items != NULL) {
			entries->cap = scores_count;
			entries->len = scores_count;

			for(ssize i = 0; i < scores_count; ++i) {
				str8 player;
				f32 t     = (f32)i / (f32)(scores_count - 1);
				f32 curve = t * t * t; /* quadratic falloff */
				u32 score = top_score - (u32)((top_score - bottom_score) * curve);
				score -= rndm_range_u32(NULL, 0, 20000u); /* small random jitter */

				if(i == 0) {
					player = str8_lit(SYS_SCORES_MOCK_PLAYER_NAME);
				} else {
					i32 rndm_idx = rndm_range_i32(NULL, 0, ARRLEN(names) - 1);
					player       = (names[rndm_idx]);
				}

				entries->items[i] = (struct sys_score){
					.value  = score,
					.rank   = i > 8 ? rndm_range_i32(NULL, 11, 17000) : i,
					.player = player,
				};
			}
		}
		callback(0, score_res, userdata);
	}

	return res;
}

int
sys_scores_personal_best_get(str8 board_id, sys_scores_req_callback callback, void *userdata)
{
	i32 res                         = -1;
	struct sys_scores_res score_res = {
		.type          = SYS_SCORE_RES_SCORES_PERSONAL_BEST_GET,
		.personal_best = {
			.score = {
				.rank   = 0,
				.value  = U32_MAX,
				.player = str8_lit(SYS_SCORES_MOCK_PLAYER_NAME),
			},
		},
	};

	if(callback) {
		res = 0;
		callback(0, score_res, userdata);
	}

	return res;
}
