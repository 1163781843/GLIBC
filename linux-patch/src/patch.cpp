#include <patch.h>
#include <binelf.h>
#include <logger.h>
#include <config.h>

patch::patch(pid_t pidno, const std::string &exename, const std::string &cmdline, const std::string &dynlib)
    : bfdelf(exename, dynlib), cmdline(cmdline)
{
    this->pidno = pidno;
}

patch::~patch()
{}

pid_t patch::patch_get_pidno(void) const
{
    return pidno;
}

int32b_t patch::patch_load_dynso()
{
    if (load_dynso(pidno)) {
        plogger(log_error, "load_dynso failure!\n");
        return -1;
    }

    return 0;
}

int32b_t patch::patch_parse_cmd()
{
    int8b_t srcsymbol[1024] = {0};
    int8b_t dstsymbol[1024] = {0};
    ulong_t srcaddr = 0;
    ulong_t dstaddr = 0;
    ulong_t offset = 0;

    if ((std::string::npos != cmdline.find("jmp"))
        && (2 != sscanf(cmdline.c_str(), "jmp %s %s\n", srcsymbol, dstsymbol) != 2)) {
        srcaddr = find_symbol(std::string(srcsymbol));
        if (ulong_mask_t == srcaddr) {
            plogger(log_error, "find symbol[%s] failure!\n", srcsymbol);
            return -1;
        }

        dstaddr = find_symbol(std::string(dstsymbol));
        if (ulong_mask_t == dstaddr) {
            plogger(log_error, "find symbol[%s] failure!\n", srcsymbol);
            return -1;
        }

        offset = dstaddr - srcaddr - 5;
        plogger(log_debug, "srcaddr[%p], dstaddr[%p], offset[%p]\n", srcaddr, dstaddr, offset);

        return 0;
    }

    return -1;
}
