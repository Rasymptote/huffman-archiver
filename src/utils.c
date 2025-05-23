#include "utils.h"
#include "huffman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>


int IsDir(char *path){
    struct stat st;
    // fill st with information about the file. if not 0 then error (dir does not exist)
    if (stat(path, &st) != 0){
        return 0;
    }
    return S_ISDIR(st.st_mode); // st_mode - type of file
}


long GetFileSize(char *path){
    FILE *file = fopen(path, "rb");
    if (!file){
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    return size;
}


void CompressFile(char *path, int symbol_size){
    int ouput_len = strlen(path) + 6;
    char *output_path = malloc(strlen(path) + 6); // ".huff" + '\0'
    snprintf(output_path, ouput_len, "%s.huff", path);
    CompressFileTo(path, output_path, symbol_size);
    free(output_path);
}


void CompressFileTo(char *input_path, char *output_path, int symbol_size){
    FILE *input = fopen(input_path, "rb");
    if (!input){
        fprintf(stderr, "Failed to open input file.\n");
        return;
    }

    FILE *output = fopen(output_path, "wb");
    if (!output){
        fprintf(stderr, "Failed to open output file.\n");
        fclose(input);
        free(output_path);
        return;
    }

    // count symbol frequencies
    int symbol_count = symbol_size == 8 ? 256 : 65536;
    int *frequency = calloc(symbol_count, sizeof(int));
    if (symbol_size == 8){
        int c;
        while((c = fgetc(input)) != EOF){
            frequency[c]++;
        } 
    } else {
        uint16_t symbol;
        while((fread(&symbol, 2, 1, input))){
            frequency[symbol]++;
        }
    }

    rewind(input);

    // build huffman tree and codes
    tree_node_t *tree = BuildHuffmanTree(symbol_count, frequency);
    huffman_code_t *codes = calloc(symbol_count, sizeof(huffman_code_t));

    char code[256]; // temporary buffer for codes
    BuildCodes(tree, codes, code, 0);

    // perform copression
    printf("Compressing %s -> %s\n", input_path, output_path);
    CompressTree(input, output, codes, symbol_size, frequency);

    // get file sizes
    long input_size = GetFileSize(input_path);
    fclose(input);
    fclose(output);
    long output_size = GetFileSize(output_path);
    printf("Input size: %ld bytes\n", input_size);
    printf("Compressed size: %ld bytes\n", output_size);
    printf("Compression ratio: %.2f%%\n\n", 100.0 * output_size / input_size);
   
    FreeHuffmanTree(tree);
    FreeHuffmanCodes(codes, symbol_count);
    free(frequency);
}


void DecompressFile(char *path){
    int output_len = strlen(path) + 6;
    char *output_path = malloc(output_len); // ".huff" + '\0'
    strncpy(output_path, path, output_len);

    // remove ".huff"
    char *dot = strrchr(output_path, '.'); // find the last occurrence of a char
    if (dot && strcmp(dot, ".huff") == 0){
        *dot = '\0';
    }

    DecompressFileTo(path, output_path);
    free(output_path);
}


void DecompressFileTo(char *input_path, char *output_path){
    FILE *input = fopen(input_path, "rb");
    if (!input){
        fprintf(stderr, "Failed to open compressed file.\n");
        return;
    }

    FILE *output = fopen(output_path, "wb");
    if (!output){
        fprintf(stderr, "Failed to open output file.\n");
        fclose(input);
        free(output_path);
        return;
    }

    printf("Decompressing %s -> %s\n", input_path, output_path);
    DecompressTree(input, output);
    fclose(input);
    fclose(output);
    
}


void CompressFilesToArchive(char **files, int file_count, char *archive_name, int symbol_size){
    FILE *archive = fopen(archive_name, "wb");
    if (!archive){
        fprintf(stderr, "Failed to open archive.\n");
        return;
    }
    
    // write total number of files to archive header
    fwrite(&file_count, sizeof(int), 1, archive);

    file_index_t *index = calloc(file_count, sizeof(file_index_t));
    // remember current position
    long index_pos = ftell(archive);
    // skip space for index 
    fseek(archive, file_count * sizeof(file_index_t), SEEK_CUR);

    for (int i = 0; i < file_count; i++){
        FILE *input = fopen(files[i], "rb");
        if (!input){
            continue;
        }

        int symbol_range = symbol_size == 8 ? 256 : 65536;
        int *frequency = calloc(symbol_range, sizeof(int));
        if (symbol_size == 8){
            int c;
            while ((c = fgetc(input)) != EOF){
                frequency[c]++;
            }
        } else {
            uint16_t symbol;
            while ((fread(&symbol, 2, 1, input))){
                frequency[symbol]++;
            }
        }
        
        rewind(input);
        tree_node_t *tree = BuildHuffmanTree(symbol_range, frequency);
        huffman_code_t *codes = calloc(symbol_range, sizeof(huffman_code_t));
        char code[256];
        BuildCodes(tree, codes, code, 0);

        // get start position before compression
        long start = ftell(archive);
        CompressTree(input, archive, codes, symbol_size, frequency);
        // get end position after compression
        long end = ftell(archive);

        // store file metadata
        strncpy(index[i].filename, files[i], 255);
        index[i].position = start;
        index[i].length = end - start;

        printf("Compressed: %s\n", index[i].filename);
        fclose(input);
        free(frequency);
        free(codes);
        FreeHuffmanTree(tree);
    }
    // return to write index at reserved position
    long end_pos = ftell(archive);
    fseek(archive, index_pos, SEEK_SET);
    fwrite(index, sizeof(file_index_t), file_count, archive);

    fseek(archive, end_pos, SEEK_SET);

    free(index);
    fclose(archive);
}


void DecompressArchive(char *archive_name){
    FILE *archive = fopen(archive_name, "rb");
    if (!archive){
        fprintf(stderr, "Failed to open archive.\n");
        return;
    }

    int file_count;
    fread(&file_count, sizeof(int), 1, archive);
    file_index_t *index = calloc(file_count, sizeof(file_index_t));
    fread(index, sizeof(file_index_t), file_count, archive);

    for (int i = 0; i < file_count; i++){
        fseek(archive, index[i].position, SEEK_SET);
        FILE *output = fopen(index[i].filename, "wb");
        if (!output){
            fprintf(stderr, "Failed to write: %s\n", index[i].filename);
            continue;
        }
        DecompressTree(archive, output);
        fclose(output);
        printf("Extracted: %s\n", index[i].filename);
    }
    free(index);
    fclose(archive);
}


void CompressDir(char *path, int symbol_size){
    DIR *dir = opendir(path);
    if (!dir){
        fprintf(stderr, "Cannot open directory.\n");
        return;
    }

    // create archive dir
    char archive_path[512];
    snprintf(archive_path, sizeof(archive_path), "%s.huff", path);
    mkdir(archive_path, 0755);

    // reading files in dir
    struct dirent *file;
    while((file = readdir(dir))){
        // full path to file
        char input_path[512];
        snprintf(input_path, sizeof(input_path), "%s/%s", path, file->d_name);

        // if element does not exist or element not a file
        struct stat st;
        if (stat(input_path, &st) != 0 || !S_ISREG(st.st_mode)){
            continue; // skip 
        }
        // full path to compressed file
        char output_path[1024];
        snprintf(output_path, sizeof(output_path), "%s/%s.huff", archive_path, file->d_name);
        CompressFileTo(input_path, output_path, symbol_size);
    }
    closedir(dir);  
}


void DecompressDir(char *path){
    DIR *dir = opendir(path);
    if (!dir){
        fprintf(stderr, "Cannot open directory.\n");
        return;
    }

    // path of output dir
    char archive_path[512];
    strncpy(archive_path, path, sizeof(archive_path));

    // remove ".huff"
    char *dot = strrchr(archive_path, '.'); // find the last occurrence of a char
    if (dot && strcmp(dot, ".huff") == 0){
        *dot = '\0';
    }
    // create output dir
    mkdir(archive_path, 0755);

    // reading files in dir
    struct dirent *file;
    while((file = readdir(dir))){
        // build full path to file
        char input_path[512];
        snprintf(input_path, sizeof(input_path), "%s/%s", path, file->d_name);

        // if element does not exist or element not a file
        struct stat st;
        if (stat(input_path, &st) != 0 || !S_ISREG(st.st_mode)){
            continue; // skip
        }

        if (!strstr(file->d_name, ".huff")){
            continue; // skip without ".huff" ext
        }

        // output filename (removing ".huff")
        char output_file_name[512];
        strncpy(output_file_name, file->d_name, sizeof(output_file_name));
        char *dot = strrchr(output_file_name, '.');
        if (dot && strcmp(dot, ".huff") == 0){
            *dot = '\0';
        }

        // build full output path
        char output_path[1024];
        snprintf(output_path, sizeof(output_path), "%s/%s", archive_path, output_file_name);
        DecompressFileTo(input_path, output_path);
    }

    closedir(dir);  
}

