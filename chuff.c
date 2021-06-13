#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define TOKEN_LEN 8 // right now only a token len of 8 bits
#define TOKEN_SET_LEN 1 << TOKEN_LEN
#define COUNT 10

typedef _Bool bool;

typedef struct Node {
	struct Node * l, * r;
	unsigned char token;
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


uint64_t get_num_chars(uint64_t* freq_arr)
{
	uint64_t num_chars = 0;
	for (uint64_t i = 0; i < TOKEN_SET_LEN; ++i)
		if (freq_arr[i] != 0)
			++num_chars;
	return num_chars;
}

Node* init_node(Node* n1, Node* n2, unsigned char tkn, unsigned int cnt, bool is_leaf)
{
	Node* N = (Node*) malloc(sizeof(Node));
	if (N == NULL)
	{
		printf("Failure initializing Node: %s\n", strerror(errno));
		exit(1);
	}
	N->l = n1;
	N->r = n2;
	N->token = tkn;
	N->count = cnt;
	N->is_leaf = is_leaf;
	return N;
}

Node** init_node_arr_from_chars(uint64_t* freq_arr, uint64_t num_chars)
{
	uint64_t j = 0;
	Node** node_arr = (Node**) calloc(num_chars, sizeof(Node*));
	for (uint64_t i = 0; i < TOKEN_SET_LEN; i++) {
		if (freq_arr[i] != 0) {
			char c[3] = "  ";
			if ((char) i == '\n')
			{
				strcpy(c, "\\n");
			}
			else if ((char) i == '\t'){
				strcpy(c, "\\t");
			}else {
				c[1] = (char)i;
			}
			printf("char %s\n", c);
			node_arr[j] = init_node(NULL, NULL, i, freq_arr[i], 1);
			++j;
		}
	}
	return node_arr;
}

void swap_idxs(Node** node_arr, uint64_t lidx, uint64_t slidx, uint64_t max_idx)
{
	// Know lidx != slidx
	// ensure lidx  == max_idx - 1 or max_idx - 2
	// ensure slidx == max_idx - 1 or max_idx - 2
	// For now, restrict lidx == max_idx - 1, slidx == max_idx - 2
	if (lidx != max_idx - 1)
	{
		Node* tmp = node_arr[lidx];
		node_arr[lidx] = node_arr[max_idx - 1];
		node_arr[max_idx - 1] = tmp;
	}
	if (slidx != max_idx - 2)
	{
		Node* tmp = node_arr[slidx];
		node_arr[slidx] = node_arr[max_idx - 2];
		node_arr[max_idx - 2] = tmp;
	}
}


Node** get_min_two(Node** node_arr, uint64_t max_idx)
{
	uint64_t lowest = UINT_MAX;
	uint64_t second_lowest = UINT_MAX;
	unsigned int lidx = max_idx - 1;
	unsigned int slidx = max_idx - 2;
	Node** lowest_pair = (Node**) calloc(2, sizeof(Node*));

	for (unsigned int i = 0; i < max_idx; i++)
	{
		if (node_arr[i]->count < lowest)
		{
			lowest = node_arr[i]->count;
			lowest_pair[0] = node_arr[i];
			lidx = i;
		}
		else if (node_arr[i]->count < second_lowest)
		{
			second_lowest = node_arr[i]->count;
			lowest_pair[1] = node_arr[i];
			slidx = i;
		}
	}
	swap_idxs(node_arr, lidx, slidx, max_idx);
	return lowest_pair;
}

/* There is probably a more efficient way to construct this tree.
 * Come back to optimize this.
 */
Node* build_tree(FILE* f, uint64_t* freq_arr)
{
	uint64_t max_idx = get_num_chars(freq_arr);
	Node** node_arr = init_node_arr_from_chars(freq_arr, max_idx);
	Node* fin_node;

	// when n == 1, return node in array
	while (max_idx > 1) {
		// Find two lowest value nodes in node arr, w/ len number giving max len, and their indicies
		Node** min_two = get_min_two(node_arr, max_idx);
		// Create node w/ the two lowest as children
		unsigned int count = min_two[0]->count + min_two[1]->count;
		Node* N = init_node(min_two[0], min_two[1], 0x00, count, 0);
		free(min_two);
		// remove the original two lowest value nodes, insert new node, decrease len number
		// we can remove the nodes since we can free the node mem in the tree, not in the arr
		node_arr[max_idx - 2] = N;
		max_idx--;
		fin_node = N;
	}
	return fin_node;
}

void free_tree(Node* N)
{
	if (N == NULL)
		return;
	free_tree(N->r);
	free_tree(N->l);
	free(N);
}

void padding (int n)
{
	int i;
	for ( i = 0; i < n; i++ )
		putchar ('\t');
}

void print2DUtil(Node* root, int space)
{
	// Base case
	if (root == NULL)
		return;

	// Increase distance between levels
	space += COUNT;

	// Process right child first
	print2DUtil(root->r, space);

	// Print current node after space
	// count
	printf("\n");
	for (int i = COUNT; i < space; i++)
		printf(" ");

		char c[4] = "   ";
		if (root->token == '\n')
		{
			strcpy(c, "\\n");
		}
		else if (root->token == '\t'){
			strcpy(c, "\\t");
		}else if (root->token == ' '){
			strcpy(c, "' '");
		}else {
			c[1] = root->token;
		}
	printf("%s: %d\n", c, root->count);

	// Process left child
	print2DUtil(root->l, space);
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
	//  - create array of correct size (cf_arr)
	//    - max size is 2^TOKEN_LEN
	//    - cell size for now is uint64_t - may be able to do varying type with switch statements
	//    - max filesize: 2^64 bytes
	//    - size_t cell_size = (size_t) ceil(log2(file_len) / 8);
	//  - for char in file:
	//    - convert token to int/long/etc
	//    - cf_arr[token]++
	uint64_t* cf_arr = init_char_freq_arr();
	if (!cf_arr)
	{
		fprintf(stderr, "char frequency allocation failed\n");
		exit(-1);
	}

	calculate_char_freqs(infile, cf_arr);

	// Build Huffman Tree with Frequencies
	Node* tree = build_tree(infile, cf_arr);
	print2DUtil(tree, 0);
	free_tree(tree);

	// Write file with Huffman Tree Symbols
	free(cf_arr); cf_arr = NULL;
	fclose(infile);
}
