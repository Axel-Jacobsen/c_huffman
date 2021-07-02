#! /usr/bin/env sh

all_eq=1;
RED='\033[0;31m'
GRN='\033[0;32m'
NC='\033[0m'

if [[ -d $1 ]]; then
	for f in "$1"/*; do
		if [[ $f != *.pine ]]; then
			./huff $f;
			./huff -d -f out.txt $f.pine;
			cmp out.txt $f && (echo "$GRN$f$NC";) || (echo "$RED$f$NC";)
			all_eq=$all_eq && [[ ! $? ]];
		fi
	done
elif [[ -f $1 ]]; then
	./huff $1;
	./huff -d -f out.txt $1.pine;
	cmp out.txt $f && (echo "$GRN$f$NC";) || (echo "$RED$f$NC";)
	all_eq=$all_eq && [[ ! $? ]];
else
	echo "$1 is not valid - must be file or directory"
	exit 1
fi

rm out.txt
exit $all_eq
