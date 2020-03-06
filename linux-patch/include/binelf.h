#ifndef __BINELF_H__
#define __BINELF_H__

#include <iostream>

#include <bfd.h>

class symbol {
public:
    symbol();
    ~symbol();
private:
};

class bfdelf {
public:
    bfdelf();
    ~bfdelf();
private:
    bfd *elfbfd;
    std::string exename;
    std::string dynlib;
};

#endif
