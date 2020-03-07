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

    if (read_dynso_symbols(pidno)) {
        plogger(log_error, "load_dynso failure!\n");
        return -1;
    }

    return 0;
}

int32b_t patch::patch_parse_cmd()
{
    int8b_t srcsymbol[1024] = {0};
    int8b_t dstsymbol[1024] = {0};

    if ((std::string::npos != cmdline.find("jmp"))
        && (2 != sscanf(cmdline.c_str(), "jmp %s %s\n", srcsymbol, dstsymbol) != 2)) {
        srcaddr = find_symbol(std::string(srcsymbol));
        if (ulong_mask_t == srcaddr) {
            plogger(log_error, "find symbol[%s] failure!\n", srcsymbol);
            return -1;
        }

        dstaddr = find_symbol(std::string(dstsymbol));
        if (ulong_mask_t == dstaddr) {
            plogger(log_error, "find symbol[%s] failure!\n", dstsymbol);
            return -1;
        }

        offset = dstaddr - srcaddr - 5;
        plogger(log_debug, "srcaddr[%p], dstaddr[%p], offset[%p]\n", srcaddr, dstaddr, offset);

        return 0;
    }

    return -1;
}

int32b_t patch::patch_jump_inject()
{
    int8b_t jmpbuf[5] = {0};
    ulong_t curaddr = 0;
    ulong_t len = 0;
    long_t *lv = 0;

    jmpbuf[0] = 0xe9;

    memcpy(&jmpbuf[1], &offset, sizeof(ulong_t));
    curaddr = srcaddr & (~(sizeof(ulong_t)));
    len = (((srcaddr + sizeof(jmpbuf)) - curaddr) + (sizeof(ulong_t) - 1)) / sizeof(ulong_t);
    lv = new long_t[len * sizeof(ulong_t)];
    if (lv) {
        for (ulong_t i = 0; i < len; i++) {
            lv[i] = ptrace(PTRACE_PEEKDATA, pidno, curaddr + i * sizeof(ulong_t), NULL);
            if (-1 == lv[i] && 0 != errno) {
                plogger(log_error, "PTRACE_PEEKDATA failure[%s]!\n", strerror(errno));
                return -1;
            }
        }

        /* Set the jump machine code at the original target address. */
        memcpy((int32b_t *)lv + (srcaddr - curaddr), jmpbuf, sizeof(jmpbuf));

        for (ulong_t i = 0; i < len; i++) {
            /* Set machine code. */
            if (ptrace(PTRACE_POKEDATA, pidno, curaddr + i * sizeof(ulong_t), lv[i]) < 0) {
                plogger(log_error, "PTRACE_POKEDATA failure[%s]!\n", strerror(errno));
                return -1;
            }
        }

        delete [] lv;
    } else {
        return -1;
    }

    return 0;
}
