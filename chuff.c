#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#define TOKEN_LEN 8

typedef _Bool bool;

typedef struct {
	struct Node* lchild;
	struct Node* rchild;
	unsigned char value;
	bool is_leaf;
} Node;


long get_file_len(FILE* f)
{
	fseek(f, 0L, SEEK_END);
	long len = ftell(f);
	fseek(f, 0L, SEEK_SET);
	return len;
}

/* Allocate enough memory for character frequencies
 */
void* init_char_freq_arr()
{
	void* cf_arr = calloc((size_t) 1 << TOKEN_LEN, sizeof(uint64_t));
	if (cf_arr)
		return cf_arr;
	return NULL;
}


void calculate_char_freqs(FILE* f, uint64_t* char_arr)
{
	fseek(f, 0L, SEEK_SET);
	int c;
	
	while ((c = fgetc(f)) != EOF)
	{
		++char_arr[c];
	}
}


void print_char_freqs(uint64_t* char_arr)
{
	for (int i = 0; i < (1 << TOKEN_LEN); ++i)
	{
		printf("%llu, ", (uint64_t)char_arr[i]);
	}
}


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "incorrect number of input arguments; usage: ./a.out <file> \n");
		exit(-1);
	}

	FILE *infile;
	infile = fopen(argv[1], "rb");

	if (!infile)
	{
		fprintf(stderr, "failed to open %s\n", argv[1]);
		exit(-1);
	}

	// Get Char Frequencies
	//	- create array of correct size (cf_arr)
	//	  - max size is 2^TOKEN_LEN
	//	  - cell size for now is uint64_t - may be able to do varying type with switch statements
	//		- max filesize: 2^64 bytes
	//    - size_t cell_size = (size_t) ceil(log2(file_len) / 8);
	//	- for char in file:
	//	  - convert token to int/long/etc
	//	  - cf_arr[token]++
	uint64_t* cf_arr = init_char_freq_arr();
	if (!cf_arr)
	{
		fprintf(stderr, "char frequency allocation failed\n");
		exit(-1);
	}

	calculate_char_freqs(infile, cf_arr);
	print_char_freqs(cf_arr);

	// Build Huffman Tree with Frequencies
	// Write file with Huffman Tree Symbols

	fclose(infile);
}
