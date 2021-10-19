build:
	@clang -Wall -Ofast -o huff chuff.c

prf:
	@clang -Wall -g -lprofiler -Ofast -o huff chuff.c

debug:
	@clang -Wall -g -O0 -o huff chuff.c
