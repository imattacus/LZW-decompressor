This is an implementation of the LZW decompression algorithm written in C.
It intialises the first 256 entries in the dictionary to be the ASCII alphabet, and then reads 12 bit codes from the compressed input file, building up the same dictionary of substrings that would have been created by the compressor.

Further improvements that should be made:
* Currently only works given an input file and an output file. Would be more flexible and reusable if it could accept any buffer as input and output as a string.
	- The dictionary stores each entries length alongside it so building an output string would be simple.
	- The LZWFile_t struct which holds the file pointer and state of reading from the file could be modified to use a tagged union, so the struct will hold either a file or some other input stream. I would then implement different getNextCode functions if the input streams require a different way of getting the codes from them and include a function pointer to the appropriate function in the struct.

Compiled with:
```gcc -o decompressor lzwdecompressor.c -Wall -Werror -pedantic```
on both macOS 10.13.3 and Debian 9 

Tested for memory leaks with:
```valgrind --leak-check=yes ./decompressor LzwInputData/compressedfile4.z```
on Debian, No memory leaks were detected.


