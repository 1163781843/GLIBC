#ifndef __PATCH_H__
#define __PATCH_H__

#include <queue>

class symaddr {
public:
    symaddr(std::string    *name, int addr);
    ~symaddr();
private:
    std::string *name;
    int addr;
};

class patch {
public:
    patch();
    ~patch();

    int patch_symbol_init(pid_t pid, const char *filename);
    int patch_read_symbols(bfd *abfd, int offset);
private:
    std::queue<symaddr *> symaddrs;
};

#endif
