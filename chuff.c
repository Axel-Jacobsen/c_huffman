#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define TOKEN_LEN 8 // right now only a token len of 8 bits
#define TOKEN_SET_LEN 1 << TOKEN_LEN

typedef _Bool bool;

typedef struct {
	struct Node * l, * r;
	unsigned char value;
	unsigned int count;
	bool is_leaf;
} Node;


// ------------------------------

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
	void* cf_arr = calloc((size_t) TOKEN_SET_LEN, sizeof(uint64_t));
	if (cf_arr)
		return cf_arr;
	return NULL;
}

void calculate_char_freqs(FILE* f, uint64_t* freq_arr)
{
	if (ftell(f) != 0)
		fseek(f, 0L, SEEK_SET);

	int c;
	while ((c = fgetc(f)) != EOF)
	{
		++freq_arr[c];
	}
	fseek(f, 0L, SEEK_SET);
}

uint64_t max_idx(uint64_t* freq_arr)
{
	// Unsorted, so algo is just linear search
	unsigned int val = 0;
	uint64_t max_count = 0;
	for (unsigned int i = 0; i < TOKEN_SET_LEN; ++i)
	{
		if (freq_arr[i] > max_count)
		{
			max_count = freq_arr[i];
			val = i;
		}
	}
	return val;
}

Node build_tree(FILE* f, uint64_t* freq_arr)
{
	bool in_tree[TOKEN_SET_LEN] = {0};

	for(;;){
		unsigned int max_token = max_idx(freq_arr);
		if (!in_tree[max_token])
		{
			in_tree[max_token] = 1;
			Node N = {
				.l = NULL,
				.r = NULL,
				.value = max_token,
				.count = freq_arr[max_token],
				.is_leaf = 1
			};
		}
	}
}

void print_char_freqs(uint64_t* freq_arr)
{
	for (int i = 0; i < (TOKEN_SET_LEN); ++i)
	{
		printf("%llu, ", (uint64_t)freq_arr[i]);
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

	uint64_t v = max_idx(cf_arr);
	printf("\n%llu %llu\n", v, cf_arr[v]);
	// Build Huffman Tree with Frequencies
	//
	/* Tree tree = build_tree(infile, cf_arr); */

	// Write file with Huffman Tree Symbols
	free(cf_arr); cf_arr = NULL;
	fclose(infile);
}
