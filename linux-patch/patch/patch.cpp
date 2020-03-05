#include <iostream>
#include <memory>
#include <fstream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <sys/mman.h>
#include <sys/syscall.h>

#include <bfd.h>

#include <patch.h>
#include <logger.h>

static long mmap_args[] = {
    0,
    -1,
    MAP_PRIVATE | MAP_ANONYMOUS,
    PROT_READ | PROT_WRITE,
    0x4000,
    0
};

std::string *symaddr::symaddr_get_name(void) const
{
    return name;
}

int symaddr::symaddr_get_addr(void) const
{
    return addr;
}

void symaddr::symaddr_show(void) const
{
}

symaddr::symaddr(std::string *name, int addr) : name(name)
{
    this->addr = addr;
    plogger(log_verbose, "symbol name[%s], addr[%p]\n", name->c_str(), addr);
}

symaddr::~symaddr()
{}

int patch::patch_continue(void) const
{
    if (ptrace(PTRACE_CONT, targpid, NULL, NULL)) {
        plogger(log_error, "PTRACE_CONT failure!\n");
        return -1;
    }

    wait(NULL);

    return 0;
}

int patch::patch_push_stack(struct user_regs_struct *registers, long value)
{
    registers->esp -= sizeof(int);

    if (ptrace(PTRACE_POKEDATA, targpid, registers->esp, value)) {
        plogger(log_error, "PTRACE_POKEDATA failure!\n");
        return -1;
    }

    return 0;
}

void patch::patch_backup_registers(const struct user_regs_struct *registers)
{
    memcpy(&orgregs, registers, sizeof(orgregs));
}

int patch::patch_restore_registers(void) const
{
    if (ptrace(PTRACE_SETREGS, targpid, NULL, &orgregs)) {
        plogger(log_error, "PTRACE_SETREGS failure!\n");
        return -1;
    }

    return 0;
}

int patch::patch_read_registers(struct user_regs_struct *registers)
{
    if (ptrace(PTRACE_GETREGS, targpid, NULL, registers) < 0) {
        plogger(log_error, "PTRACE_GETREGS failure!\n");
        return -1;
    }

    plogger(log_notice, "registers->eip[%p], registers->esp[%p], registers->ebx[%p], registers->eax[%p]\n", registers->eip, registers->esp, registers->ebx, registers->eax);

    return 0;
}

int patch::patch_write_registers(struct user_regs_struct *registers)
{
    if (ptrace(PTRACE_SETREGS, targpid, NULL, registers)) {
        plogger(log_error, "PRTACE_SETREGS failure!\n");
        return -1;
    }

    return 0;
}

int patch::patch_set_data(void)
{
    char jmpbuf[5] = {0};
    int curaddr = 0;
    int len = 0;
    int *lv = NULL;

    jmpbuf[0] = 0xe9;
    memcpy(&jmpbuf[1], &targoffset, sizeof(int));

    curaddr = targsrcaddrinfo & (~3); /* Address 4 byte alignment. */
    len = (((targsrcaddrinfo + sizeof(jmpbuf)) - curaddr) + 3) / 4;
    lv = new int[len * sizeof(int)];
    if (lv) {
        for (int i = 0; i < len; i++) {
            /* Get machine code. */
            lv[i] = ptrace(PTRACE_PEEKDATA, targpid, curaddr + i * sizeof(int), NULL);
            if (-1 == lv[i] && 0 != errno) {
                plogger(log_error, "PTRACE_PEEKDATA failure[%s]!\n", strerror(errno));
                return -1;
            }
            plogger(log_notice, "lv[%d][%p], (curaddr + i * sizeof(int))[%p], (targsrcaddrinfo + sizeof(jmpbuf))[%p]\n",
                i, lv[i], curaddr + i * sizeof(int), (targsrcaddrinfo + sizeof(jmpbuf)));
        }

        /* Set the jump machine code at the original target address. */
        memcpy((char *)lv + (targsrcaddrinfo - curaddr), jmpbuf, sizeof(jmpbuf));

        for (int i = 0; i < len; i++) {
            /* Set machine code. */
            if (ptrace(PTRACE_POKEDATA, targpid, curaddr + i * sizeof(int), lv[i]) < 0) {
                plogger(log_error, "PTRACE_POKEDATA failure[%s]!\n", strerror(errno));
                return -1;
            }

            plogger(log_notice, "lv[%d][%p], (curaddr + i * sizeof(int))[%p], (targsrcaddrinfo + sizeof(jmpbuf))[%p]\n",
                i, lv[i], curaddr + i * sizeof(int), (targsrcaddrinfo + sizeof(jmpbuf)));
        }

        delete [] lv;
    } else {
        return -1;
    }
    plogger(log_notice, "curaddr[%p], targsrcaddrinfo[%p], len[%d], sizeof(jmpbuf)[%d]\n", curaddr, targsrcaddrinfo, len, sizeof(jmpbuf));

    return 0;
}

int patch::patch_read_symbols(bfd *abfd, int offset, const char *filenames)
{
    long storage_needed;
    int ret = 0;
    asymbol **symbol_table = NULL;
    long number_of_symbols;
    long i;

    storage_needed = bfd_get_symtab_upper_bound(abfd);
    if (storage_needed < 0) {
        plogger(log_warning, "bfd_get_symtab_upper_bound failure!\n");
        ret = -1;
        goto dynsym;
    }

    if (0 == storage_needed) {
        plogger(log_warning, "no symbols!\n");
        goto dynsym;
    }

    symbol_table = new asymbol*[storage_needed];
    number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
    if (number_of_symbols < 0) {
        plogger(log_warning, "bfd_canonicalize_symtab failure!\n");
        goto dynsym;
    }

    for (i = 0; i < number_of_symbols; i++) {
        asymbol *asym = symbol_table[i];
        const char *sym_name = bfd_asymbol_name(asym);
        int symclass = bfd_decode_symclass(asym);
        int sym_value = bfd_asymbol_value(asym) + offset;

        if (*sym_name == '\0') {
            continue;
        }

        if (bfd_is_undefined_symclass(symclass)) {
            continue;
        }
        symaddrs.push_back(new symaddr(new std::string(sym_name), sym_value));
    }

dynsym:
    if (symbol_table) {
        delete []symbol_table;
        symbol_table = nullptr;
    }

    storage_needed = bfd_get_dynamic_symtab_upper_bound(abfd);
    if (storage_needed < 0) {
        plogger(log_warning, "bfd_get_dynamic_symtab_upper_bound failure!\n");
        ret = -1;
        goto out;
    }
    if (storage_needed == 0) {
        plogger(log_warning, "no symbols!\n");
        goto out;
    }

    symbol_table = new asymbol*[storage_needed];
    number_of_symbols = bfd_canonicalize_dynamic_symtab(abfd, symbol_table);
    if (number_of_symbols < 0) {
        plogger(log_warning, "bfd_canonicalize_symtab failure!\n");
        ret = -1;
        goto out;
    }

    for (i = 0; i < number_of_symbols; i++) {
        asymbol *asym = symbol_table[i];
        const char *sym_name = bfd_asymbol_name(asym);
        int symclass = bfd_decode_symclass(asym);
        int sym_value = bfd_asymbol_value(asym) + offset;

        if (*sym_name == '\0') {
            continue;
        }

        if (bfd_is_undefined_symclass(symclass)) {
            continue;
        }

        //plogger(log_debug, "[%s] start addr[%p], %s = %p\n", filenames, offset, sym_name, sym_value);
        symaddrs.push_back(new symaddr(new std::string(sym_name), sym_value));
    }

out:
    if (symbol_table) {
        delete []symbol_table;
        symbol_table = nullptr;
    }

    return ret;
}

int patch::patch_symbol_init(pid_t pid, std::string &filename)
{
    char buf[4096] = {0};
    bfd *abfd = NULL;
    char targsrcfilename[1024] = {0};
    char *targpname = NULL;

    plogger(log_debug, "target symbol init, pid[%d] filename[%s], mmap[%p]\n", pid, filename.c_str(), mmap);

    snprintf(targsrcfilename, sizeof(targsrcfilename) - 1, "%s", filename.c_str());

    targpname = strrchr(targsrcfilename, '/');

    snprintf(buf, sizeof(buf), "/proc/%d/maps", pid);

    std::ifstream proc_map(buf, std::ios::in);
    if (!proc_map.is_open()) {
        plogger(log_error, "%s open failure!\n", buf);
    }

    while (proc_map.getline(buf, sizeof(buf) - 1)) {
        int vm_start, vm_end, pgoff, major, minor, ino;
        char flags[5], mfilename[4096] = {0};
        char *key = NULL, *tokstr = NULL, *tmpkey = NULL;

        if ((sscanf(buf, "%x-%x %4s %x %d:%d %d %s", &vm_start, &vm_end,
            flags, &pgoff, &major, &minor, &ino, mfilename) < 7)) {
            plogger(log_error, "invalid format in /proc/$$/maps? %s\n", buf);
            continue;
        }

        tokstr = mfilename;
        while ((key = strsep(&tokstr, "/"))) {
            tmpkey = key;
        }

        if ((targpname && !strcmp(++targpname, tmpkey)) || !strcmp(filename.c_str(), tmpkey)) {
            continue;
        }

        if ('r' == flags[0] && 'x' == flags[2] && 0 == pgoff && '\0' != *mfilename) {
            abfd = bfd_openr(mfilename, NULL);
            if (nullptr == abfd) {
                plogger(log_warning, "bfd_openr '%s' failure!\n", mfilename);
                continue;
            }

            bfd_check_format(abfd, bfd_object);
            patch_read_symbols(abfd, vm_start, mfilename);
            bfd_close(abfd);
        }
    }

    abfd = bfd_openr(filename.c_str(), NULL);
    if (nullptr == abfd) {
        plogger(log_warning, "bfd_openr failure!\n");
        return -1;
    }
    bfd_check_format(abfd, bfd_object);
    patch_read_symbols(abfd, 0, filename.c_str());
    bfd_close(abfd);
    proc_map.close();

    targpid = pid;
    targname = filename;

    return 0;
}

int patch::patch_attach(void)
{
    if (ptrace(PTRACE_ATTACH, targpid, NULL, NULL) < 0) {
        plogger(log_error, "Attached to target '%d' failure!\n", targpid);
        return -1;
    }

    plogger(log_debug, "Attached to target '%d' success!\n", targpid);
    wait(NULL);

    return 0;
}

int patch::patch_detach(void)
{
    ptrace(PTRACE_DETACH, targpid, NULL, NULL);

    return 0;
}

int patch::patch_parse_command(std::string &command)
{
    int ret = 0;
    char targsrcsymbol[1024] = {0};
    char targdstsymbol[1024] = {0};
    char targdl[1024] = {0};
    bfd *abfd = NULL;

    if (!strncasecmp(command.c_str(), "jmp", 3)) {
        if (sscanf(command.c_str(), "jmp %s %s\n", targsrcsymbol, targdstsymbol) != 2) {
            return -1;
        }

        targsrcaddrinfo = patch_lookup_symaddr(targsrcsymbol);
        if (!targsrcaddrinfo) {
            plogger(log_debug, "symbol '%s' not found, targsrcaddrinfo[%p]\n", targsrcsymbol, (void *)targsrcaddrinfo);
            return -1;
        }

        targdstaddrinfo = patch_lookup_symaddr(targdstsymbol);
        if (!targdstaddrinfo) {
            plogger(log_debug, "symbol '%s' not found, targsrcaddrinfo[%p]\n", targsrcsymbol, (void *)targsrcaddrinfo);
            return -1;
        }

        targoffset = targdstaddrinfo - targsrcaddrinfo - 5;

        plogger(log_debug, "targsrcsymbol[%s], targdstsymbol[%s], targoffset[%d]\n", targsrcsymbol, targdstsymbol, targoffset);
    } else if (!strncasecmp(command.c_str(), "dl", 2)) {
        char *outbuf = NULL;
        int outsize = 0;
        struct user_regs_struct curregs = {0};
        char interrupt[] = {(char)0xcd, (char)0x80, (char)0xcc, (char)0x00};
        long value = 0;
        long raddr = 0;

        int dlopenaddr = patch_lookup_symaddr("__libc_dlopen_mode");
        plogger(log_notice, "dlopenaddr[%p]\n", dlopenaddr);

#if 0
        if (patch_read_registers(&curregs)) {
            plogger(log_error, "patch_read_registers failure!\n");
            return -1;
        }

        patch_backup_registers(&curregs);

        memcpy(&value, interrupt, sizeof(interrupt));
        if (patch_push_stack(&curregs, value)) {
            plogger(log_error, "patch_push_stack failure!\n");
            return -1;
        }

        curregs.eip = curregs.esp;
        raddr = curregs.esp + 2;

        for (int i = 0; i < (sizeof(mmap_args) / sizeof(mmap_args[0])); i++) {
            plogger(log_notice, "Set value[%p], args %d[%p]\n", value, i, mmap_args[i]);
            if (patch_push_stack(&curregs, mmap_args[i])) {
                plogger(log_error, "patch_push_stack failure!\n");
                return -1;
            }
        }

        if (patch_push_stack(&curregs, raddr)) {
            plogger(log_error, "patch_push_stack failure!\n");
            return -1;
        }

        curregs.ebx = curregs.esp + 4;
        curregs.eax = 90;

        if (patch_write_registers(&curregs)) {
            plogger(log_error, "patch_write_registers failure!\n");
            return -1;
        }

        if (patch_continue()) {
            plogger(log_error, "patch_continue failure!\n");
            return -1;
        }

        if (patch_read_registers(&curregs)) {
            plogger(log_error, "patch_read_registers failure!\n");
            return -1;
        }
        plogger(log_notice, "curregs.eax[%p], curregs.ebx[%p], MAP_FAILED[%p]\n", curregs.eax, curregs.ebx, MAP_FAILED);

        if (patch_restore_registers()) {
            plogger(log_error, "patch_restore_registers failure!\n");
            return -1;
        }
#endif

        if (sscanf(command.c_str(), "dl %s jmp %s %s\n", targdl, targsrcsymbol, targdstsymbol) != 3) {
            plogger(log_error, "targdl[%s], targsrcsymbol[%s], targdstsymbol[%s]\n", targdl, targsrcsymbol, targdstsymbol);
            return -1;
        }

        abfd = bfd_openr(targdl, NULL);
        if (NULL == abfd) {
            plogger(log_error, "bfd_openr failure!\n");
            return -1;
        }
        bfd_check_format(abfd, bfd_object);

        bfd_close(abfd);

        plogger(log_notice, "targdl[%s], targsrcsymbol[%s], targdstsymbol[%s]\n", targdl, targsrcsymbol, targdstsymbol);
    }

    return ret;
}

int patch::patch_lookup_symaddr(const char *symbol) const
{
    for (auto iter = symaddrs.begin(); iter != symaddrs.end(); iter++) {
        if (!strcmp((*iter)->symaddr_get_name()->c_str(), symbol)) {
            (*iter)->symaddr_show();
            return (*iter)->symaddr_get_addr();
        }
    }

    return 0;
}

patch::patch()
{
    bfd_init();
}

patch::~patch()
{}
