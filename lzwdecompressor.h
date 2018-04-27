// Wrapper struct for the LZW compressed input file to store state while reading 12 bits at a time
typedef struct LZWFile {
    FILE* fp;               // File pointer to input file
    uint8_t bits;           // 4 bits left over from last byte to be added to the next byte
    int bitsleftover;       // Flag indicating if bits are left over from last read or not - 0 or 1
    unsigned long length;
} LZWFile_t;

// Struct to store each dictionary entry
typedef struct LZWDictEntry {
    unsigned char *entry;   // The entries string
    int length;             // Length of the string of characters in bytes, does not include any null terminator 
} LZWDictEntry_t;

// Decompress an input file to an output file returns 1 if successful, 0 if not
int decompress(FILE* , FILE*);
