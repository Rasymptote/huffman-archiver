#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>

// maximum number of symbols (256 for 8 bit, 65536 for 16 bit)
#define SYMBOLS_MAX_NUM 65536


typedef struct tree_node_t {
    int symbol;
    double frequency;
    struct tree_node_t *left;
    struct tree_node_t *right;
} tree_node_t;


typedef struct huffman_code_t {
    char code[256];
    int length;
} huffman_code_t;


tree_node_t* BuildHuffmanTree(int symbol_count, int *frequency);
void BuildCodes(tree_node_t *node, huffman_code_t *codes, char *code, int depth);
void CompressTree(FILE *input, FILE *output, huffman_code_t *codes, int symbol_size, int *frequency);
void DecompressTree(FILE *input, FILE *output);
void FreeHuffmanTree(tree_node_t *tree);
void FreeHuffmanCodes(huffman_code_t *codes, int symbol_count);

#endif

