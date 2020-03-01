#include <iostream>
#include <memory>
#include <fstream>

#include <bfd.h>

#include <patch.h>
#include <logger.h>

symaddr::symaddr(std::string *name, int addr) : name(name)
{}

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
        plogger(log_debug, "%s = %p\n", sym_name, sym_value);
        symaddrs.push(new symaddr(new std::string(sym_name), sym_value));
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

        plogger(log_debug, "%s = %p\n", sym_name, sym_value);
        symaddrs.push(new symaddr(new std::string(sym_name), sym_value));
    }

out:
    if (symbol_table) {
        delete []symbol_table;
        symbol_table = nullptr;
    }

    return ret;
}

int patch::patch_symbol_init(pid_t pid, const char *filename)
{
    char buf[4096] = {0};
    bfd *abfd = NULL;

    plogger(log_debug, "target symbol init, pid[%d] filename[%s]\n", pid, filename);

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

    abfd = bfd_openr(filename, NULL);
    if (nullptr == abfd) {
        plogger(log_warning, "bfd_openr failure!\n");
        return -1;
    }
    bfd_check_format(abfd, bfd_object);
    patch_read_symbols(abfd, 0);
    bfd_close(abfd);

    proc_map.close();

    return 0;
}

patch::patch()
{
    bfd_init();
}

patch::~patch()
{}

int main(int argc, const char **argv)
{
    std::unique_ptr<patch> init(new patch);

    init->patch_symbol_init(16570, "/opt/asterisk/sbin/asterisk");

    return 0;
}
