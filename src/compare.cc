// compare.cc
// Runs several predictors over one trace in a single pass and prints the
// direction MPKI of each of them. Used for the comparison in the report.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "branch.h"
#include "trace.h"
#include "predictor.h"
#include "my_predictor.h"

// 4096-entry bimodal predictor alone (the bimodal part of pm_predictor)
class bimodal_predictor : public branch_predictor {
public:
	branch_update u;
	branch_info bi;
	unsigned char tab[1<<PM_BIMODAL_BITS];

	bimodal_predictor (void) {
		memset (tab, 1, sizeof (tab));
	}

	branch_update *predict (branch_info & b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL)
			u.direction_prediction (tab[b.address & ((1<<PM_BIMODAL_BITS)-1)] >> 1);
		else
			u.direction_prediction (true);
		u.target_prediction (0);
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned char *c = &tab[bi.address & ((1<<PM_BIMODAL_BITS)-1)];
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}
		}
	}
};

#define N 5

int main (int argc, char *argv[]) {
	if (argc != 2) {
		fprintf (stderr, "Usage: %s <filename>.gz\n", argv[0]);
		exit (1);
	}
	init_trace (argv[1]);

	branch_predictor *p[N] = {
		new gshare_predictor (),			// sample gshare
		new bimodal_predictor (),			// bimodal only
		new pm_predictor (12, 9, 6, 4, 15, 0, true),	// 4-way, allocate on any miss
		new pm_predictor (12, 10, 5, 2),		// 2-way global predictor
		new pm_predictor ()				// 4-way global predictor (default)
	};
	long long int dmiss[N] = { 0 };

	for (;;) {
		trace *t = read_trace ();
		if (!t) break;
		for (int i = 0; i < N; i++) {
			branch_update *u = p[i]->predict (t->bi);
			if (t->bi.br_flags & BR_CONDITIONAL)
				dmiss[i] += u->direction_prediction () != t->taken;
			p[i]->update (u, t->taken, t->target);
		}
	}
	end_trace ();

	for (int i = 0; i < N; i++) printf ("%0.3f ", 1000.0 * (dmiss[i] / 1e8));
	printf ("\n");
	exit (0);
}
