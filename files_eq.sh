#! /usr/bin/env sh

if [[ -d $1 ]]; then
	for f in "$1"/*; do
		if [[ $f != *.pine ]]; then
			./huff $f >/dev/null;
			cmp --silent out/out.txt $f && (echo "$f success"; exit 0) || (echo "$f failure"; exit 1)
		else
			echo "$f skipped"
		fi
	done
elif [[ -f $1 ]]; then
	./huff $1 >/dev/null;
	cmp --silent out/out.txt $f && (echo "$f success"; exit 0) || (echo "$f failure"; exit 1)
else
	echo "$1 is not valid"
	exit 1
fi

