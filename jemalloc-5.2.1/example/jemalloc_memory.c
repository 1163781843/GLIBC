#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

__thread int __attribute__((tls_model("initial-exec"))) thread = 1;

static void *memory_malloc(void *userdata)
{
    char *memory[8] = {0};
    int i;

    printf("memory_malloc enter, sizeof(memory) / sizeof(memory[0]): %ld, thread: %p\n", sizeof(memory) / sizeof(memory[0]), &thread);

    for (i = 0; i < sizeof(memory) / sizeof(memory[0]); i++) {
        memory[i] = malloc(i << 2);
        strcpy(memory[i], "Hello World");
    }

    for (i = 0; i < sizeof(memory) / sizeof(memory[0]); i++) {
        free(memory[i]);
        memory[i] = NULL;
    }

    return NULL;
}


int main(int argc, char **argv)
{
    pthread_t thread1;

#if 0
    int index;

    for (index = 0; index < 10; index++) {
        pthread_create(&thread1, NULL, memory_malloc, NULL);
    }
#endif

    memory_malloc(NULL);

    do {
        sleep(1);
    } while (0);

    return 0;
}