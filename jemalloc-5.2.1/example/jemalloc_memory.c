#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    char *memory[128] = {0};
    int i;

    for (i = 0; i < 128; i++) {
        memory[i] = malloc(i);
    }

    return 0;
}