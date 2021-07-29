build:
	@clang -Wall -Ofast -o huff chuff.c

prf:
	@clang -Wall -g -lprofiler -Ofast -o huff chuff.c
