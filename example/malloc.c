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

static size_t series(size_t size)
{
    int count = 0, i;

    for (count = 0; size != 0; count++, size >>= 1);

    return count;
}

#define byte_aligned(size) size //(1 << series((size)))

static void print_info()
{
    struct mallinfo info = mallinfo();

    log(1, "Total malloc: %lu, total free: %lu, inuse heap: %lu, heap top: %lu, total mmap: %lu, mmap_cout: %d\n",
        info.arena, info.fordblks, info.uordblks, info.keepcost, info.hblkhd, info.hblks);
}

#define ALLOC_TIME 9

static void *start_runtime(void *arg)
{
    int malloc_size = 0;
    int alloc_time = ALLOC_TIME;
    char *a[ALLOC_TIME];
    char *b[ALLOC_TIME];

    //mallopt(M_TRIM_THRESHOLD, 64 * 1024);

    int i, j;
    for(i = 0; i < ALLOC_TIME; i++) {
        a[i] = (char *)malloc(byte_aligned(52722));
        memset(a[i], 1, byte_aligned(52722));
        malloc_size += byte_aligned(52722);
        b[i] = (char *)malloc(byte_aligned(1));
        memset(b[i], 1, byte_aligned(1));
        malloc_size += byte_aligned(1);
    }

    log(1, "malloc finished, %d kB, %d, %d\n", malloc_size / 1024, sizeof(size_t), byte_aligned(52722));
    for(i = ALLOC_TIME - 1; i >= 0; i--) {
        free(a[i]);
        free(b[i]);
    }
    log(1, "free finished\n");

    print_info();

    log(1, "Again malloc and free!\n");
    for(i = 0; i < ALLOC_TIME; i++) {
        a[i] = (char *)malloc(byte_aligned(52722));
        memset(a[i], 1, byte_aligned(52722));
        malloc_size += byte_aligned(52722);
        b[i] = (char *)malloc(byte_aligned(1));
        memset(b[i], 1, byte_aligned(1));
        malloc_size += byte_aligned(1);
    }

    log(1, "malloc finished, %d kB, %d, %d\n", malloc_size / 1024, sizeof(size_t), byte_aligned(52722));
    for(i = ALLOC_TIME - 1; i >= 0; i--) {
        free(a[i]);
        free(b[i]);
    }
    log(1, "free finished\n");

    while (1) {
        sleep(2);
    }
}

static void glib_log_fun(int level, const char *file, int line, const char *msg)
{
    log(level, "[GLIBC] %s:%d %s\n", file, line, msg);
}

int main()
{
    int malloc_size = 0;
    int alloc_time = ALLOC_TIME;
    char *a[ALLOC_TIME];
    char *b[ALLOC_TIME];
    pthread_t thread = -1;

    glibc_set_log(glib_log_fun);

    //mallopt(M_TRIM_THRESHOLD, 64 * 1024);

    int i, j;
    for(i = 0; i < ALLOC_TIME; i++) {
        a[i] = (char *)malloc(byte_aligned(52722));
        memset(a[i], 1, byte_aligned(52722));
        malloc_size += byte_aligned(52722);
        b[i] = (char *)malloc(byte_aligned(1));
        memset(b[i], 1, byte_aligned(1));
        malloc_size += byte_aligned(1);
    }

    log(1, "malloc finished, %d kB, %d, %d\n", malloc_size / 1024, sizeof(size_t), byte_aligned(52722));
    for(i = ALLOC_TIME - 1; i >= 0; i--) {
        free(a[i]);
        free(b[i]);
    }
    log(1, "free finished, %d\n", ((4+16) & ~16));

    pthread_create(&thread, NULL, start_runtime, NULL);
    sleep(1);

    print_info();

    //malloc_trim(0);

    print_info();

    while(1) {
        sleep(3);
    }
}
