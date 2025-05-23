# Huffman Archiver in C

**Lossless file compressor/decompressor** using Huffman coding algorithm. Supports both single files and directories with configurable symbol size (8/16-bit).

## Features
- **Huffman coding** with optimal prefix trees
- Support for **8-bit and 16-bit symbol encoding**
- File **and directory** compression/decompression
- Multi-file archive creation/extraction
- Detailed compression statistics (ratio, sizes)

## Building
```bash
$ git clone https://github.com/Rasymptote/huffman-archiver
$ cd huffman-archiver
$ make
```

## Usage
You can always run with the --help flag to print the docs:
```bash
$ ./huff --help
Usage: ./huff [options] <input>
Options:
 -c, --compress         Compress input files/directory (default).
 -d, --decompress       Decompress input files/direcrory.
 -1, --8bit             Use 8-bit symbols (default).
 -2, --16bit            Use 16-bit symbols.
 -h, --help             Display that information.
```
For example, let's compress and decompress the sample:
```bash
$ ./huff -c sample.txt
Compressing sample.txt -> sample.txt.huff
Input size: 134697 bytes
Compressed size: 74245 bytes
Compression ratio: 55.12%

$ ./huff -d sample.txt.huff
Decompressing sample.txt.huff -> sample.txt
```