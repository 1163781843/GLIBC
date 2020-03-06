#ifndef __PATCH_H__
#define __PATCH_H__

#include <memory>
#include <list>
#include <iostream>

#include <binelf.h>
#include <config.h>

class patch : public bfdelf {
public:
    patch();
    ~patch();

    enum {
        patch_targpid = 0 << 1,
        patch_targbin = 1 << 1,
        patch_command = 1 << 2,
        patch_total   = patch_targpid | patch_targbin | patch_command,
        patch_dynlib  = 1 << 4
    };
private:
    static std::list<std::shared_ptr<symbol>> symaddrs;
    pid_t pidno;
};

#endif
