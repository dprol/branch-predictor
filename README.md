Pentium M Hybrid Branch Predictor (Assignment 1)

Branch direction predictor for the CBP-2 infrastructure, based on the Pentium M
outcome predictors: a bimodal table plus a tagged global predictor. The loop
predictor and branch target prediction are not implemented.

Files
* src/my_predictor.h -- `pm_predictor` (the assignment); `gshare_predictor` is the original sample
* src/predict.cc -- original driver, runs `pm_predictor` on one trace
* src/test_example.cc -- checks `pm_predictor` against the worked example of the handout (Appendix B)
* src/compare.cc -- runs gshare, bimodal only and three `pm_predictor` configurations in one pass
* run -- original script (needs csh and dc)
* run.sh, compare.sh -- bash scripts for all the traces
* results.txt, comparison.txt -- output of `./run.sh traces` and `./compare.sh traces`
* REPORT.md -- report

How to build and run

    cd src
    make                      # builds predict (and predict_extra_credit, not implemented)
    cd ..
    ./run traces              # or ./run.sh traces if csh/dc are not installed

    cd src
    make test_example && ./test_example     # handout example, prints "0 mismatches"
    make compare
    cd ..
    ./compare.sh traces       # MPKI of each predictor for each trace

The traces are read with `/usr/bin/bzip2 -dc` (see src/trace.h).

Result: average 5.848 MPKI over the 20 traces (sample gshare: 6.306).

Configuration (constructor of `pm_predictor`, defaults in parentheses)
* bimodal_bits (12) -- 4096 2-bit counters
* set_bits (9), ways (4) -- global predictor of 512 sets x 4 ways = 2048 entries
* tag_bits (6)
* history_length (15) -- global history register, used instead of the PIR
* address_shift (0) -- low address bits to drop before indexing
* alloc_on_any_miss (false) -- true gives the simplified allocation of the handout example

No external code was used; the set-associative lookup and the LRU replacement of the
global predictor are in `pm_predictor` itself.
