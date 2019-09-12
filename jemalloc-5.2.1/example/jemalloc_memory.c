#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static void *memory_malloc(void *userdata)
{
    char *memory[1024] = {0};
    int i;

    printf("memory_malloc enter, sizeof(memory) / sizeof(memory[0]): %ld\n", sizeof(memory) / sizeof(memory[0]));

    for (i = 0; i < sizeof(memory) / sizeof(memory[0]); i++) {
        memory[i] = malloc(i << 2);
    }

    return NULL;
}


int main(int argc, char **argv)
{
    pthread_t thread1;

    //pthread_create(&thread1, NULL, memory_malloc, NULL);

    //sleep(5);
    memory_malloc(NULL);

    while (1) {
        sleep(1);
    }

    return 0;
}