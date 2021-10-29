build:
	@clang -Wall -Wextra -Ofast -o huff chuff.c

prf:
	@clang -Wall -Wextra -g -lprofiler -Ofast -o huff chuff.c

debug:
	@clang -Wall -Wextra -g -O0 -o huff chuff.c

.PHONY: clean
clean:
	@# force to supress complaints when there aren't .pine files.
	@# probably not good practice, but I dislike complaining!
	@rm -f *.pine testfiles/*.pine
