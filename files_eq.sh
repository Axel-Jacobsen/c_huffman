#! /usr/bin/env sh

./huff $1 >/dev/null
cmp --silent out/out.txt $1 && (echo "success"; exit 0) || (echo "failure"; exit 1)

