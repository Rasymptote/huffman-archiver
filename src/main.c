#include "huffman.h"
#include "utils.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

// help
void PrintHelp(char *program_name){
    printf("Usage: %s [options] <input>\n", program_name);
    printf("Options:\n");
    printf(" -c, --compress         Compress input files/directory (default).\n");
    printf(" -d, --decompress       Decompress input files/direcrory.\n");
    printf(" -1, --8bit             Use 8-bit symbols (default).\n");
    printf(" -2, --16bit            Use 16-bit symbols.\n");
    printf(" -h, --help             Display that information.\n");
}

// compress/decompress mode
enum Mode{
    COMPRESS,
    DECOMPRESS
};


int main(int argc, char *argv[]){
    // no args
    if (argc < 2) {
        PrintHelp(argv[0]);
        return 1;
    }

    // default mode: compress
    enum Mode operation = COMPRESS;
    // default symbol size: 8 bit
    int symbol_size = 8;
    // for multi-file archive 
    char *output_name = NULL;

    // args parsing
    static struct option long_options[] = {
        {"compress", no_argument, 0, 'c'},
        {"decompress", no_argument, 0, 'd'},
        {"8bit", no_argument, 0, '1'},
        {"16bit", no_argument, 0, '2'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    }; 

    // flags
    int opt;
    while ((opt = getopt_long(argc, argv, "cd12ho", long_options, NULL)) != -1){
        switch (opt){
        case 'c':
            operation = COMPRESS;
            break;
        case 'd':
            operation = DECOMPRESS;
            break;
        case '1':
            symbol_size = 8;
            break;
        case '2':
            symbol_size = 16;
            break;
        case 'h':
            PrintHelp(argv[0]);
            return 0;
        default:
            PrintHelp(argv[0]);
            return 1;
        }
    }
   

    // input dir/file 
    char *input[256];
    int file_count = 0;
    for (int i = optind; i < argc; i++)
        input[file_count++] = argv[i];


    if (file_count == 0){
        fprintf(stderr, "Error: Missing input file/directory.\n");
        return 1;
    }
    

    if (operation == COMPRESS){
        if (file_count == 1){
            if (IsDir(input[0])){    
                CompressDir(input[0], symbol_size);
            } else {
                CompressFile(input[0], symbol_size);
            }
        } else {
            char *archive = "archive.huff";
            CompressFilesToArchive(input, file_count, archive, symbol_size);
        }
    } else if (operation == DECOMPRESS){
        if (IsDir(input[0])){
            DecompressDir(input[0]);
        } else {
            FILE *file = fopen(input[0], "rb");

            // ? archive or file 
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);

            int maybe_file_count = 0;
            if (fread(&maybe_file_count, sizeof(int), 1, file) == 1 && sizeof(int) + maybe_file_count * sizeof(file_index_t) < file_size){
                file_index_t *maybe_index = malloc(maybe_file_count * sizeof(file_index_t));
                fread(maybe_index, sizeof(file_index_t), maybe_file_count, file);
                int valid = 1;
                for (int i = 0; i < maybe_file_count; i++){
                    if (maybe_index[i].position <= 0 || maybe_index[i].position > file_size || 
                    maybe_index[i].length <= 0 || 
                    maybe_index[i].position + maybe_index[i].length > file_size){
                        valid = 0;
                        break;
                    }
                }
                free(maybe_index);
                fseek(file, 0, SEEK_SET);

                if (valid){
                    fclose(file);
                    DecompressArchive(input[0]);
                    return 0;
                }
            }
            fclose(file);
            DecompressFile(input[0]);
        }
    }

    return 0;
}



