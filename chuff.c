#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "chuff.h"

#define TOKEN_LEN 8 // right now only a token len of 8 bits
#define TOKEN_SET_LEN 1 << TOKEN_LEN

/* Errors:
 * 
 *	free_tree: two classes of errors
 *		- segfault - freeing the same node twice, implying tree is graph
 *		- free non-malloc'd mem - implying... huh?
 *		these two imply graph creation is messed up somehow. if instead of
 *		freeing you print, you see multiple nodes occurring frequently.
 *		occurs also for only some infiles of larger size.
 *
 *		From "assert_tree_creation.py", we know that breaking files have
 *		cyclic graphs, instead of trees! Can start narrowing down these
 *		issues
 */

uint64_t* calculate_char_freqs(FILE* f) {
	uint64_t* freq_arr = (uint64_t*) calloc((size_t) TOKEN_SET_LEN, sizeof(uint64_t));
	if (!freq_arr) {
		fprintf(stderr, "char frequency allocation failed\n");
		exit(1);
	}

	fseek(f, 0L, SEEK_END);
	uint64_t flen = ftell(f);
	fseek(f, 0L, SEEK_SET);

	uint8_t c = 0;
	for (int i = 0; i < flen; i++) {
		fread((void*)&c, 1, 1, f);
		freq_arr[c]++;
	}
	fseek(f, 0L, SEEK_SET);
	return freq_arr;
}

uint64_t get_num_chars(uint64_t* freq_arr) {
	uint64_t num_chars = 0;
	for (uint64_t i = 0; i < TOKEN_SET_LEN; i++)
		if (freq_arr[i] != 0)
			++num_chars;
	return num_chars;
}

Node* init_node(Node* n1, Node* n2, uint8_t tkn, uint64_t cnt, bool is_leaf) {
	Node* N = (Node*) malloc(sizeof(Node));
	if (!N) {
		printf("Failure initializing Node: %s\n", strerror(errno));
		exit(1);
	} else if (n1 != NULL && n2 != NULL && tkn != 0) {
		fprintf(stderr, "initializing node with non-null children and token \n");
		exit(1);
	}
	printf("ME: %p  L: %p  R: %p  TKN: ", (void*) N, (void*) n1, (void*) n2);
	printle(tkn, 8);
	printf("  count: %llu  is_leaf: %d\n", cnt, is_leaf);
	N->l = n1;
	N->r = n2;
	N->token = tkn;
	N->count = cnt;
	N->is_leaf = is_leaf;
	return (Node*) N;
}

CharCode* init_charcode(uint64_t char_code, uint64_t fin_idx,	uint8_t token) {
	CharCode* C = (CharCode*) malloc(sizeof(CharCode));
	if (C ==NULL) {
		printf("Failure initializing CharCode: %s\n", strerror(errno));
		exit(1);
	}
	C->code = char_code;
	C->fin_idx = fin_idx;
	C->token = token;
	return C;
}

Node** init_node_arr_from_chars(uint64_t* freq_arr, uint64_t num_chars) {
	uint64_t j = 0;
	if (num_chars > SIZE_MAX) { // DLTBL
		printf("HACF\n" );
		exit(1);
	}
	Node** node_arr = (Node**) calloc(num_chars, sizeof(Node*));
	for (uint64_t i = 0; i < TOKEN_SET_LEN; i++) {
		if (freq_arr[i] != 0) {
			node_arr[j] = init_node(NULL, NULL, i, freq_arr[i], 1);
			++j;
		}
	}
	return node_arr;
}


void swap_idxs(Node** node_arr, uint64_t lidx, uint64_t slidx, uint64_t max_idx) {
	// swap 1
	Node* ldx_tmp = node_arr[slidx];
	node_arr[lidx] = node_arr[max_idx - 1];
	node_arr[max_idx - 1] = ldx_tmp;
	// swap 2
	if (slidx == max_idx - 1)
		slidx = lidx;
	Node* slidx_tmp = node_arr[slidx];
	node_arr[slidx] = node_arr[max_idx - 2];
	node_arr[max_idx - 2] = slidx_tmp;
}


Node** get_min_two(Node** node_arr, uint64_t max_idx) {
	uint64_t lowest = UINT_MAX;
	uint64_t second_lowest = UINT_MAX;
	int64_t lidx = UINT_MAX;
	int64_t slidx = UINT_MAX;

	Node** lowest_pair = (Node**) calloc(2, sizeof(Node*));
	if (!lowest_pair) {
		printf("failure initializing node array in get_min_two: %s\n", strerror(errno));
		exit(1);
	}
	for (uint64_t i = 0; i < max_idx; i++) {
		if (node_arr[i]->count < lowest) 	{
			lowest = node_arr[i]->count;
			lowest_pair[0] = node_arr[i];
			lidx = i;
		}
		else if (node_arr[i]->count < second_lowest) 	{
			second_lowest = node_arr[i]->count;
			lowest_pair[1] = node_arr[i];
			slidx = i;
		}
	}
	swap_idxs(node_arr, slidx, lidx, max_idx);
	return lowest_pair;
}

/* There is probably a more efficient way to construct this tree.
 * Come back to optimize this.
 */
Node* build_tree(uint64_t* freq_arr) {
	uint64_t max_idx = get_num_chars(freq_arr);

	Node* fin_node;
	Node** node_arr = init_node_arr_from_chars(freq_arr, max_idx);

	// when n == 1, return node in array
	while (max_idx > 1) {
		// Find two lowest value nodes in node arr, w/ len number giving max len, and their indicies
		printf("getting min nodes\n");
		Node** min_two = get_min_two(node_arr, max_idx);
		// Create node w/ the two lowest as children
		printf("creating new node\n");
		printf("node 1...\n");
		uint64_t c1 = min_two[0]->count;
		printf("node 2...\n");
		uint64_t c2 = min_two[1]->count;
		printf("can i even add bro\n");
		uint64_t count = c1 + c2; //min_two[0]->count + min_two[1]->count;
		Node* N = init_node(min_two[0], min_two[1], 0, count, 0);
		printf("inserting mins into tree\n");
		free(min_two);

		// remove the original two lowest value nodes, insert new node, decrease len number
		// we can remove the nodes since we can free the node mem in the tree, not in the arr
		printf("inserting into arr\n");
		node_arr[max_idx - 2] = N;
		printf("reduce max_idx\n");
		max_idx--;
		printf("fin node assignment\n");
		fin_node = N;
		printf("max_idx: %llu\n", max_idx);
	}
	printf("nadda\n");
	return fin_node;
}

// Recursive call to get tree depth
// use cnt = 0 at top level
unsigned int _tree_depth(Node* N, unsigned int cnt) {
	if (N == NULL) return cnt;
	return fmax(
			_tree_depth(N->l, cnt + 1),
			_tree_depth(N->r, cnt + 1));
}

unsigned int tree_depth(Node* N) {
	return _tree_depth(N, 0) - 1;
}

void _traverse(Node* N, CharCode* cur_cmprs, CharCode** write_table) {
	// add is_leaf back to node
	if (N->is_leaf) {
		cur_cmprs->token = N->token;
		write_table[N->token] = cur_cmprs;
		return;
	}
	CharCode* left_charcode = init_charcode(
			cur_cmprs->code << 1 | 0,
			cur_cmprs->fin_idx + 1,
			0);
	CharCode* right_charcode = init_charcode(
			cur_cmprs->code << 1 | 1,
			cur_cmprs->fin_idx + 1,
			0);
	_traverse(N->l, left_charcode, write_table);
	_traverse(N->r, right_charcode, write_table);
}

CharCode** traverse_tree(Node* N) {
	CharCode** md_arr = (CharCode**) calloc(TOKEN_SET_LEN, sizeof(CharCode*));
	if (!md_arr) {
		fprintf(stderr, "failed to allocate\n");
		exit(1);
	}
	CharCode* first_charcode = init_charcode(0, 0, 0);
	_traverse(N, first_charcode, md_arr);
	return md_arr;
}

void free_charcodes(CharCode** C) {
	for (int i = 0; i < TOKEN_SET_LEN; i++) {
		if (C[i])
			free(C[i]);
	}
}

// i love recursion
void free_tree(Node* N, int level) {
	if (N == NULL) return;

	// PRINT BLOCK
	for (int i = 0; i < level; i++)
		printf(" ");

	printf("%p: R: %d L: %d TKN: ", (void*)N, N->r == NULL, N->l == NULL);
	printle(N->token, 8);
	printf("\n");

	if ((N->r != NULL || N->l != NULL) && N->token != 0) {
		printf("ERROR ERROR ERROR in free_tree\n");
		fprintf(stderr, "fuck! how the hell did the node get a token \n");
		/* exit(1); */
	}
	// END PRINT BLOCK

	free_tree(N->r, level+1);
	free_tree(N->l, level+1);
	free((Node*)N);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "usage: ./huff <file>\n");
		exit(1);
	}

	FILE *infile;
	infile = fopen(argv[1], "r");
	if (!infile) {
		fprintf(stderr, "failed to open %s\n", argv[1]);
		exit(1);
	}

	printf("calculating char freqs...\n");
	uint64_t* freq_arr = calculate_char_freqs(infile);

	printf("building tree...\n");
	// Build Huffman Tree with Frequencies
	Node* tree = build_tree(freq_arr);

	print2DUtil(tree, 1);
	unsigned int D = tree_depth(tree);
	printf("treeeee depth: %d\n", D);
	printf("traversing tree\n");
	/* CharCode** C = traverse_tree(tree); */
	/* printf("done traversing tree\n"); */
	/* for (int i = 0; i < TOKEN_SET_LEN; i++) { */
	/* 	if (C[i])	{ */
	/* 		printf("TKN: "); */
	/* 		printle(C[i]->token, 8); */
	/* 		printf("\n"); */
	/* 		printf("Code: "); */
	/* 		printle(C[i]->code, C[i]->fin_idx); */
	/* 		printf(" \n\n"); */
	/* 	} */
	/* } */
	printf("Done Loop\n");

	printf("Freeing Tree\n");
	free_tree(tree, 0);
	printf("Freeing Charcodes\n");
	/* free_charcodes(C); */
	printf("Freeing Freq. Arr.\n");
	free(freq_arr);
	printf("Done\n");
	// Write file with Huffman Tree Symbols
	fclose(infile);
}
