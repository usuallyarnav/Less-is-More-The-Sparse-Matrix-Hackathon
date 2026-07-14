#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Required for malloc, calloc, and free
#include "sparse_utils.h"

// --- 1. Original Text Data ---
char lines[MAX_LINES][200] = {
    "data structures are fun and data is powerful",
    "algorithms and data structures are important",
    "learning data structures takes time and practice",
    "practice makes concepts in data structures strong",
    "algorithms solve problems and data helps",
    "understanding structures and algorithms is useful",
    "data data data makes patterns visible",
    "practice and patience improve problem solving",
    "structures help organize data efficiently",
    "algorithms and structures go hand in hand"
};

// Dictionary Variables
char words[MAX_WORDS][20];
int lineCount = 10;
int wordCount = 0;

// --- 2. Defining the Sparse Variables ---
// We declare the actual storage that was promised in the header file
unsigned char bitmap[bitmapSize] = {0}; // Fills the 125-byte map with 0s initially
unsigned char *sparseValues = NULL;     // The pointer that will become our exact-size vault
int nonZeroCount = 0;                   // Counter for the exact number of words

// Temporary storage used ONLY during setup
int *tempMatrix = NULL;

// --- 3. Dictionary Helper Functions ---
int findWord(char *w) {
    for(int i = 0; i < wordCount; i++) {
        if(strcmp(words[i], w) == 0) return i;
    }
    return -1;
}

int addWord(char *w) {
    int idx = findWord(w);
    if(idx != -1) return idx;

    strcpy(words[wordCount], w);
    wordCount++;
    return wordCount - 1;
}

// --- 4. The Build & Compress Pipeline ---
void buildMatrix() {
    // TRICK #1: We dynamically allocate a temporary 1D array to act as our 2D grid.
    // calloc fills it with zeros automatically. It takes 4,000 bytes temporarily.
    tempMatrix = (int *)calloc(MAX_LINES * MAX_WORDS, sizeof(int));

    char temp[200];
    char *token;

    // Parse the text just like the original code
    for(int i = 0; i < lineCount; i++) {
        strcpy(temp, lines[i]);
        token = strtok(temp, " ");
        while(token != NULL) {
            int col = addWord(token);
            // Convert 2D coordinates (row, col) into a 1D position
            tempMatrix[(i * MAX_WORDS) + col]++;
            token = strtok(NULL, " ");
        }
    }

    // Now that the temporary grid is full of frequencies, we instantly compress it.
    compressMatrix();
}

void compressMatrix() {
    // 1. Scan the temporary grid to find exactly how many non-zero numbers exist.
    nonZeroCount = 0;
    for (int i = 0; i < MAX_LINES * MAX_WORDS; i++) {
        if (tempMatrix[i] > 0) {
            nonZeroCount++;
        }
    }

    // TRICK #2: We allocate the exact amount of bytes needed for our vault. ZERO waste.
    sparseValues = (unsigned char *)malloc(nonZeroCount * sizeof(unsigned char));

    // 2. Fill the Bitmap and the Vault
    int vaultIndex = 0;
    for (int i = 0; i < MAX_LINES * MAX_WORDS; i++) {
        if (tempMatrix[i] > 0) {
            SET_BIT(bitmap, i);                       // Flip the light switch to ON
            sparseValues[vaultIndex] = tempMatrix[i]; // Store the frequency in the 1-byte char
            vaultIndex++;
        }
    }

    // TRICK #3: Destroy the massive 4,000-byte temporary grid.
    // From this point on, your program is running solely on ~185 bytes!
    free(tempMatrix);
    tempMatrix = NULL;
}

// --- 5. The Retrieval Function ---
int getSparseValue(int r, int c) {
    // Find the 1D position of the requested row and column
    int pos = (r * MAX_WORDS) + c;

    // If the bit is 0, the word isn't there. Return 0 immediately without searching.
    if (!GET_BIT(bitmap, pos)) {
        return 0;
    }

    // If the bit is 1, count how many 1s came BEFORE this position.
    // This tells us exactly which slot in the sparseValues vault holds our number.
    int count = 0;
    for (int i = 0; i < pos; i++) {
        if (GET_BIT(bitmap, i)) {
            count++;
        }
    }

    return sparseValues[count];
}

// --- 6. Memory Clean Up ---
void freeSparseData() {
    // This must be called at the end of main.c to prevent memory leaks
    if (sparseValues != NULL) {
        free(sparseValues);
        sparseValues = NULL;
    }
}