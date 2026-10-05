#!/bin/bash
# MPKI of gshare, bimodal only, and three pm_predictor configurations
# (build first with: cd src; make compare)
if [ -z "$1" ]; then
	printf "Usage: $0 <trace-file-directory>\n"
	exit 1
fi
printf "%-12s %8s %8s %8s %8s %8s\n" trace gshare bimodal pm-any pm-2way pm-4way
for i in `find $1 -name '*.trace.*' | sort`; do
	printf "%-12s" `basename $i .trace.bz2`
	./src/compare $i | awk '{ for (k = 1; k <= NF; k++) printf " %8.3f", $k; printf "\n" }'
done | awk '{ print; for (k = 2; k <= NF; k++) sum[k] += $k; n++ }
	END { printf "%-12s", "average"; for (k = 2; k <= 6; k++) printf " %8.3f", sum[k] / n; printf "\n" }'
