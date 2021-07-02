#! /usr/bin/env sh

all_eq=1;

if [[ -d $1 ]]; then
	for f in "$1"/*; do
		if [[ $f != *.pine ]]; then
			./huff $f >/dev/null;
			cmp --silent out/out.txt $f && (echo "$f success"; exit 0) || (echo "$f failure"; exit 1)
			all_eq=$all_eq && [[ ! $? ]];
		fi
	done
elif [[ -f $1 ]]; then
	./huff $1 >/dev/null;
	cmp --silent out/out.txt $f && (echo "$f success"; exit 0) || (echo "$f failure"; exit 1)
	all_eq=$all_eq && [[ ! $? ]];
else
	echo "$1 is not valid - must be file or directory"
	exit 1
fi

exit $all_eq
