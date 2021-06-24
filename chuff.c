#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "chuff.h"

#define TOKEN_LEN 8 // right now only a token len of 8 bits
#define TOKEN_SET_LEN ((uint8_t)1 << TOKEN_LEN)
#define WRITE_CHUNK_SIZE 8000
#define NUM_BYTES(bits) ((bits - 1) / 8 + 1)

/* errors:
 * - when WRITE_CHUNK_SIZE > size of file, random letters scrambled
 * - when WRITE_CHUNK_SIZE < size of file, completely fubar
 */

/* improvements:
 * - multithread in calc_char_freqs
 * - multithread in file writing?
 * - safe calloc/malloc
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

uint16_t get_num_chars(uint64_t* freq_arr) {
	uint16_t num_chars = 0;
	for (int i = 0; i < TOKEN_SET_LEN; i++) {
		printf("i = %d freq_arr[i] = %llu\n", i, freq_arr[i]);
		if (freq_arr[i] != 0)
			++num_chars;
	}
	return num_chars;
}

Node* init_node(Node* n1, Node* n2, uint8_t tkn, uint64_t cnt, bool is_leaf) {
	Node* N = (Node*) malloc(sizeof(Node));
	if (!N) {
		fprintf(stderr, "failure initializing Node: %s\n", strerror(errno));
		exit(1);
	}
	N->l = n1;
	N->r = n2;
	N->token = tkn;
	N->count = cnt;
	N->is_leaf = is_leaf;
	return (Node*) N;
}

CharCode* init_charcode(uint64_t code, uint8_t code_len,	uint8_t token) {
	CharCode* C = (CharCode*) malloc(sizeof(CharCode));
	if (!C) {
		fprintf(stderr, "failure initializing CharCode: %s\n", strerror(errno));
		exit(1);
	}
	C->code = code;
	C->code_len = code_len;
	C->token = token;
	return C;
}

Node** init_node_arr_from_chars(uint64_t* freq_arr, uint16_t num_chars) {
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
		fprintf(stderr, "failure initializing node array in get_min_two: %s\n", strerror(errno));
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
	uint16_t max_idx = get_num_chars(freq_arr);
	printf("BUILD TREE\n");
	printf("	max_idx = %d\n", max_idx);
	num_chars = max_idx;

	Node* fin_node;
	Node** node_arr = init_node_arr_from_chars(freq_arr, max_idx);

	// when n == 1, return node in array
	while (max_idx > 1) {
		printf("	max_idx = %d\n", max_idx);
		// Find two lowest value nodes in node arr, w/ len number giving max len, and their indicies
		printf("min two\n");
		Node** min_two = get_min_two(node_arr, max_idx);
		// Create node w/ the two lowest as children
		uint64_t count = min_two[0]->count + min_two[1]->count;

		printf("init node\n");
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

/* given the main node N, the token (i.e. symbol), and
 * the code which defines the new node's position in
 * the tree.
 */
void reconstruct_tree(Node* N, uint8_t token, uint8_t code_len, uint64_t code) {
	Node* cur_node = N;
	for (uint8_t i = 64; i > 64 - code_len; i--) {
		bool is_leaf = i == (64 - code_len + 1);
		uint64_t shift = (uint64_t) 1 << (i - 1);
		if ((code & shift) == shift) {
			if (cur_node->r == NULL)
				cur_node->r = init_node(NULL, NULL, token, 0, is_leaf);
			cur_node = cur_node->r;
		} else {
			if (cur_node->l == NULL)
				cur_node->l = init_node(NULL, NULL, token, 0, is_leaf);
			cur_node = cur_node->l;
		}
	}
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
	if (N->is_leaf) {
		/* printf("TRAVERSE TOKEN "); printle_byte(N->token); printf("\n"); */
		cur_cmprs->token = N->token;
		write_table[N->token] = cur_cmprs;
		return;
	}
	CharCode* left_charcode = init_charcode(
			cur_cmprs->code | ((uint64_t)0 << (63 - cur_cmprs->code_len)),
			cur_cmprs->code_len + 1,
			0);
	CharCode* right_charcode = init_charcode(
			cur_cmprs->code | ((uint64_t)1 << (63 - cur_cmprs->code_len)),
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
	printf("	init charcode\n");
	CharCode* first_charcode = init_charcode(0, 0, 0);
	printf("	starting traverse...\n");
	_traverse(N, first_charcode, md_arr);
	return md_arr;
}

void free_charcodes(CharCode** C) {
	for (int i = 0; i < TOKEN_SET_LEN; i++) {
		if (C[i]) free(C[i]);
	}
}

// i love recursion
void free_tree(Node* N) {
	if (N == NULL) return;
	free_tree(N->r);
	free_tree(N->l);
	free(N);
}

void write_charcode(FILE* outfile, CharCode* c) {
	/* write to file <1:char><1:num bits in tree code><x:tree code>
	 * to current position in outfile
	 */
	fwrite(&c->token, 1, 1, outfile);
	fwrite(&c->code_len, 1, 1, outfile);
	uint8_t num_code_bytes = NUM_BYTES(c->code_len);
	uint64_t code = c->code >> (64 - c->code_len);
	fwrite(&code, num_code_bytes, 1, outfile);
}

void encode(FILE* infile, FILE* outfile, CharCode** write_table) {
	// FILE FORMAT
	//	HEADER - tells decoder how to read file
	//		consists of
	//		<1 byte: N = number of unique symbols in compressed file>
	//		<N bytes: <SYMBOL>> where
	//				SYMBOL = <symbol (1 byte) : \
	//						  # depth of symbol in tree (1 byte) : \
	//						  code (# of bits, plus padding to make it bytes)>
	//	CONTENTS - encoded symbols in file
	//	TAIL
	//		<1 byte: number of bits padding end of file>
	// MSB of byte is the first instruction (i.e. left/right instruction)
	fseek(infile, 0L, SEEK_END);
	uint64_t flen = ftell(infile);
	fseek(infile, 0L, SEEK_SET);

	// Write to the file in chunks of 8 kb (8 bytes per u64, 1024 of)
	// write_chunk is the chunk of mem that gets written each time.
	// chunk_idx is the index of the write_chunk array that is being written to.
	// int_idx is the index of the uint64_t (given by write_chunk[chunk_idx]) that hasn't
	//   been written to yet.
	uint64_t* write_chunk = (uint64_t*)calloc(WRITE_CHUNK_SIZE, sizeof(uint64_t));
	uint64_t chunk_idx = 0;
	int8_t int_idx = 0;

	// start of file writing
	// first, write the number of chars
	fwrite(&num_chars, sizeof(uint16_t), 1, outfile);

	// we only need the code and code_len, and seperating
	// these from the CharCode object allows generalization in writing
	// accross write_chunk elements. I.e., each time we add to a write_chunk,
	// we add the most significant code_len bits of code.
	printf("  writing charcodes...\n");
	for (int i = 0; i < 256; i++)
		if (write_table[i])
			write_charcode(outfile, write_table[i]);

	// Start by loading a char from the infile
	uint8_t c = 0;
	uint64_t infile_pos = 1;
	fread(&c, 1, 1, infile);

	uint8_t tail_padding_zeros = 0;
	uint64_t code = write_table[c]->code;
	uint64_t code_len = write_table[c]->code_len;
	printf("  writing file...\n");
	for (;;) {
		write_chunk[chunk_idx] |= code >> int_idx;
		int_idx += code_len;

		if (int_idx >= 63) {
			// overflow on charcode, increment chunk_idx and prepare
			// chunk_idx/code/code_len/int_idx for next write
			printf("  int_idx overflow...\n");
			code = code << (code_len - int_idx - 64);
			code_len = int_idx - 64;
			chunk_idx++;
			int_idx = 0;
		} else {// load another char
			if (infile_pos == flen) {
				printf("  finishing write\n");
				// if we are out of chars and here, we write and are finished!
				// set bytes to big endian order
				for (int i = 0; i < chunk_idx+1; i++) {
					write_chunk[i] = htonll(write_chunk[i]);
				}
				// doing simple math to reduce number of redundant bits to less than 8
				uint8_t final_u64_num_junk_bits = 64 - int_idx;
				uint8_t full_junk_bytes = NUM_BYTES(final_u64_num_junk_bits) - 1; // we can right shift the final byte by this many bytes
				uint8_t num_bytes_to_write = 8 - full_junk_bytes;
				tail_padding_zeros = final_u64_num_junk_bits - 8 * full_junk_bytes;
				uint64_t tail_chunk = write_chunk[chunk_idx]; //>> (8 * full_junk_bytes);

				fwrite(write_chunk, sizeof(uint64_t), chunk_idx, outfile);
				fwrite(&tail_chunk, 1, num_bytes_to_write, outfile);
				printf("  finished write\n");
				break;
			}
			printf("  loading char... ");
			// load a char here
			fread(&c, 1, 1, infile);
			infile_pos++;
			/* printle_byte(c); */
			/* printf("\n"); */
			/* printf("i am seg faulting on c?!?!\n"); */
			/* printf("write_table[%d] == NULL\n", c); */
			/* CharCode* r = write_table[c]; */
			/* printf("%d\n", r == NULL); */
			/* printf("hm\n"); */
			/* printf("write_table[%d] == NULL: %d\n", c, write_table[c] == NULL); */
			code = write_table[c]->code;
			code_len = write_table[c]->code_len;
			printf("  done loading char\n");
		}
		// write and reset write_chunk, set chunk_idx to 0
		if (chunk_idx == WRITE_CHUNK_SIZE) { // this chunk is full
			printf("  flipping over write chunk size\n");
			// set bytes to big endian order
			for (int i = 0; i < chunk_idx+1; i++) {
				/* write_chunk[i] = htonll(write_chunk[i]); */
			}
			fwrite(write_chunk, sizeof(uint64_t), WRITE_CHUNK_SIZE, outfile);
			chunk_idx = 0;
		}
	}
	fwrite(&tail_padding_zeros, sizeof(uint8_t), 1, outfile);
	free(write_chunk);
}

// FILE FORMAT
//	HEADER - tells decoder how to read file
//		consists of
//		<1 byte: N = number of unique symbols in compressed file>
//		<N bytes: <SYMBOL>> where
//				SYMBOL = <symbol (1 byte) : \
//						  # of bits of code in tree (1 byte) : \
//						  code (# of bits, plus padding to make it bytes)>
//	CONTENTS - encoded symbols in file
//	TAIL
//		<1 byte: number of bits padding end of file>
void decode(FILE* encoded_fh, FILE* decoded_fh) {
	// hop to end of file to get file len, padding zeros
	// offset of -1 so we can read the tail padding too.
	// once we are done, return it to start of file.
	uint8_t tail_padding_zeros = 0;
	fseek(encoded_fh, -1L, SEEK_END);
	uint64_t end_pos = ftell(encoded_fh) + 1;
	fread(&tail_padding_zeros, 1, 1, encoded_fh);
	fseek(encoded_fh, 0L, SEEK_SET);

	// read header
	uint16_t num_symbols;
	fread(&num_symbols, 2, 1, encoded_fh);

	// read symbols
	uint8_t token, code_len;
	uint64_t code;
	Node* root = init_node(NULL, NULL, 0, 0, 0);
	for (uint16_t i = 0; i < num_symbols; i++) {
		// symbol token
		fread(&token, 1, 1, encoded_fh);
		// # bits
		fread(&code_len, 1, 1, encoded_fh);
		// code
		uint8_t num_code_bytes = NUM_BYTES(code_len);

		code = 0;
		fread(&code, num_code_bytes, 1, encoded_fh);

		code <<= (64 - code_len);

		reconstruct_tree(root, token, code_len, code);
	}

	// read actual file data
	uint64_t cur_file_pos = ftell(encoded_fh);
	uint8_t byte;
	uint8_t byte_valid_bits;
	Node* N = root;
	for (; cur_file_pos < end_pos - 1; cur_file_pos++) {
		// read byte
		fread(&byte, 1, 1, encoded_fh);
		// for each bit in byte, move in tree
		byte_valid_bits = cur_file_pos == end_pos - 2 ? tail_padding_zeros : 0;
		for (int i = 7; i >= byte_valid_bits; i--) {
			uint8_t shift = 1 << i;
			if ((byte & shift) == shift)
				N = N->r;
			else
				N = N->l;

			if (N->is_leaf) {
				fwrite(&N->token, 1, 1, decoded_fh);
				N = root;
			}
		}
	}
	free_tree(root);
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

	printf("calculating char freqs...\n");
	uint64_t* freq_arr = calculate_char_freqs(infile);
	printf("freq_arr[0xff] = %llu\n", freq_arr[0xff]);

	printf("build tree...\n");
	Node* tree = build_tree(freq_arr);
	printf("%llu\n", tree->count);
	printf("print tree\n");
	print2DUtil(tree, 1);
	printf("print tree\n");
	printf("traverse tree...\n");
	CharCode** C = traverse_tree(tree);

	FILE* decoded;
	decoded = fopen("./out/out.txt", "w");
	if (!decoded) {
		fprintf(stderr, "failed to open decoded\n");
		exit(1);
	}
	
	printf("encode...\n");
	encode(infile, outfile, C);

	printf("free tree...\n");
	free_tree(tree);

	printf("free charcodes...\n");
	free_charcodes(C);

	printf("free freq arr...\n");
	free(freq_arr);

	// Write file with Huffman Tree Symbols
	fclose(outfile);
	outfile = fopen(outfile_name, "r");

	printf("decode...\n");
	decode(outfile, decoded);

	fclose(infile);
}
