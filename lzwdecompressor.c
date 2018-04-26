#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "lzwdecompressor.h"

// Get the next 12 bit code from the input file
// Supplied with a pointer to a LZWFile struct to wrap the file pointer together with state
// Returns a valid code between 0 - 4095 or 4096 if no code 
static uint16_t getNextCode(LZWFile_t *input) {
    uint16_t code;
    uint8_t bytes[2];

    if (input->bitsleftover) {
        // There are 4 bits remaining from a previously read byte
        bytes[0] = input->bits;
        // And read in 1 new byte, checking the read was successful
        if ((fread(bytes+1, sizeof(uint8_t), 1, input->fp)) < 1)
            return 4096;

        // Code is the leftover 4 bits left shifted 8 OR'd with the whole newly read byte
        code = (bytes[0] << 8) | bytes[1];
        input->bitsleftover = 0;
    } else {
        // Need to read in 2 new bytes
        if ((fread(bytes, sizeof(uint8_t), 2, input->fp)) < 2)
            return 4096;

        // Check if these 2 bytes were the last in the input file
        if (ftell(input->fp) == input->length) {
            // If so - account for padding of 12 to 16 bits, can just shift first byte right by 8 because first 4 bits already zero 
            code = (bytes[0] << 8) | bytes[1];
            fprintf(stdout, "End of file, byte[0]: %d byte[1]: %d code: %d\n", bytes[0], bytes[1], code);
        } else {
            // Code is the first byte left shifted 4 OR'd with second byte right shifted 4
            code = (bytes[0] << 4) | (bytes[1] >> 4);

            // Save last 4 bits of second byte in the input struct to make next code
            input->bitsleftover = 1;
            input->bits = bytes[1] & 0b00001111;
        }
    }
    return code;
}

// Reset dictionary from position specificed by start ending at len
// Provided with pointer to start of array and length of array
static void resetDictionary(LZWDictEntry_t *dict, int start, int len) {
    // Free each entry and zero the structure
    for (int i=start; i<len; i++) {
        free(dict[i].entry);
        memset(dict+i, 0, sizeof(LZWDictEntry_t));
    }
}

// Decompress a LZW compressed file
// Takes an compressed input file and a file to output to 
void decompress(FILE *inputfile, FILE *outputfile) {
    uint16_t prevcode;                                          // The last code read from the compressed input file 
    uint16_t currcode;                                          // The current code read from the compressed input file
    int dictIndex = 0;                                          // The next free index in the dictionary to put new entries 
    LZWDictEntry_t dictionary[4096];                            // Dictionary of 4096 string entries and their lengths - 12 bits can only address up to 4095

    

    // Open supplied input file and initialise LZWFile_t structure
    LZWFile_t *input = malloc(sizeof(LZWFile_t));
    memset(input, 0, sizeof(LZWFile_t));
    input->bitsleftover = 0;
    input->fp = inputfile;
    // Determine number of bytes in file
    if (fseek(input->fp,0,SEEK_END) == 0) {
        input->length = ftell(input->fp);
        rewind(input->fp);
        fprintf(stdout, "File is %lu bytes long\n", input->length);
    } else {
        fprintf(stderr, "Could not find end of file\n");
        return;
    }

    // Initialise dictionary with ASCII entries from 0 to 255
    memset(dictionary, 0, sizeof(LZWDictEntry_t) * 4096);
    for (dictIndex = 0; dictIndex<256; dictIndex++) {
        dictionary[dictIndex].entry = malloc(sizeof(unsigned char));
        *(dictionary[dictIndex].entry) = dictIndex;
        dictionary[dictIndex].length = 1;
    }

    // Get the first code from compressed input, check it is valid and then output it
    if ((prevcode = getNextCode(input)) >= dictIndex) {
        fprintf(stderr, "First code not in dictionary\n");
        resetDictionary(dictionary, 0, dictIndex); // Free used dictionary entries because exiting
        free(input);
        return;
    }
    fwrite(dictionary[prevcode].entry, sizeof(unsigned char), dictionary[prevcode].length, outputfile);
    while ((currcode = getNextCode(input)) < 4096) {

        if (currcode > dictIndex) {
            fprintf(stderr, "Code out of range of dictionary\n");
            resetDictionary(dictionary, 0, dictIndex); // Free used dictionary entries because exiting
            free(input);
            return;
        }

        // Initialise new dictionary entry 
        dictionary[dictIndex].length = dictionary[prevcode].length + 1;
        dictionary[dictIndex].entry = malloc(dictionary[dictIndex].length * sizeof(unsigned char));
        memset(dictionary[dictIndex].entry, 0, dictionary[dictIndex].length * sizeof(unsigned char));

        if (currcode >= dictIndex) {
            // The current code is the one that is about to be created this iteration but is not available in dictionary yet,
            // so take first character from previous output
            memcpy(dictionary[dictIndex].entry, dictionary[prevcode].entry, dictionary[prevcode].length);
            memcpy(dictionary[dictIndex].entry + dictionary[prevcode].length, dictionary[prevcode].entry, 1);
        } else {
            // Take first char from current code that is already in the dictionary
            memcpy(dictionary[dictIndex].entry, dictionary[prevcode].entry, dictionary[prevcode].length);
            memcpy(dictionary[dictIndex].entry + dictionary[prevcode].length, dictionary[currcode].entry, 1);
        }

        dictIndex++;

        fwrite(dictionary[currcode].entry, sizeof(unsigned char), dictionary[currcode].length, outputfile);

        if (dictIndex >= 4096) {
            // Reset dictionary to use again, preserving the initial ASCII alphabet
            resetDictionary(dictionary, 256, 4096);
            dictIndex = 256;
        }
        prevcode = currcode;
    }

    fprintf(stdout, "Last char: %d %s\n", prevcode, dictionary[prevcode].entry);

    resetDictionary(dictionary, 0, 4096); // Free whole dictionary because exiting
    free(input);
}

int main(int argc, const char * argv[]) {
    FILE* input;
    FILE* output;
    char outputname[256];

    if (argc != 2) {
        fprintf(stderr, "Usage:%s filename\n", argv[0]);
        exit(1);
    }

    input = fopen(argv[1], "rb");
    if (input == NULL) {
        fprintf(stderr, "Could not open file %s\n", argv[1]);
        exit(1);
    }

    strcat(outputname, argv[1]);
    strcat(outputname, ".decompressed");

    output = fopen(outputname, "wb+");
    if (output == NULL) {
        fprintf(stderr, "Could not open output file\n");
        exit(1);
    }
    
    decompress(input, output);

    fclose(input);
    fclose(output);
}