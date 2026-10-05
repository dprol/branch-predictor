#!/bin/bash
# bash version of ./run for systems without csh and dc
if [ -z "$1" ]; then
	printf "Usage: $0 <trace-file-directory>\n"
	exit 1
fi
if [ ! -e src/predict ]; then
	printf "predict program is not built.\n"
	exit 1
fi
for i in `find $1 -name '*.trace.*' | sort`; do
	mpki=`./src/predict $i | tail -1 | sed -e '/MPKI/s///'`
	printf "%-40s\t%0.3f\n" $i $mpki
done | awk '{ print; sum += $NF; n++ } END { printf "average MPKI: %0.3f\n", sum / n }'
exit 0
