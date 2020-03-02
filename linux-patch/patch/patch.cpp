#include <iostream>
#include <memory>
#include <fstream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>

#include <bfd.h>

#include <patch.h>
#include <logger.h>

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
    plogger(log_debug, "%s = %p[%p]\n", name->c_str(), (void *)addr, name);
}

symaddr::symaddr(std::string *name, int addr) : name(name)
{
    //plogger(log_debug, "%s = %p[%p]\n", name->c_str(), (void *)addr, name);
    this->addr = addr;
}

symaddr::~symaddr()
{}

int patch::patch_read_symbols(bfd *abfd, int offset)
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
        //plogger(log_debug, "%s = %p\n", sym_name, sym_value);
        symaddrs.push_back(new symaddr(new std::string(sym_name), sym_value));
    }
    //plogger(log_debug, "storage_needed[%d], number_of_symbols[%d]\n", storage_needed, number_of_symbols);

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

        //plogger(log_debug, "%s = %p\n", sym_name, sym_value);
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

    plogger(log_debug, "target symbol init, pid[%d] filename[%s]\n", pid, filename.c_str());

    snprintf(buf, sizeof(buf), "/proc/%d/maps", pid);

    std::ifstream proc_map(buf, std::ios::in);
    if (!proc_map.is_open()) {
        plogger(log_error, "%s open failure!\n", buf);
    }

    while (proc_map.getline(buf, sizeof(buf) - 1)) {
        int vm_start, vm_end, pgoff, major, minor, ino;
        char flags[5], mfilename[4096] = {0};
        if ((sscanf(buf, "%x-%x %4s %x %d:%d %d %s", &vm_start, &vm_end,
            flags, &pgoff, &major, &minor, &ino, mfilename) < 7)) {
            plogger(log_error, "invalid format in /proc/$$/maps? %s\n", buf);
            continue;
        }
        //plogger(log_debug, "0x%x-0x%x %s 0x%x %s\n", vm_start, vm_end, flags, pgoff, mfilename);

        if ('r' == flags[0] && 'x' == flags[2] && 0 == pgoff && '\0' != *mfilename) {
            //plogger(log_debug, "file %s @ %p\n", mfilename, vm_start);
            abfd = bfd_openr(mfilename, NULL);
            if (nullptr == abfd) {
                plogger(log_warning, "bfd_openr '%s' failure!\n", mfilename);
                continue;
            }

            bfd_check_format(abfd, bfd_object);
            patch_read_symbols(abfd, vm_start);
            bfd_close(abfd);
        }
    }

    abfd = bfd_openr(filename.c_str(), NULL);
    if (nullptr == abfd) {
        plogger(log_warning, "bfd_openr failure!\n");
        return -1;
    }
    bfd_check_format(abfd, bfd_object);
    patch_read_symbols(abfd, 0);
    bfd_close(abfd);
    proc_map.close();

    targpid = pid;
    targname = filename;

    return 0;
}

int patch::patch_attach_targprocess(void)
{
    if (ptrace(PTRACE_ATTACH, targpid, NULL, NULL) < 0) {
        plogger(log_error, "Attached to target '%d' failure!\n", targpid);
        return -1;
    }

    plogger(log_debug, "Attached to target '%d' success!\n", targpid);
    wait(NULL);

    return 0;
}

int patch::patch_detach_targprocess(void)
{
    ptrace(PTRACE_DETACH, targpid, NULL, NULL);

    return 0;
}

int patch::patch_parse_command(std::string &command)
{
    int ret = 0;
    char targsrcsymbol[1024] = {0};
    char targdstsymbol[1024] = {0};

    if (strstr(command.c_str(), "jmp")) {
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
    } else if (strstr(command.c_str(), "dl")) {
        return -1;
    }

    return ret;
}

int patch::patch_lookup_symaddr(const char *symbol)
{
    for (auto iter = symaddrs.begin(); iter != symaddrs.end(); iter++) {
        if (!strcmp((*iter)->symaddr_get_name()->c_str(), symbol)) {
            (*iter)->symaddr_show();
            return (*iter)->symaddr_get_addr();
        }
    }

    return 0;
}

int patch::patch_set_data(void)
{
    char jmpbuf[5] = {0};
#if 0
    int addr0 = 0;
    int len = 0;
    int *lv = NULL;
#endif

    jmpbuf[0] = 0xe9;
    memcpy(&jmpbuf[1], &targoffset, sizeof(int));

    if (set_data(targpid, targsrcaddrinfo, jmpbuf, sizeof(jmpbuf))) {
        return -1;
    }

#if 0
    addr0 = targsrcaddrinfo & (~3);
    len = (((targsrcaddrinfo + sizeof(jmpbuf)) - addr0) + 3) / 4;
    lv = new int[sizeof(jmpbuf) * sizeof(int)];

    for (int i = 0; i < len; i++) {
        lv[i] = ptrace(PTRACE_PEEKDATA, targpid, addr0 + i * sizeof(int), NULL);
        if (-1 == lv[i] && 0 != errno) {
            plogger(log_error, "ptrace PTRACE_PEEKDATA failure, targpid[%d]!\n", targpid);
            return -1;
        }
    }

    memcpy((char *)lv + (targsrcaddrinfo - addr0), jmpbuf, sizeof(jmpbuf));

    for (int i = 0; i < len; i++) {
        lv[i] = ptrace(PTRACE_PEEKDATA, targpid, addr0 + i * sizeof(int), NULL);
        if (-1 == lv[i] && 0 != errno) {
            plogger(log_error, "ptrace PTRACE_PEEKDATA failure!\n");
            return -1;
        }
    }
#endif

    return 0;
}

int patch::set_data(pid_t pid, int addr, void *val, int vlen)
{
    int i;
    int addr0 = addr & ~3;
    int len = (((addr + vlen) - addr0) + 3)/4;
    int *lv = (int *)malloc(len * sizeof(int));

    for (i = 0; i < len; i++) {
        lv[i] = ptrace(PTRACE_PEEKDATA, pid, addr0 + i * sizeof(int), NULL);
        if (lv[i] == -1 && errno != 0) {
            perror("ptrace peek");
            return -1;
        }
    }

    memcpy((char *)lv + (addr - addr0), val, vlen);
    for (i = 0; i < len; i++) {
        if (ptrace(PTRACE_POKEDATA, pid, addr0 + i * sizeof(int), lv[i]) < 0) {
            perror("ptrace poke");
            return -1;
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

static void patch_usage(char *prog)
{
    fprintf(stdout, "Usage:\n"
        "\t %s -p <target-process-pid> -s <target-process-binary-file> -c <command> [-d dl]\n\n"
        "\t dl: Represent dynamic library\n\n", prog);
}

int main(int argc, char * const *argv)
{
    int flags = 0;
    int c;
    pid_t targpid = - 1;
    std::string targname;
    std::string destdl;
    std::string command;
    int ret = 0;

    while (-1 != (c = getopt(argc, argv, "p:s:d::c:"))) {
        switch (c) {
        case 'p':
            targpid = atoi(optarg);
            if (targpid <= 0) {
                plogger(log_warning, "Invalid pid[%d], please check it!\n", targpid);
                return -1;
            }
            flags |= patch::patch_targpid;
            break;
        case 's':
            targname = std::string(optarg);
            flags |= patch::patch_targbin;
            break;
        case 'd':
            destdl = std::string(optarg);
            break;
        case 'c':
            command = std::string(optarg);
            flags |= patch::patch_command;
            break;
        default:
            break;
        }
    }

    if (!(flags & patch::patch_total)) {
        patch_usage(argv[0]);
        return -1;
    }

    std::unique_ptr<patch> init(new patch);

    if (init->patch_symbol_init(targpid, targname)) {
        plogger(log_error, "patch symbol init failure!\n");
        return -1;
    }

    if (init->patch_attach_targprocess()) {
        plogger(log_error, "patch attach target process failure!\n");
        return -1;
    }

    if (init->patch_parse_command(command)) {
        plogger(log_error, "patch parse command failure!\n");
        ret = -1;
        goto finished;
    }

    if (init->patch_set_data()) {
        plogger(log_error, "patch set memory data failure!\n");
        ret = -1;
        goto finished;
    }

finished:
    init->patch_detach_targprocess();

    return ret;
}
