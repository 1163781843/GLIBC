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

static void glib_log_fun(int level, const char *file, int line, const char *msg)
{
    log(level, "[GLIBC] %s:%d %s\n", file, line, msg);
}

#define TCACHE_BIN_ROW_SIZE 64
#define TCACHE_BIN_COL_SIZE 7
#define TCACHE_SIZE         (1 << 4)
#define TCACHE_THREAD       0

static pthread_t tcache_threads[TCACHE_THREAD] = {-1};

static void tcache_malloc(char *tcaches[TCACHE_BIN_ROW_SIZE][TCACHE_BIN_COL_SIZE])
{
    int i, j;
    size_t size_count = 0;
    size_t count = 0;

    for (i = 0; i < TCACHE_BIN_COL_SIZE; i++) {
        for (j = 0; j < TCACHE_BIN_ROW_SIZE; j++) {
            count++;
            size_t size = TCACHE_SIZE -1 + TCACHE_SIZE * j;
            size_count += size;
            tcaches[j][i] = malloc(size);
        }
    }
    log(1, "Total malloc: %ld bytes, malloc count: %ld\n", size_count, count);
}

static void tcache_free(char *tcaches[TCACHE_BIN_ROW_SIZE][TCACHE_BIN_COL_SIZE])
{
    int i, j;
    size_t count = 0;

    for (i = 0; i < TCACHE_BIN_COL_SIZE - 1; i++) {
        for (j = 0; j < TCACHE_BIN_ROW_SIZE; j++) {
            if (tcaches[j][i]) {
                count++;
                free(tcaches[j][i]);
                tcaches[j][i] = NULL;
            }
        }
    }
    log(1, "Total free count: %ld\n", count);
}

static void *tcache_thread(void *arg)
{
    char *tcaches[TCACHE_BIN_ROW_SIZE][TCACHE_BIN_COL_SIZE];

    sleep(60);

    tcache_malloc(tcaches);
    tcache_free(tcaches);

    tcache_malloc(tcaches);
    tcache_free(tcaches);

    getchar();
}

static void tcache_pthreads()
{
    int i;
    pthread_t thread;

    for (i = 0; i < TCACHE_THREAD; i++) {
        pthread_create(&tcache_threads[i], NULL, tcache_thread, NULL);
    }

    log(1, "Start tcache %d thread!\n", TCACHE_THREAD + 1);
    pthread_create(&thread, NULL, tcache_thread, NULL);
}

int main()
{
    char *tcaches[TCACHE_BIN_ROW_SIZE][TCACHE_BIN_COL_SIZE];
    pthread_t pthread;

    glibc_set_log(glib_log_fun);

    tcache_malloc(tcaches);
    tcache_free(tcaches);

    tcache_malloc(tcaches);
    tcache_free(tcaches);

    print_info();

    tcache_pthreads();

    getchar();

    print_info();

    return 0;
}