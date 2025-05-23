#ifndef UTILS_H
#define UTILS_H

typedef struct file_index_t {
    char filename[256];
    long position;
    long length;
} file_index_t;

int IsDir(char *path);
long GetFileSize(char *file);
void CompressFileTo(char *input_path, char *output_path, int symbol_size);
void CompressFile(char *path, int symbol_size);
void DecompressFileTo(char *input_path, char *output_path);
void DecompressFile(char *path);
void CompressFilesToArchive(char **files, int file_count, char *archive_name, int symbol_size);
void DecompressArchive(char *archive_name);
void CompressDir(char *path, int symbol_size);
void DecompressDir(char *path);

#endif
