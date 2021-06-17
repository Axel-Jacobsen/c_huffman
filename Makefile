build:
	@clang -Wall -o huff chuff.c

debug:
	@clang -g -Wall -o huff chuff.c
