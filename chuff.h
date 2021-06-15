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

void printbe(uint64_t v, uint64_t max_idx) {
	for (uint64_t j = 0; j < max_idx; j++) {
		printf("%llu", v & 1);
		v = v >> 1;
	}
}

void printle(uint64_t v, uint64_t max_idx) {
	for (uint64_t j = max_idx; j > 0; j--) {
		uint64_t shift = 1 << j;
		printf("%d", (bool)((v & shift) == shift));
	}
}
