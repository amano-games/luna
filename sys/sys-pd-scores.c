#include "base/dbg.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "base/utils.h"
#include "sys/sys-scoreboards.h"
#include "sys/sys-pd-scores.h"

#define PD_SCORES_ADD_MAX_RETRY 3

enum pd_scores_req_type {
	PD_SCORES_REQ_TYPE_NONE,

	PD_SCORES_REQ_TYPE_GET,
	PD_SCORES_REQ_TYPE_ADD,
	PD_SCORES_REQ_TYPE_PERSONAL_BEST_GET,

	PD_SCORES_REQ_TYPE_NUM_COUNT,
};

struct pd_scores_req_get {
	str8 board_id;
	struct alloc alloc;
};

struct pd_scores_req_personal_best {
	str8 board_id;
};

struct pd_scores_req_add {
	usize attemps;
	str8 board_id;
	u32 value;
};

struct pd_scores_req {
	u32 id;
	enum pd_scores_req_type type;
	enum sys_scores_req_state state;
	sys_scores_req_callback callback;
	union {
		struct pd_scores_req_get get;
		struct pd_scores_req_add add;
		struct pd_scores_req_personal_best personal_best;
	};
	void *userdata;
};

struct pd_scores_state {
	u32 next_id;
	b16 busy;
	u8 start;
	u8 end;
	struct pd_scores_req reqs[20];
};

static struct pd_scores_state SCORES_QUERIES_STATE;
static struct pd_scores_state SCORES_MUTATIONS_STATE;

void pd_scores_start_next(struct pd_scores_state *state);
void pd_get_scores_callback(PDScoresList *scores, const char *error_message);
void pd_add_score_callback(PDScore *score, const char *error_message);
void pd_personal_best_get_callback(PDScore *score, const char *error_message);

int
sys_scores_queries_clear_queue(void)
{
	int res                       = 0;
	struct pd_scores_state *state = &SCORES_QUERIES_STATE;
	log_info("sys-scores", "Clear scores queries queue, start: %d, end: %d", (int)state->start, (int)state->end);
	if(!state->busy) {
		state->start = 0;
		state->end   = 0;
	} else {
		state->end = (state->start + 1) % ARRLEN(state->reqs);
	}
	return res;
}

int
sys_scores_mutations_clear_queue(void)
{
	int res                       = 0;
	struct pd_scores_state *state = &SCORES_MUTATIONS_STATE;
	log_info("sys-scores", "Clear scores mutations queue, start: %d, end: %d", (int)state->start, (int)state->end);
	if(!state->busy) {
		state->start = 0;
		state->end   = 0;
	} else {
		state->end = (state->start + 1) % ARRLEN(state->reqs);
	}
	return res;
}

int
sys_score_add(
	str8 board_id,
	u32 value,
	sys_scores_req_callback callback,
	void *userdata)
{
	dbg_check(value != 0, "sys-scores", "Submited value of 0");
	struct pd_scores_state *state = &SCORES_MUTATIONS_STATE;
	u8 next                       = (state->end + 1) % ARRLEN(state->reqs);

	dbg_check(next != state->start, "sys-scores", "Score add queue Full");
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));
	struct pd_scores_req *req = state->reqs + state->end;
	req->type                 = PD_SCORES_REQ_TYPE_ADD;
	req->userdata             = userdata;
	req->callback             = callback;
	req->id                   = state->next_id++;
	req->state                = SYS_SCORE_REQ_STATE_QUEUE;
	req->add.board_id         = board_id; // TODO: copy board_id
	req->add.value            = value;
	req->add.attemps          = 0;
	log_info("sys-scores", "Queue add score for %s: %" PRIu32 "", req->add.board_id.str, req->add.value);
	state->end = next;

	if(!state->busy) { pd_scores_start_next(state); }

	return 0;

error:
	return -1;
}

int
sys_scores_get(
	str8 board_id,
	sys_scores_req_callback callback,
	void *userdata,
	struct alloc alloc)
{
	struct pd_scores_state *state = &SCORES_QUERIES_STATE;
	u8 next                       = (state->end + 1) % ARRLEN(state->reqs);

	dbg_check(next != state->start, "sys-scores", "Scores get queue Full");
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));
	struct pd_scores_req *req = state->reqs + state->end;
	req->type                 = PD_SCORES_REQ_TYPE_GET;
	req->userdata             = userdata;
	req->callback             = callback;
	req->id                   = state->next_id++;
	req->state                = SYS_SCORE_REQ_STATE_QUEUE;
	req->get.alloc            = alloc;
	req->get.board_id         = board_id;
	state->end                = next;

	if(!state->busy) { pd_scores_start_next(state); }

	return 0;

error:
	return -1;
}

int
sys_scores_personal_best_get(
	str8 board_id,
	sys_scores_req_callback callback,
	void *userdata)
{
	struct pd_scores_state *state = &SCORES_QUERIES_STATE;
	u8 next                       = (state->end + 1) % ARRLEN(state->reqs);

	dbg_check(next != state->start, "sys-scores", "Personal best queue Full");
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));
	struct pd_scores_req *req   = state->reqs + state->end;
	req->type                   = PD_SCORES_REQ_TYPE_PERSONAL_BEST_GET;
	req->userdata               = userdata;
	req->callback               = callback;
	req->id                     = state->next_id++;
	req->state                  = SYS_SCORE_REQ_STATE_QUEUE;
	req->personal_best.board_id = board_id;
	state->end                  = next;

	if(!state->busy) { pd_scores_start_next(state); }

	return 0;

error:
	return -1;
}

void
pd_scores_start_next(struct pd_scores_state *state)
{
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));
	dbg_assert(PD_SCORES_GET);
	dbg_assert(PD_SCORE_ADD);
	dbg_assert(PD_PERSONAL_BEST_GET);

	if(state->start == state->end) {
		state->busy = false;
		return;
	}

	struct pd_scores_req *req = state->reqs + state->start;
	state->busy               = true;
	switch(req->type) {
	case PD_SCORES_REQ_TYPE_GET: {
		PD_SCORES_GET((const char *)req->get.board_id.str, pd_get_scores_callback);
	} break;
	case PD_SCORES_REQ_TYPE_ADD: {
		log_info("sys-scores", "Adding score for %s: %" PRIu32 "", req->add.board_id.str, req->add.value);
		PD_SCORE_ADD((const char *)req->add.board_id.str, req->add.value, pd_add_score_callback);
	} break;
	case PD_SCORES_REQ_TYPE_PERSONAL_BEST_GET: {
		PD_PERSONAL_BEST_GET((const char *)req->personal_best.board_id.str, pd_personal_best_get_callback);
	} break;
	default: {
		dbg_sentinel("sys-scores");
	} break;
	}

error:
	return;
}

void
pd_add_score_callback(PDScore *score, const char *error_message)
{
	dbg_assert(PD_SCORE_FREE);
	struct pd_scores_state *state = &SCORES_MUTATIONS_STATE;
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));
	if(state->start == state->end) return; // nothing in queue

	struct pd_scores_req *req = state->reqs + state->start;
	dbg_assert(req->type == PD_SCORES_REQ_TYPE_ADD);
	struct sys_scores_res res = {.type = SYS_SCORE_RES_SCORES_ADD};

	if(error_message) {
		log_error("sys-scores", "Failed to submit score to board %s: %s", req->add.board_id.str, error_message);
		if(req->add.attemps < PD_SCORES_ADD_MAX_RETRY) {
			req->add.attemps++;
			PD_SCORE_ADD((const char *)req->add.board_id.str, req->add.value, pd_add_score_callback);
			log_info("sys-scores", "Attempt: %d, to submit score to board: %s", (int)req->add.attemps, req->add.board_id.str);
			return;
		}
		res.error_message = str8_cstr((char *)error_message);
	} else {
		log_info("sys-scores", "Submited score for board %s: %d. %s %" PRIu32 "", req->add.board_id.str, score->rank, score->player, score->value);
		res.add = (struct sys_scores_res_add){
			.score = (struct sys_score){
				.rank   = score->rank,
				.value  = score->value,
				.player = str8_cstr(score->player),
			},
		};
	}

	if(req->callback) {
		req->callback(req->id, res, req->userdata);
	}

	state->start = (state->start + 1) % ARRLEN(state->reqs);
	state->busy  = false;

	pd_scores_start_next(state);
	if(score != NULL) {
		PD_SCORE_FREE(score);
	}
}

void
pd_get_scores_callback(PDScoresList *scores, const char *error_message)
{
	dbg_assert(PD_SCORES_LIST_FREE);
	struct pd_scores_state *state = &SCORES_QUERIES_STATE;
	struct sys_scores_res res     = {.type = SYS_SCORE_RES_SCORES_GET};
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));
	if(state->start == state->end) return; // nothing in queue

	struct pd_scores_req *req = state->reqs + state->start;
	dbg_assert(req->type == PD_SCORES_REQ_TYPE_GET);

	if(error_message) {
		log_error("sys-scores", "Failed to get scores for board %s: %s", req->get.board_id.str, error_message);
		res.error_message = str8_cstr((char *)error_message);
		goto error;
	} else {
		log_info("sys-scores", "Got scores for board %s: No. of scores: %d", req->get.board_id.str, scores->count);
		for(ssize i = 0; i < (ssize)scores->count; ++i) {
			log_info("sys-scores", "%d. %s: %" PRIu32 "", scores->scores[i].rank, scores->scores[i].player, scores->scores[i].value);
		}
		res.get = (struct sys_scores_res_get){
			.board_id        = req->get.board_id,
			.last_updated    = scores->lastUpdated,
			.player_included = scores->playerIncluded,
		};

		if(scores->count > 0) {
			struct sys_score_arr *entries = &res.get.entries;
			if(req->get.alloc.allocf != NULL) {
				entries->items = alloc_arr(req->get.alloc, entries->items, scores->count);
			}
			if(entries->items == NULL) {
				log_error("sys-scores", "Failed to allocate memory for %d scores", scores->count);
			} else {
				entries->cap = scores->count;
				entries->len = scores->count;
				for(usize i = 0; i < scores->count; ++i) {
					entries->items[i] = (struct sys_score){
						.value  = scores->scores[i].value,
						.rank   = scores->scores[i].rank,
						.player = str8_cstr(scores->scores[i].player),
					};
				}
			}
		}
	}

error:
	if(req->callback) {
		req->callback(req->id, res, req->userdata);
	}

	state->start = (state->start + 1) % ARRLEN(state->reqs);
	state->busy  = false;
	if(scores != NULL) {
		PD_SCORES_LIST_FREE(scores);
	}
	pd_scores_start_next(state);
	return;
}

void
pd_personal_best_get_callback(PDScore *score, const char *error_message)
{
	dbg_assert(PD_SCORE_FREE);
	struct pd_scores_state *state = &SCORES_QUERIES_STATE;
	struct sys_scores_res res     = {.type = SYS_SCORE_RES_SCORES_PERSONAL_BEST_GET};
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));
	if(state->start == state->end) return; // nothing in queue

	struct pd_scores_req *req = state->reqs + state->start;
	dbg_assert(req->type == PD_SCORES_REQ_TYPE_PERSONAL_BEST_GET);

	if(error_message) {
		log_error("sys-scores", "Failed to get personal best for board %s: %s", req->personal_best.board_id.str, error_message);
		res.error_message = str8_cstr((char *)error_message);
	} else {
		if(score) {
			log_info("sys-scores", "Personal best for board %s: %" PRIu32 "", req->personal_best.board_id.str, score->value);
			res.personal_best = (struct sys_scores_res_personal_best){
				.score = (struct sys_score){
					.rank   = score->rank,
					.value  = score->value,
					.player = str8_cstr(score->player),
				},
			};
		} else {
			log_info("sys-scores", "No personal best for board %s", req->personal_best.board_id.str);
		}
	}

	if(req->callback) {
		req->callback(req->id, res, req->userdata);
	}

	state->start = (state->start + 1) % ARRLEN(state->reqs);
	state->busy  = false;

	pd_scores_start_next(state);
	if(score != NULL) {
		PD_SCORE_FREE(score);
	}
}
