#ifndef __JEMALLOC_H__
#define __JEMALLOC_H__

#ifdef GRANDSTREAM_NETWORKS
static inline void jelogger(int level, const char *file, int line, const char *format, ...)
{
	char buffer[65535] = {0};
	va_list ap;
	int size = 0;

	size = snprintf(buffer, sizeof(buffer) - 1, "[%s:%d] [%ld] -- ", file, line, syscall(SYS_gettid));

	va_start(ap, format);
	vsnprintf(buffer + size, sizeof(buffer) - size - 1, format, ap);
	va_end(ap);

	fprintf(stderr, "%s\n", buffer);
}

#define jelog(level, ...) do {									\
	if (level) {											\
		jelogger(level, __FILE__, __LINE__, __VA_ARGS__);	\
	}														\
} while (0)
#endif

#endif
