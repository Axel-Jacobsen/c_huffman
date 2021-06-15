#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define TOKEN_LEN 8 // right now only a token len of 8 bits
#define TOKEN_SET_LEN 1 << TOKEN_LEN

typedef _Bool bool;

typedef struct Node {
	struct Node * l, * r;
	unsigned char token;
	unsigned int count;
	bool is_leaf;
} Node;

typedef struct CharCode {
	uint64_t code;
	uint64_t fin_idx;
	char token;
} CharCode;


uint64_t charset_len = 0;
// ------------------------------

uint64_t* calculate_char_freqs(FILE* f)
{
	uint64_t* freq_arr = (uint64_t*) calloc((size_t) TOKEN_SET_LEN, sizeof(uint64_t));

	if (!freq_arr)
	{
		fprintf(stderr, "char frequency allocation failed\n");
		exit(-1);
	}

	fseek(f, 0L, SEEK_SET);

	int c;
	while ((c = fgetc(f)) != EOF)
	{
		++freq_arr[c];
	}
	fseek(f, 0L, SEEK_SET);

	return freq_arr;
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

CharCode* init_charcode(uint64_t char_code, uint64_t fin_idx,	char token)
{
	CharCode* C = (CharCode*) malloc(sizeof(CharCode));
	if (C ==NULL)
	{
		printf("Failure initializing Node: %s\n", strerror(errno));
		exit(1);
	}
	C->code = char_code;
	C->fin_idx = fin_idx;
	C->token = token;
	return C;
}

Node** init_node_arr_from_chars(uint64_t* freq_arr, uint64_t num_chars)
{
	uint64_t j = 0;
	Node** node_arr = (Node**) calloc(num_chars, sizeof(Node*));
	for (uint64_t i = 0; i < TOKEN_SET_LEN; i++) {
		if (freq_arr[i] != 0) {
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
	swap_idxs(node_arr, lidx, slidx, max_idx);
	return lowest_pair;
}

/* There is probably a more efficient way to construct this tree.
 * Come back to optimize this.
 */
Node* build_tree(FILE* f, uint64_t* freq_arr)
{
	uint64_t max_idx = get_num_chars(freq_arr);
	charset_len = max_idx;

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

// Recursive call to get tree depth
// use cnt = 0 at top level
unsigned int tree_depth(Node* N, unsigned int cnt)
{
	if (N == NULL) return cnt;
	return fmax(tree_depth(N->l, cnt+1), tree_depth(N->r, cnt+1));
}

/*
 * Traverse tree
 */
void traverse_helper(Node* N, CharCode* cur_cmprs, CharCode** write_table)
{
	if (N->is_leaf) {
		cur_cmprs->token = N->token;
		write_table[N->token] = cur_cmprs;
		return;
	}
	CharCode* left = init_charcode(
			cur_cmprs->code << 1 | 0x0,
			cur_cmprs->fin_idx+1,
			0x00
	);
	CharCode* right = init_charcode(
			cur_cmprs->code << 1 | 0x1,
			cur_cmprs->fin_idx+1,
			0x00
	);
	traverse_helper(N->l, left, write_table);
	traverse_helper(N->r, right, write_table);
}

CharCode** traverse_tree(Node* N)
{
	CharCode** md_arr = (CharCode**) malloc(sizeof(CharCode*) * charset_len);
	CharCode* first_charcode = init_charcode(0, 0, 0x00);
	traverse_helper(N, first_charcode, md_arr);
	return md_arr;
}

// i love recursion
void free_tree(Node* N)
{
	if (N == NULL)
		return;
	free_tree(N->r);
	free_tree(N->l);
	free(N);
}

void print_padding (int n)
{
	for (int i = 0; i < n; i++) putchar('\t');
}

void print2DUtil(Node* root, int space)
{
	int count = 10;

	// Base case
	if (root == NULL)
		return;

	// Increase distance between levels
	space += count;

	// Process right child first
	print2DUtil(root->r, space);

	// Print current node after space
	// count
	printf("\n");
	for (int i = count; i < space; i++)
		printf(" ");

	char c[4] = "   ";
	if (root->token == '\n') {
		strcpy(c, "\\n");
	} else if (root->token == '\t') {
		strcpy(c, "\\t");
	} else if (root->token == ' ') {
		strcpy(c, "' '");
	} else {
		c[1] = root->token;
	}
	printf("%s: %d\n", c, root->count);

	// Process left child
	print2DUtil(root->l, space);
}

void printbe(uint64_t v, uint64_t max_idx)
{
	for (int j = 0; j < max_idx; j++) {
		printf("%llu", (v & 1));
		v = v >> 1;
	}
	printf("\n");
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

	uint64_t* freq_arr = calculate_char_freqs(infile);

	// Build Huffman Tree with Frequencies
	Node* tree = build_tree(infile, freq_arr);

	print2DUtil(tree, 2);

	printf("tree depth: %u\n", tree_depth(tree, 0));
	CharCode** v = traverse_tree(tree);
	for (int i = 0; i < TOKEN_SET_LEN; i++)
	{
		printf("Pointer: %p\n", (void *) v[i]);
		if (v[i])
		{
			printf("Token: %c ", v[i]->token);
			printf("Code: ");
			printbe(v[i]->code, v[i]->fin_idx);
		}
		printf("-------------\n");
	}
	// To Do
	// - How to represent write table, for variable token len? make write table
	// - how to write bytes for variable token len?
	// For now, just individual bytes

	/* print2DUtil(tree, 0); */
	free_tree(tree);



	// Write file with Huffman Tree Symbols
	free(freq_arr);
	fclose(infile);
}
