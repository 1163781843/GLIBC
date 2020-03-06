#ifndef __PATCH_H__
#define __PATCH_H__

#include <memory>
#include <list>
#include <iostream>

#include <binelf.h>
#include <config.h>

class patch : public bfdelf {
public:
    patch(pid_t pidno, const std::string &exename, const std::string &cmdline, const std::string &dynlib = nullptr);
    ~patch();

    enum {
        patch_targpid = 0 << 1,
        patch_targbin = 1 << 1,
        patch_command = 1 << 2,
        patch_total   = patch_targpid | patch_targbin | patch_command,
        patch_dynlib  = 1 << 4
    };

    pid_t patch_get_pidno(void) const;
    int32b_t patch_load_dynso();
    int32b_t patch_parse_cmd();
private:
    pid_t pidno;
    const std::string &cmdline;
};

#endif
