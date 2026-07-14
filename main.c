#include <stdio.h>
#include "sparse_utils.h"

void printAll()
{
    printf("--- Original Text ---\n");
    for (int i = 0; i < lineCount; i++)
        printf("[%d] %s\n", i, lines[i]);

    printf("\n--- Vocabulary Index ---\n");
    for (int i = 0; i < wordCount; i++)
        printf("%d: %s ", i, words[i]);

    printf("\n\n--- Compressed Sparse Matrix (Frequencies) ---\n");
    for (int i = 0; i < lineCount; i++)
    {
        for (int j = 0; j < wordCount; j++)
        {

            // dynamic sourcing 😎 , so basically I get it from my vault
            int frequency = getSparseValue(i, j);

            // ab zero toh dikha nahi sakta cus well... ganda dikhega
            if (frequency == 0)
                printf(". ");
            else
                printf("%d ", frequency);
        }
        printf("\n");
    }

    // bitmapSize is 125 cus its built for MAX_WORDS=100, but we only ever get 33 real words.
    // so 67 columns of bitmap are just... sitting there doing nothing. counting what we ACTUALLY use.
    int usedBitmapBytes = (lineCount * wordCount + 7) / 8; // +7 so it rounds up, cant have half a byte lol

    printf("\n                 Memory usage                  \n");
    printf("Orignal size : %lu  bytes\n", (unsigned long)(MAX_LINES * MAX_WORDS * sizeof(int))); // finally used lu, the warning was annoying me
    printf("bitmap takes size :    %d bytes\n", usedBitmapBytes);
    printf("The vault :    %d bytes (0 waste)\n", nonZeroCount);
    printf("Total Memory Used:     %d bytes\n", usedBitmapBytes + nonZeroCount);
}

int main()
{
    buildMatrix();
    printAll();
    freeSparseData();

    return 0;
}
