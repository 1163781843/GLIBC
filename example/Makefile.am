AUTOMAKE_OPTIONS = foreign

##
## Applications
##

sbin_PROGRAMS               = glibc_malloc \
							  glibc_malloc_tcache \
							  glibc_malloc_arena

glibc_malloc_SOURCES        = glibc_malloc.c
glibc_malloc_CFLAGS         = $(AM_CFLAGS)
glibc_malloc_LDADD          = -lpthread
glibc_malloc_LDFLAGS        = -no-undefined -export-dynamic -lm $(AM_LDFLAGS)


glibc_malloc_tcache_SOURCES = glibc_malloc_tcache.c
glibc_malloc_tcache_CFLAGS  = $(AM_CFLAGS)
glibc_malloc_tcache_LDADD   = -lpthread
glibc_malloc_tcache_LDFLAGS = -no-undefined -export-dynamic -lm $(AM_LDFLAGS)

glibc_malloc_arena_SOURCES  = glibc_malloc_arena.c
glibc_malloc_arena_CFLAGS   = $(AM_CFLAGS)
glibc_malloc_arena_LDADD    = -lpthread
glibc_malloc_arena_LDFLAGS  = -no-undefined -export-dynamic -lm $(AM_LDFLAGS)