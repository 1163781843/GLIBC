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
    char *log_buffer = NULL;
    time_t tt;
    struct tm *t = NULL;
    struct timeval tv;

    va_list ap;
    va_start(ap, format);
    vasprintf(&log_buffer, format, ap);
    va_end(ap);

#if 1
    tt = time(NULL);
    t = localtime(&tt);
    gettimeofday(&tv, NULL);

    fprintf(stderr, "[%4d-%02d-%02d %02d:%02d:%02d:%03ld] [%04ld] %s:%d -- %s\n", t->tm_year + 1900, t->tm_mon + 1,
        t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec, syscall(SYS_gettid), file, line, log_buffer);
#else
    fprintf(stderr, "[%04ld] %s:%d -- %s\n",
        syscall(SYS_gettid), file, line, log_buffer);
#endif

    if (log_buffer) {
        free(log_buffer);
    }
}

#define log(level, ...) \
    __log((level), __FILE__, __LINE__, __VA_ARGS__)

static size_t series(size_t size)
{
    int count = 0, i;

    for (count = 0; size != 0; count++, size >>= 1);

    //log(1, "count: %d\n", count);

    return count;
}

#define byte_aligned(size) (1 << series((size)))

static void print_info()
{
    struct mallinfo info = mallinfo();

    log(1, "Total malloc: %lu, total free: %lu, inuse heap: %lu, heap top: %lu, total mmap: %lu, mmap_cout: %d\n",
        info.arena, info.fordblks, info.uordblks, info.keepcost, info.hblkhd, info.hblks);
}

static void *start_runtime(void *arg)
{
    int malloc_size = 0;
    int alloc_time = 4000;
    char *a[alloc_time];
    char *b[alloc_time];

    //mallopt(M_TRIM_THRESHOLD, 64 * 1024);

    int i, j;
    for(i = 0; i < alloc_time; i++) {
        a[i] = (char *)malloc(byte_aligned(52722));
        memset(a[i], 1, byte_aligned(52722));
        malloc_size += byte_aligned(52722);
        b[i] = (char *)malloc(byte_aligned(1));
        memset(b[i], 1, byte_aligned(1));
        malloc_size += byte_aligned(1);
    }

    log(1, "malloc finished, %d kB, %d, %d\n", malloc_size / 1024, sizeof(size_t), byte_aligned(52722));
    for(i = alloc_time - 1; i >= 0; i--) {
        free(a[i]);
        free(b[i]);
    }
    log(1, "free finished\n");

    print_info();

    while (1) {
        sleep(2);
    }
}

int main()
{
    int malloc_size = 0;
    int alloc_time = 4000;
    char *a[alloc_time];
    char *b[alloc_time];
    pthread_t thread = -1;

    //mallopt(M_TRIM_THRESHOLD, 64 * 1024);

    int i, j;
    for(i = 0; i < alloc_time; i++) {
        a[i] = (char *)malloc(byte_aligned(52722));
        memset(a[i], 1, byte_aligned(52722));
        malloc_size += byte_aligned(52722);
        b[i] = (char *)malloc(byte_aligned(1));
        memset(b[i], 1, byte_aligned(1));
        malloc_size += byte_aligned(1);
    }

    log(1, "malloc finished, %d kB, %d, %d\n", malloc_size / 1024, sizeof(size_t), byte_aligned(52722));
    for(i = alloc_time - 1; i >= 0; i--) {
        free(a[i]);
        free(b[i]);
    }
    log(1, "free finished\n");

    pthread_create(&thread, NULL, start_runtime, NULL);
    sleep(1);

    print_info();

    malloc_trim(0);

    print_info();

    while(1) {
        sleep(3);
    }
}
