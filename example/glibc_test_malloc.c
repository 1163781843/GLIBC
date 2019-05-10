#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <stdarg.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/syscall.h>

static void __log(int level, const char *file, int line, const char *format, ...)
{
    struct timeval tv;
    char buffer[1024] = {0};

    va_list ap;
    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);

    gettimeofday(&tv, NULL);

    fprintf(stderr, "[%ld:%06ld] [%04ld] %s:%03d -- %s\n",
        tv.tv_sec, tv.tv_usec, syscall(SYS_gettid), file, line, buffer);
}

#define log(level, ...) \
    __log((level), __FILE__, __LINE__, __VA_ARGS__)

#define byte_default_aligned(size, boundary) \
    (((size) + ((boundary) - 1)) & ~((boundary) - 1))


#define MALLINFO_FORMAT "%50s: %lu (%lu KB)\n"

static void print_info()
{
    struct mallinfo info = mallinfo();

    log(1,
        "\n\033[33;1m"MALLINFO_FORMAT
        MALLINFO_FORMAT
        MALLINFO_FORMAT
        MALLINFO_FORMAT
        MALLINFO_FORMAT
        MALLINFO_FORMAT
        MALLINFO_FORMAT
        MALLINFO_FORMAT
        MALLINFO_FORMAT
        MALLINFO_FORMAT"\33[0m",
        "Total arena number(include main and thread arena)", info.arena_count, info.arena_count / 1024,
        "Non-mmapped space allocated (bytes)", info.arena, info.arena / 1024,
        "Number of free chunks", info.ordblks, info.ordblks / 1024,
        "Number of free fastbin blocks", info.smblks, info.smblks / 1024,
        "Number of mmapped regions", info.hblks, info.hblks / 1024,
        "Space allocated in mmapped regions (bytes)", info.hblkhd, info.hblkhd / 1024,
        "Maximum total allocated space (bytes)", info.usmblks, info.usmblks / 1024,
        "Space in freed fastbin blocks (bytes)", info.uordblks, info.uordblks / 1024,
        "Total free space (bytes)", info.fordblks, info.fordblks / 1024,
        "Top-most, releasable space (bytes)", info.keepcost, info.keepcost / 1024);
}

#define ALLOC_TIME 1024

static void glib_log_fun(int level, const char *file, int line, const char *msg)
{
    log(level, "[GLIBC] %s:%d %s\n", file, line, msg);
}

int main()
{
    int malloc_size = 0;
    char *a[ALLOC_TIME];
    char *b[ALLOC_TIME];
    pthread_t thread = -1;

    glibc_set_log(glib_log_fun);

    int i, j;
    for(i = 0; i < ALLOC_TIME; i++) {
        int size = i;
        a[i] = (char *)malloc(size);
        memset(a[i], 1, size);
        malloc_size += size;
    }
    log(1, "malloc finished, %d bytes\n", malloc_size);

    for(i = ALLOC_TIME - 1; i >= 0; i--) {
        free(a[i]);
    }
    log(1, "free finished, %d\n", ((4+16) & ~16));

    sleep(1);

    print_info();

    getchar();
}
