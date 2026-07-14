#ifndef SPARSE_UTILS_H   // basically ask the compiler to ignore this in the build if it already has already seen it so no crashes
#define SPARSE_UTILS_H // the flag necessary for the above header guard

#include <stdlib.h> // Required for dynamic memory allocation (malloc/free)

// if you are wondering why dynamic memory its because why you waste extra space !!

#define MAX_LINES 10
#define MAX_WORDS 100
#define bitmapSize 125 // 10 times 10 = 1000 / 1 byte = 125 bytes lesgooo , intrestingly the most space taking stuff

// Original Data Structures
extern char lines[MAX_LINES][200];
extern char words[MAX_WORDS][20];
extern int wordCount;
extern int lineCount;

// NEW: Sparse Representation Structures
extern unsigned char bitmap[bitmapSize];   // the representation
extern unsigned char *sparseValues;        // Changed to pointer for dynamic coding (zero waste)
extern int nonZeroCount;                  // Tracks how many 1s are in the bitmap

// Helper Macros ( this aint your protein macros trust me lol) for Bit-Level manipulations
// Finds the byte (pos/8) and the specific bit (pos%8)
#define SET_BIT(map, pos) (map[(pos) / 8] |= (1 << ((pos) % 8)))
#define GET_BIT(map, pos) (map[(pos) / 8] & (1 << ((pos) % 8)))

// Function declarations
int findWord(char *w);
int addWord(char *w);
void buildMatrix();

void compressMatrix();
int getSparseValue(int r, int c);
void freeSparseData(); // Function to clean up the dynamic memory at the end

#endif