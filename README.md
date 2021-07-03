# Huffman Compression in C

Simple huffman compression written in C to get used to the language.

`make` to compile the code, then

```console
$ ./huff myfile
... generates myfile.pine

$ ./huff -d myfile.pine
... decodes myfile.pine

$ ./huff -d -f myfile_newname myfile.pine
... decodes myfile.pine into a file named myfile_newname
```

## Testing

Run `./files_eq.sh <file or directory>`. The script will compress and decompress your file, or each file in your directory, and compares it to the original.

## Performance

Filesize of `enwik8` is ~95 Mb, where the compressed `enwik8.pine` is ~62 Mb. For reference, `gzip` gets `enwik8` down to ~35 Mb. No Hutter Prize for me anytime soon.

Further, performance drops drastically depending on the characterstics of the file. The absolute worst case would be every possible token (i.e. all bytes from value 0x00 to 0xff) being listed an equal amount of times. This is the file `testfiles/all_ascii`, and my program "compresses" it from 256 bytes to ~1 kB. This is because Huffman Compression encodes characters in a binary tree, where more frequent characters are higher up the tree. If every character occurs an equal amount of times, every character will be a leaf at the same depth, and there will be no benefit from ordering the bytes by their frequency in the tree. With the added cost of storing the symbol definitions in the compressed file, the compressed file size increases quick.
