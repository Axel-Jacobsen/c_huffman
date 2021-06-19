#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "chuff.h"

#define TOKEN_LEN 8 // right now only a token len of 8 bits
#define TOKEN_SET_LEN (1 << TOKEN_LEN)
#define WRITE_CHUNK_SIZE 80000

/* todos:
 * - multithread in calc_char_freqs
 * - multithread in file writing?
 */
uint16_t num_chars = 0;


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

uint8_t get_num_chars(uint64_t* freq_arr) {
	uint8_t num_chars = 0;
	for (uint8_t i = 0; i < TOKEN_SET_LEN - 1; i++)
		if (freq_arr[i] != 0)
			++num_chars;
	return num_chars;
}

Node* init_node(Node* n1, Node* n2, uint8_t tkn, uint64_t cnt, bool is_leaf) {
	Node* N = (Node*) malloc(sizeof(Node));
	if (!N) {
		printf("failure initializing Node: %s\n", strerror(errno));
		exit(1);
	}
	N->l = n1;
	N->r = n2;
	N->token = tkn;
	N->count = cnt;
	N->is_leaf = is_leaf;
	return (Node*) N;
}

CharCode* init_charcode(uint64_t char_code, uint64_t code_len,	uint8_t token) {
	CharCode* C = (CharCode*) malloc(sizeof(CharCode));
	if (!C) {
		printf("failure initializing CharCode: %s\n", strerror(errno));
		exit(1);
	}
	C->code = char_code;
	C->code_len = code_len;
	C->token = token;
	return C;
}

Node** init_node_arr_from_chars(uint64_t* freq_arr, uint8_t num_chars) {
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

void swap_idxs(Node** node_arr, uint64_t lidx, uint64_t slidx, uint64_t max_idx) {
	// swap 1
	Node* ldx_tmp = node_arr[slidx];
	node_arr[lidx] = node_arr[max_idx - 1];
	node_arr[max_idx - 1] = ldx_tmp;
	// swap 2
	if (slidx == max_idx - 1) // slidx has been swapped by code above, so now in lidx
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
	uint8_t max_idx = get_num_chars(freq_arr);
	num_chars = max_idx;

	Node* fin_node;
	Node** node_arr = init_node_arr_from_chars(freq_arr, max_idx);

	// when n == 1, return node in array
	while (max_idx > 1) {
		// Find two lowest value nodes in node arr, w/ len number giving max len, and their indicies
		Node** min_two = get_min_two(node_arr, max_idx);
		// Create node w/ the two lowest as children
		uint64_t count = min_two[0]->count + min_two[1]->count;

		Node* N = init_node(min_two[0], min_two[1], 0, count, 0);
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
			cur_cmprs->code | (uint64_t)0 << (63 - cur_cmprs->code_len),
			cur_cmprs->code_len + 1,
			0);
	CharCode* right_charcode = init_charcode(
			cur_cmprs->code | (uint64_t)1 << (63 - cur_cmprs->code_len),
			cur_cmprs->code_len + 1,
			0);
	free(cur_cmprs);
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


void write_charcode(FILE* outfile, CharCode* c) {
	/* write to file <1:char><1:num bits in tree code><x:tree code>
	 * to current position in outfile
	 */
	fwrite(&c->token, 1, 1, outfile);
	fwrite(&c->code_len, 1, 1, outfile);
	uint8_t num_code_bytes = (c->code_len - 1) / 8 + 1;
	fwrite(&c->code, num_code_bytes, 1, outfile);
}


void write_to_file(FILE* infile, FILE* outfile, CharCode** write_table) {
	// MSB of byte is the first instruction (i.e. left/right instruction)
	fseek(infile, 0L, SEEK_END);
	uint64_t flen = ftell(infile);
	fseek(infile, 0L, SEEK_SET);

	// Write to the file in chunks of 8 kb (8 bytes per u64, 1024 of)
	// write_chunk is the chunk of mem that gets written each time.
	// chunk_idx is the index of the write_chunk array that is being written to.
	// int_idx is the index of the uint64_t (given by write_chunk[chunk_idx]) that hasn't
	// been written to yet.
	uint64_t* write_chunk = (uint64_t*)calloc(WRITE_CHUNK_SIZE, sizeof(uint64_t));
	unsigned int chunk_idx = 0;
	int8_t int_idx = 0;

	// Start by loading a char from the infile
	uint8_t c = 0;
	uint64_t infile_pos = 1;
	fread((void*)&c, 1, 1, infile);
	CharCode* charcode = write_table[c];

	// we only need the code and code_len, and seperating
	// these from the CharCode object allows generalization in writing
	// accross write_chunk elements. I.e., each time we add to a write_chunk,
	// we add the most significant code_len bits of code.
	uint64_t code = charcode->code;
	uint64_t code_len = charcode->code_len;

	uint8_t tail_padding_zeros;

	for (;;) {
		printf("%llu / %llu     code_len: %llu chunk_idx: %u int_idx: %u\n", infile_pos, flen, code_len, chunk_idx, int_idx);
		write_chunk[chunk_idx] |= code >> int_idx;
		int_idx += code_len;

		if (int_idx >= 63) {
			// overflow on charcode, increment chunk_idx and prepare
			// chunk_idx/code/code_len/int_idx for next write
			chunk_idx++;
			code = code << (code_len - int_idx - 63);
			code_len = int_idx - 63;
			int_idx = 0;
		} else {// load another char
			if (infile_pos == flen) {
				// if we are out of chars and here, we write and are finished!
				fwrite(write_chunk, sizeof(uint64_t), chunk_idx, outfile);
				tail_padding_zeros = 64 - int_idx; // not actually zeros, but just junk
				break;
			}
			// load a char here
			fread((void*)&c, 1, 1, infile);
			infile_pos++;
			charcode = write_table[c];
			code = charcode->code;
			code_len = charcode->code_len;
		}
		if (chunk_idx == WRITE_CHUNK_SIZE) { // this chunk is full
			// write and reset write_chunk, set chunk_idx to 0
			chunk_idx = 0;
			fwrite(write_chunk, sizeof(uint64_t), WRITE_CHUNK_SIZE, outfile);
		}
	}
	free(write_chunk);
	fseek(infile, 0L, SEEK_SET);
	fseek(outfile, 0L, SEEK_SET);

	// write header - tells decoder how to read file
	// consists of
	//	<1 byte: number of bits padding end of file>
	//	<1 byte: N = number of unique symbols in compressed file>
	//	<N bytes: <SYMBOL>> where
	//		SYMBOL = <symbol (1 byte) : \
	//						  # bits of code in tree (1 byte) : \
	//						  code (# of bits, plus padding to make it bytes)>
	fwrite(&tail_padding_zeros, sizeof(uint8_t), 1, outfile);
	fwrite(&num_chars, sizeof(uint8_t), 1, outfile);
	for (int i = 0; i < 256; i++) {
		if (write_table[i])
			write_charcode(outfile, write_table[i]);
	}
}

void free_charcodes(CharCode** C) {
	for (int i = 0; i < TOKEN_SET_LEN; i++) {
		if (C[i]) free(C[i]);
	}
}

// i love recursion
void free_tree(Node* N, int level) {
	if (N == NULL) return;
	free_tree(N->r, level+1);
	free_tree(N->l, level+1);
	free(N);
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

	FILE *outfile;
	char* outfile_name = strcat(argv[1], ".pine");
	outfile = fopen(outfile_name, "w");
	if (!outfile) {
		fprintf(stderr, "failed to open %s\n", outfile_name);
		exit(1);
	}

	/* printf("calculating char freqs...\n"); */
	uint64_t* freq_arr = calculate_char_freqs(infile);

	/* printf("building tree...\n"); */
	// Build Huffman Tree with Frequencies
	Node* tree = build_tree(freq_arr);

	CharCode** C = traverse_tree(tree);

	write_to_file(infile, outfile, C);

	free_tree(tree, 0);
	free_charcodes(C);
	free(freq_arr);

	// Write file with Huffman Tree Symbols
	fclose(infile);
}
