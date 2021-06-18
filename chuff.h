typedef _Bool bool;

typedef struct Node {
	struct Node * l, * r;
	uint64_t count;
	uint8_t token;
	bool is_leaf;
} Node;

typedef struct CharCode {
	uint64_t code;
	uint64_t fin_idx;
	uint8_t token;
} CharCode;


void printle(uint64_t v, uint64_t max_idx) {
	for (uint64_t j = max_idx; j > 0; j--) {
		uint64_t shift = 1 << (j - 1);
		printf("%d", (bool)((v & shift) == shift));
	}
}

void print_padding (int n) {
	for (int i = 0; i < n; i++) putchar('\t');
}

void print2DUtil(Node* root, int space) {
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

	printle(root->token, 8);
	printf(": %llu\n", root->count);

	// Process left child
	print2DUtil(root->l, space);
}

void printbe(uint64_t v, uint64_t max_idx) {
	for (uint64_t j = 0; j < max_idx; j++) {
		printf("%llu", v & 1);
		v = v >> 1;
	}
}

