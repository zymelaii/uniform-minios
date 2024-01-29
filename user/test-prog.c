#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("%8d\n\n\n", 1);

    for (int j = 0; j <= 100; ++j) {
        printf("\r");
        printf("%80d\r", j);
        for (int i = 0; i < j * 70 / 100; ++i) { printf("|"); }
        for (int k = 0; k < 5000000; ++k) {
            // do nothing
        }
    }
    printf("\n");
}
