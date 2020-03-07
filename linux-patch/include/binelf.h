#ifndef __BINELF_H__
#define __BINELF_H__

#include <iostream>
#include <sysmem.h>
#include <bfd.h>

class symbol {
public:
    symbol(const std::shared_ptr<std::string> &symbname, ulong_t memaddr);
    ~symbol();
    const std::shared_ptr<std::string> &symbname_get(void) const;
    ulong_t memaddr_get() const;
private:
    const std::shared_ptr<std::string> symbname;
    ulong_t memaddr;
};

class bfdelf : public sysmem {
public:
    bfdelf(const std::string &exename, const std::string &dynlib = nullptr);
    ~bfdelf();
    int32b_t readsymbols(pid_t pidno);
    int32b_t read_dynso_symbols(pid_t pidno);
    int32b_t load_dynso(pid_t pidno);
    int32b_t unload_dynso(pid_t pidno);
    ulong_t find_symbol(const std::string &src) const;
private:
    int32b_t push_symbols(bfd *abfd, int32b_t offset, const int8b_t *filename);
    static std::list<std::shared_ptr<symbol>> symaddrs;
    bfd *elfbfd;
    const std::string &exename;
    const std::string &dynlib;
    ulong_t handle;
};

#endif
