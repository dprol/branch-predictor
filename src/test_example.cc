// test_example.cc
// Checks pm_predictor against the worked example of the assignment handout
// (Appendix B, "Hybrid Predictor Example"): 8-bit addresses, 16 bimodal
// counters, a 2-way global predictor with 4 sets and 2-bit tags, 4-bit GHR.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "branch.h"
#include "predictor.h"
#include "my_predictor.h"

int main (void) {
	const unsigned int b1 = 0x44, b2 = 0x6C, b3 = 0x90;
	const int n = 22;
	unsigned int address[n] = { b1, b3, b1, b2, b3, b1, b3, b1, b2, b3, b1,
		b2, b1, b2, b3, b1, b3, b1, b2, b3, b1, b3 };
	bool taken[n] = { 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1 };
	// final prediction and global hit columns of the handout table
	bool expected[n] = { 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1 };
	bool hit[n] = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0 };

	pm_predictor p (4, 2, 2, 2, 4, 2, true);

	// initial state used by the handout
	for (int i = 0; i < 16; i++) p.bimodal[i] = i % 4;
	for (int set = 0; set < 4; set++) {
		p.counter[set * 2] = set;
		p.counter[set * 2 + 1] = 3 - set;
		p.age[set * 2] = 1;
	}

	int errors = 0;
	for (int i = 0; i < n; i++) {
		branch_info bi;
		bi.address = address[i];
		bi.opcode = 0;
		bi.br_flags = BR_CONDITIONAL;
		pm_update *u = (pm_update*)p.predict (bi);
		bool ok = u->direction_prediction () == expected[i] && (u->way >= 0) == hit[i];
		printf ("%2d  set %d tag %d  %-6s  predict %-2s  actual %-2s  %s\n", i + 1,
			u->set, u->tag, u->way >= 0 ? "hit" : "no hit",
			u->direction_prediction () ? "T" : "NT", taken[i] ? "T" : "NT",
			ok ? "" : "MISMATCH");
		errors += !ok;
		p.update (u, taken[i], 0);
	}
	printf ("%d mismatches\n", errors);
	return errors != 0;
}
