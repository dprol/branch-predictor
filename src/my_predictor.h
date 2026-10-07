// my_predictor.h
// This file contains a sample gshare_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.

class gshare_update : public branch_update {
public:
	unsigned int index;
};

class gshare_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	15
#define TABLE_BITS	15
	gshare_update u;
	branch_info bi;
	unsigned int history;
	unsigned char tab[1<<TABLE_BITS];

	gshare_predictor (void) : history(0) { 
		memset (tab, 0, sizeof (tab));
	}

	branch_update *predict (branch_info & b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) {
			u.index = 
				  (history << (TABLE_BITS - HISTORY_LENGTH)) 
				^ (b.address & ((1<<TABLE_BITS)-1));
			u.direction_prediction (tab[u.index] >> 1);
		} else {
			u.direction_prediction (true);
		}
		u.target_prediction (0);
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			unsigned char *c = &tab[((gshare_update*)u)->index];
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}
			history <<= 1;
			history |= taken;
			history &= (1<<HISTORY_LENGTH)-1;
		}
	}
};

//
// Pentium M hybrid branch predictors
// This class implements a simple hybrid branch predictor based on the Pentium M branch outcome prediction units. 
// Instead of implementing the complete Pentium M branch outcome predictors, the class below implements a hybrid 
// predictor that combines a bimodal predictor and a global predictor. 
//
// Bimodal predictor: a table of 2-bit counters indexed by the branch address.
// Global predictor: a tagged, set-associative cache of 2-bit counters. The
// branch address is XORed with the global history register (used here in
// place of the PIR); the upper bits of the result select the set and the
// lower bits are the tag. On a global hit the global counter gives the final
// prediction, otherwise the bimodal counter does.
class pm_update : public branch_update {
public:
        unsigned int index;	// bimodal table index
        unsigned int set;	// global predictor set
        unsigned int tag;	// global predictor tag
        int way;		// hit way in the global predictor, -1 if no hit
        bool conditional;
};

class pm_predictor : public branch_predictor {
public:
#define PM_BIMODAL_BITS	12	// 4096 bimodal counters
#define PM_SET_BITS	9	// 512 sets ...
#define PM_WAYS		4	// ... x 4 ways = 2048 global entries
#define PM_TAG_BITS	6
#define PM_HISTORY_LENGTH	15
        pm_update u;
        int bimodal_bits, set_bits, tag_bits, ways, history_length, address_shift;

        // allocation policy of the global predictor:
        // alloc_on_any_miss = false: a new entry is allocated only when the
        //   branch was mispredicted, with its counter set to the weak state
        //   of the actual outcome (the default).
        // alloc_on_any_miss = true: an entry is allocated on every global
        //   miss and it inherits the counter of the entry it replaces (the
        //   simplified scheme of the worked example in the handout).
        bool alloc_on_any_miss;

        unsigned int history;
        unsigned char *bimodal;	// 2-bit counters
        unsigned char *counter;	// 2-bit counters, [set * ways + way]
        unsigned char *valid;
        unsigned int *tags;
        unsigned int *age;	// LRU age, the largest one in a set is the victim

        pm_predictor (int bimodal_bits = PM_BIMODAL_BITS, int set_bits = PM_SET_BITS,
                      int tag_bits = PM_TAG_BITS, int ways = PM_WAYS,
                      int history_length = PM_HISTORY_LENGTH, int address_shift = 0,
                      bool alloc_on_any_miss = false) :
                bimodal_bits(bimodal_bits), set_bits(set_bits), tag_bits(tag_bits),
                ways(ways), history_length(history_length), address_shift(address_shift),
                alloc_on_any_miss(alloc_on_any_miss), history(0) {
            int entries = (1 << set_bits) * ways;
            bimodal = new unsigned char[1 << bimodal_bits];
            counter = new unsigned char[entries];
            valid = new unsigned char[entries];
            tags = new unsigned int[entries];
            age = new unsigned int[entries];
            memset (bimodal, 1, 1 << bimodal_bits);	// weakly not taken
            memset (counter, 1, entries);
            memset (valid, 0, entries);
            memset (tags, 0, entries * sizeof (unsigned int));
            memset (age, 0, entries * sizeof (unsigned int));
        }

        ~pm_predictor (void) {
            delete[] bimodal;
            delete[] counter;
            delete[] valid;
            delete[] tags;
            delete[] age;
        }

        branch_update *predict (branch_info & b) {
            u.conditional = b.br_flags & BR_CONDITIONAL;
            u.way = -1;
            if (u.conditional) {
                unsigned int address = b.address >> address_shift;

                // bimodal outcome prediction
                u.index = address & ((1 << bimodal_bits) - 1);
                bool prediction = bimodal[u.index] >> 1;

                // global predictor lookup; a hit overrides the bimodal prediction
                unsigned int hash = (address ^ history) & ((1 << (set_bits + tag_bits)) - 1);
                u.set = hash >> tag_bits;
                u.tag = hash & ((1 << tag_bits) - 1);
                for (int w = 0; w < ways; w++) {
                    int i = u.set * ways + w;
                    if (valid[i] && tags[i] == u.tag) {
                        u.way = w;
                        prediction = counter[i] >> 1;
                        break;
                    }
                }
                u.direction_prediction (prediction);
            } else {
                u.direction_prediction (true);
            }

            // branch target prediction is not implemented
            u.target_prediction (0);

            return &u;
        }

        void update (branch_update *u, bool taken, unsigned int target) {
            pm_update *pu = (pm_update*)u;
            if (!pu->conditional) return;
            int base = pu->set * ways;
            if (pu->way >= 0) {
                update_counter (&counter[base + pu->way], taken);
                touch (base, pu->way);
            } else if (alloc_on_any_miss || pu->direction_prediction () != taken) {
                // victim: first invalid way, otherwise the least recently used one
                int victim = -1;
                for (int w = 0; w < ways && victim < 0; w++)
                    if (!valid[base + w]) victim = w;
                if (victim < 0) {
                    victim = 0;
                    for (int w = 1; w < ways; w++)
                        if (age[base + w] > age[base + victim]) victim = w;
                }
                valid[base + victim] = 1;
                tags[base + victim] = pu->tag;
                if (alloc_on_any_miss)
                    update_counter (&counter[base + victim], taken);
                else
                    counter[base + victim] = taken ? 2 : 1;
                touch (base, victim);
            }
            update_counter (&bimodal[pu->index], taken);
            history <<= 1;
            history |= taken;
            history &= (1 << history_length) - 1;
        }

private:
        void update_counter (unsigned char *c, bool taken) {
            if (taken) {
                if (*c < 3) (*c)++;
            } else {
                if (*c > 0) (*c)--;
            }
        }

        // mark a way as the most recently used one of its set
        void touch (int base, int way) {
            for (int w = 0; w < ways; w++) age[base + w]++;
            age[base + way] = 0;
        }
};

//
// Complete Pentium M branch predictors for extra credit
// This class implements the complete Pentium M branch prediction units. 
// It implements both branch target prediction and branch outcome predicton. 
class cpm_update : public branch_update {
public:
        unsigned int index;
};

class cpm_predictor : public branch_predictor {
public:
        cpm_update u;

        cpm_predictor (void) {
        }

        branch_update *predict (branch_info & b) {
            u.direction_prediction (true);
            u.target_prediction (0);
            return &u;
        }

        void update (branch_update *u, bool taken, unsigned int target) {
        }

};


