#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

#define TOKEN_LEN 8 // right now only a token len of 8 bits
#define TOKEN_SET_LEN 1 << TOKEN_LEN

typedef _Bool bool;

typedef struct {
  struct Node * l, * r;
  unsigned char token;
  unsigned int count;
  bool is_leaf;
  bool in_tree;
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
  // Unsorted, so just linear search
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

uint64_t get_num_chars(uint64_t* freq_arr)
{
  uint64_t num_chars = 0;
  for (uint64_t i = 0; i < TOKEN_SET_LEN; ++i)
    if (freq_arr[i] != 0)
      ++num_chars;
  return num_chars;
}

Node* create_node_arr_from_chars(uint64_t* freq_arr, uint64_t num_chars)
{
  Node* node_arr = (Node*) calloc(num_chars, sizeof(Node*));
  uint64_t j = 0;
  for (uint64_t i = 0; i < TOKEN_SET_LEN; i++) {
    if (freq_arr[i] != 0) {
      node_arr[j] = (Node) {
        .l = NULL,
        .r = NULL,
        .token = i,
        .count = freq_arr[i],
        .is_leaf = 1
      };
      ++j;
    }
  }

  return node_arr;
}

Node** get_min_two(Node* node_arr, uint64_t max_idx)
{
  uint64_t lowest = UINT_MAX;
  uint64_t second_lowest = UINT_MAX;
  Node** lowest_pair = (Node**) calloc(2, sizeof(Node*));

  for (unsigned int i = 0; i < max_idx; i++)
  {
	if (node_arr[i].count < lowest)
	{
		lowest = node_arr[i].count;
		lowest_pair[0] = &node_arr[i];
	}
	else if (node_arr[i].count < lowest)
	{
		second_lowest = node_arr[i].count;
		lowest_pair[1] = &node_arr[i];
	}
  }
  return lowest_pair;
}

Node* build_tree(FILE* f, uint64_t* freq_arr)
{
  uint64_t num_chars = get_num_chars(freq_arr);
  Node* nodes = create_node_arr_from_chars(freq_arr, num_chars);

  while (num_chars > 1) {
	  // Find two lowest value nodes in node arr, w/ len number giving max len, and their indicies
	  Node** min_two = get_min_two(nodes, num_chars);
	  // Create node w/ the two lowest as children
	  Node N = (Node) {
		  .l = min_two[0],
		  .r = min_two[1],
		  .token = NULL,
		  .count = min_two[0]->count + min_two[1]->count,
		  .is_leaf = 0,
		  .in_tree = 1
	  };
	  free(min_two);
	  // remove the original two lowest value nodes, insert new node, decrease len number
	  // when n == 1, return node in array
  }

  return nodes;
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

  // Write file with Huffman Tree Symbols
  free(cf_arr); cf_arr = NULL;
  fclose(infile);
}
