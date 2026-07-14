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

    printf("\n                 Memory usage                  \n");
    printf("Orignal size : %d  bytes\n", (MAX_LINES * MAX_WORDS * sizeof(int))); // I wanted to use lu cause well the sizeee iss too biggggggggg lmfao
    printf("bitmap takes size :    %d bytes\n", bitmapSize);
    printf("The :    %d bytes (0 waste)\n", nonZeroCount);
    printf("Total Memory Used:     %d bytes\n", bitmapSize + nonZeroCount);
}

int main()
{
    buildMatrix();
    printAll();
    freeSparseData();

    return 0;
}