#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


void Swap(tree_node_t **a, tree_node_t **b){
    tree_node_t *tmp = *a;
    *a = *b;
    *b = tmp;
}


void HeapifyUp(tree_node_t **heap, int ind){
    int parent = (ind - 1) / 2;
    if (ind && heap[parent]->frequency > heap[ind]->frequency){
        Swap(&heap[parent], &heap[ind]);
        HeapifyUp(heap, parent);
    }
}


void HeapifyDown(tree_node_t **heap, int size, int ind){
    int smallest = ind;
    int left = 2 * ind + 1;
    int right = 2 * ind + 2;

    if (left < size && heap[left]->frequency < heap[smallest]->frequency){
        smallest = left;
    }
    if (right < size && heap[right]->frequency < heap[smallest]->frequency){
        smallest = right;
    }

    if (smallest != ind){
        Swap(&heap[ind], &heap[smallest]);
        HeapifyDown(heap, size, smallest);
    }
}


tree_node_t* ExtractMinNode(tree_node_t **heap, int size){
    tree_node_t *minNode = heap[0];
    heap[0] = heap[--size];
    HeapifyDown(heap, size, 0);
    return minNode;
}

tree_node_t* BuildHuffmanTree(int symbol_count, int *frequency){
    tree_node_t **heap = malloc(symbol_count * sizeof(tree_node_t*));
    int size = 0;

    for (int i = 0; i < symbol_count; i++){
        if (frequency[i]){
            // new tree from symbol
            tree_node_t *node = malloc(sizeof(tree_node_t));
            node->symbol = i;
            node->frequency = frequency[i];
            node->left = NULL;
            node->right = NULL;
            heap[size++] = node;
        }
    }
    // building min heap
    for (int i = (size - 2) / 2; i >= 0; i--){
        HeapifyDown(heap, size, i);
    }

    // building huffman tree
    while (size > 1){
        // extract 2 nodes with min frequency
        tree_node_t *a = ExtractMinNode(heap, size);
        size--;
        tree_node_t *b = ExtractMinNode(heap, size);
        size--;
        // tree_node_t *a = heap[0];
        // heap[0] = heap[--size];
        // HeapifyDown(heap, size, 0);
        // tree_node_t *b = heap[0];
        // heap[0] = heap[--size];
        // HeapifyDown(heap, size, 0);

        // merging nodes into a new parent node
        tree_node_t *parent = malloc(sizeof(tree_node_t));

        // not a leaf
        parent->symbol = -1;
        parent->frequency = a->frequency + b->frequency;
        parent->left = a;
        parent->right = b;

        // add parent to the heap
        heap[size++] = parent;

        // heapify up to save heap properties
        HeapifyUp(heap, size - 1);
    }

    tree_node_t *root = heap[0];
    free(heap);
    return root;
}


void BuildCodes(tree_node_t *node, huffman_code_t *codes, char *code, int depth){
    if (!node->left && !node->right){
        code[depth] = '\0';
        strcpy(codes[node->symbol].code, code);
        codes[node->symbol].length = depth;
        return;
    }

    if (node->left){
        code[depth] = '0';
        BuildCodes(node->left, codes, code, depth + 1);
    }

    if (node->right){
        code[depth] = '1';
        BuildCodes(node->right, codes, code, depth + 1);
    }
}


void CompressTree(FILE *input, FILE *output, huffman_code_t *codes, int symbol_size, int *frequency){
    int symbol_range = symbol_size == 8 ? 256 : 65536;

    // ======== WRITE HEADER INFORMATION ========

    // write symbol size
    fwrite(&symbol_size, sizeof(int), 1, output);

    // write symbols count
    int count = 0;
    for (int i = 0; i < symbol_range; i++){
        if (frequency[i]){
            count++;
        }
    }
    fwrite(&count, sizeof(int), 1, output);
    
    // write symbol-frequency pairs
    for (int i = 0; i < symbol_range; i++){
        if (frequency[i]){
            fwrite(&i, sizeof(int), 1, output);
            fwrite(&frequency[i], sizeof(int), 1, output);
        }
    }


    // ======== CALC TOTAL BIT COUNT ========

    rewind(input); // reset file position
    long bit_count = 0;

    // for 8-bit symbols
    if (symbol_size == 8){
        int c;
        while ((c = fgetc(input)) != EOF){
            bit_count += codes[c].length;
        }
    // for 16-bit symbols
    } else{
        uint16_t symbol;
        while (fread(&symbol, 2, 1, input)){
            bit_count += codes[symbol].length;
        }
    }
    // write total bit count
    fwrite(&bit_count, sizeof(long), 1, output);


    // ======== COMPRESSION ========

    rewind(input);
    int bit_pos = 0; // current bit position in buffer
    unsigned char buffer = 0; // bit buffer

    // for 8-bit symbols
    if (symbol_size == 8){
        int c;
        while ((c = fgetc(input)) != EOF){
            for (int i = 0; codes[c].code[i]; i++){
                buffer <<= 1; // free space
                if (codes[c].code[i] == '1'){
                    buffer |= 1; // set bit 
                }
                // if buffer full then write to file
                if (++bit_pos == 8){
                    fputc(buffer, output);
                    bit_pos = 0;
                    buffer = 0;
                }
            }
        } 
    // for 16-bit symbols
    } else{
        uint16_t symbol;
        while (fread(&symbol, 2, 1, input)){
            for (int i = 0; codes[symbol].code[i]; i++){
                buffer <<= 1;
                if (codes[symbol].code[i] == '1'){
                    buffer |= 1;
                }
                if (++bit_pos == 8){
                    fputc(buffer, output);
                    bit_pos = 0;
                    buffer = 0;
                }
            }
        }
        
    }

    // write remaining bits
    if (bit_pos > 0){
        buffer <<= (8 - bit_pos);
        fputc(buffer, output);
    }
}


void FreeHuffmanTree(tree_node_t *tree){
    if (!tree){
        return;
    }
    FreeHuffmanTree(tree->left);
    FreeHuffmanTree(tree->right);
    free(tree);
}


void FreeHuffmanCodes(huffman_code_t *codes, int symbol_count){
    free(codes);
}


void DecompressTree(FILE *input, FILE *output){
    int symbol_size;
    fread(&symbol_size, sizeof(int), 1, input);
    int symbol_range = symbol_size == 8 ? 256 : 65536;

    int count;
    fread(&count, sizeof(int), 1, input);

    int *frequency = calloc(symbol_range, sizeof(int));
    for (int i = 0; i < count; i++){
        int s, f;
        fread(&s, sizeof(int), 1, input);
        fread(&f, sizeof(int), 1, input);
        frequency[s] = f;
    }

    long bit_count;
    fread(&bit_count, sizeof(long), 1, input);


    tree_node_t *tree = BuildHuffmanTree(symbol_range, frequency);
    tree_node_t *node = tree; // = root
    int byte;
    long read_bit = 0;
    while ((byte = fgetc(input)) != EOF && read_bit < bit_count){
        for (int i = 7; i >= 0 && read_bit < bit_count; i--, read_bit++){
            int bit = (byte >> i) & 1;
            node = bit ? node->right : node->left; // if 1 then right, 0 - left
            // if leaf
            if (!node->left && !node->right){
                if (symbol_size == 8){
                    fputc((uint8_t)node->symbol, output);
                } else{
                    uint16_t symbol = (uint16_t)node->symbol;
                    fwrite(&symbol, 2, 1, output);
                }
                node = tree; // reset the current node pointer back to the root for the next symbol (prefix codes)
            }
        }
    }

    FreeHuffmanTree(tree);
    free(frequency);
}


