#include <memory>
#include <list>
#include <fstream>
#include <patch.h>
#include <binelf.h>
#include <logger.h>

std::list<std::shared_ptr<symbol>> bfdelf::symaddrs;

symbol::symbol(const std::shared_ptr<std::string> &symbname, ulong_t memaddr) : symbname(symbname)
{
    this->memaddr = memaddr;
}

symbol::~symbol()
{}

const std::shared_ptr<std::string> &symbol::symbname_get(void) const
{
    return symbname;
}

ulong_t symbol::memaddr_get() const
{
    return memaddr;
}

bfdelf::bfdelf(const std::string &exename, const std::string &dynlib) : exename(exename), dynlib(dynlib)
{}

bfdelf::~bfdelf()
{}

int32b_t bfdelf::read_dynso_symbols(pid_t pidno)
{
    int8b_t buf[1024] = {0};
    bfd *abfd = nullptr;

    snprintf(buf, sizeof(buf) - 1, "/proc/%d/maps", pidno);
    std::ifstream procmaps(buf, std::ios::in);
    if (!procmaps.is_open()) {
        plogger(log_error, "%s open failure!\n", buf);
        return -1;
    }

    while (procmaps.getline(buf, sizeof(buf) - 1)) {
        int32b_t vm_start, vm_end, pgoff, major, minor, ino;
        int8b_t flags[5], mfilename[1024] = {0};
        int8b_t *key = nullptr, *tokstr = nullptr, *tmpkey = nullptr, *tmptokstr = nullptr;

        if ((sscanf(buf, "%x-%x %4s %x %d:%d %d %s", &vm_start, &vm_end,
            flags, &pgoff, &major, &minor, &ino, mfilename) < 7)) {
            plogger(log_error, "invalid format in /proc/$$/maps? %s\n", buf);
            continue;
        }

        if (dynlib.compare(std::string(mfilename))) {
            continue;
        }

        if ('r' == flags[0] && 'x' == flags[2] && 0 == pgoff && '\0' != *mfilename) {
            abfd = bfd_openr(mfilename, NULL);
            if (nullptr == abfd) {
                continue;
            }

            bfd_check_format(abfd, bfd_object);
            push_symbols(abfd, vm_start, mfilename);
            bfd_close(abfd);
        }
    }

    procmaps.close();

    return 0;
}

int32b_t bfdelf::readsymbols(pid_t pidno)
{
    int8b_t buf[1024] = {0};
    bfd *abfd = nullptr;
    int8b_t *pname = nullptr;
    int8b_t pos = 0;

    pos = exename.rfind("/");

    snprintf(buf, sizeof(buf) - 1, "/proc/%d/maps", pidno);
    std::ifstream procmaps(buf, std::ios::in);
    if (!procmaps.is_open()) {
        plogger(log_error, "%s open failure!\n", buf);
        return -1;
    }

    while (procmaps.getline(buf, sizeof(buf) - 1)) {
        int32b_t vm_start, vm_end, pgoff, major, minor, ino;
        int8b_t flags[5], mfilename[1024] = {0};
        int8b_t *key = nullptr, *tokstr = nullptr, *tmpkey = nullptr, *tmptokstr = nullptr;

        if ((sscanf(buf, "%x-%x %4s %x %d:%d %d %s", &vm_start, &vm_end,
            flags, &pgoff, &major, &minor, &ino, mfilename) < 7)) {
            plogger(log_error, "invalid format in /proc/$$/maps? %s\n", buf);
            continue;
        }

        tmptokstr = tokstr = strdup(mfilename);
        while ((key = strsep(&tokstr, "/"))) {
            tmpkey = key;
        }
        delete tmptokstr;
        tmptokstr = nullptr;

        if ((0 != pos) && !strcmp(exename.c_str() + pos + 1, tmpkey)) {
            continue;
        }

        if ((pname && !strcmp(++pname, tmpkey)) || !strcmp(exename.c_str(), tmpkey)) {
            continue;
        }

        if ('r' == flags[0] && 'x' == flags[2] && 0 == pgoff && '\0' != *mfilename) {
            abfd = bfd_openr(mfilename, NULL);
            if (nullptr == abfd) {
                continue;
            }

            bfd_check_format(abfd, bfd_object);
            push_symbols(abfd, vm_start, mfilename);
            bfd_close(abfd);
        }
    }
    procmaps.close();

    abfd = bfd_openr(exename.c_str(), NULL);
    if (nullptr == abfd) {
        plogger(log_warning, "bfd_openr failure!\n");
        return -1;
    }
    bfd_check_format(abfd, bfd_object);
    push_symbols(abfd, 0, exename.c_str());
    bfd_close(abfd);

    return 0;
}

int32b_t bfdelf::load_dynso(pid_t pidno)
{
    int32b_t retval = 0;
    struct user_regs_struct curregs = {0};
    ulong_t dlopenaddr = 0;
    ulong_t stack_top_ptr = 0;

    if (read_regs(pidno, &curregs)) {
        plogger(log_error, "read register failure!\n");
        return -1;
    }
    backup_bpregs(&curregs);
    backup_curregs(&curregs);

    //dlopenaddr = find_symbol(std::string("__libc_dlopen_mode"));
    dlopenaddr = find_symbol(std::string("link_dynamic_dlopen"));
    if (!dlopenaddr) {
        plogger(log_error, "find symbol failure!\n");
        return -1;
    }
    plogger(log_debug, "dlopenaddr[%p], dynlib.c_str()[%s]\n", dlopenaddr, dynlib.c_str());

    stack_top_ptr = push_data(pidno, &curregs, dynlib.c_str(), dynlib.size() + 1);
    if (ulong_mask_t == stack_top_ptr) {
        plogger(log_error, "push_data failure!\n");
        return -1;
    }

    if (push_data(pidno, &curregs, RTLD_LAZY | RTLD_NOW)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

    if (push_data(pidno, &curregs, stack_top_ptr)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

    if (push_data(pidno, &curregs, 0xcc00)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

#if defined(__linux__) && defined(__i386__)
    curregs.eip = dlopenaddr;
#elif defined(__linux__) && defined(__x86_64__)
#elif defined(__linux__) && defined(__arm__)
#elif defined(__linux__) && defined(__aarch64__)
#endif

    if (write_regs(pidno, &curregs)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

    if (read_regs(pidno, &curregs)) {
        plogger(log_error, "read register failure!\n");
        retval = -1;
        goto finished;
    }

    if (prcontinue(pidno)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

    if (read_regs(pidno, &curregs)) {
        plogger(log_error, "read register failure!\n");
        retval = -1;
        goto finished;
    }

#if defined(__linux__) && defined(__i386__)
    handle = curregs.eax;
#elif defined(__linux__) && defined(__x86_64__)
#elif defined(__linux__) && defined(__arm__)
#elif defined(__linux__) && defined(__aarch64__)
#endif

finished:
    if (restore_bpregs(pidno)) {
        plogger(log_error, "restore_bpregs failure!\n");
        retval = -1;
    }

    return retval;
}

int32b_t bfdelf::unload_dynso(pid_t pidno)
{
    ulong_t dlcloseaddr = 0;
    struct user_regs_struct curregs;
    int32b_t retval = 0;

    if (read_regs(pidno, &curregs)) {
        plogger(log_error, "read register failure!\n");
        return -1;
    }
    backup_bpregs(&curregs);
    backup_curregs(&curregs);

    dlcloseaddr = find_symbol(std::string("__libc_dlclose"));
    if (!dlcloseaddr) {
        plogger(log_error, "find symbol failure!\n");
        return -1;
    }
    plogger(log_debug, "dlcloseaddr[%p], handle[%p]\n", dlcloseaddr, handle);

    if (push_data(pidno, &curregs, handle)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

    if (push_data(pidno, &curregs, 0xcc000000)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

#if defined(__linux__) && defined(__i386__)
    curregs.eip = dlcloseaddr;
#elif defined(__linux__) && defined(__x86_64__)
#elif defined(__linux__) && defined(__arm__)
#elif defined(__linux__) && defined(__aarch64__)
#endif

    if (write_regs(pidno, &curregs)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

    if (read_regs(pidno, &curregs)) {
        plogger(log_error, "read register failure!\n");
        retval = -1;
        goto finished;
    }

    if (prcontinue(pidno)) {
        plogger(log_error, "push_data failure!\n");
        retval = -1;
        goto finished;
    }

    if (read_regs(pidno, &curregs)) {
        plogger(log_error, "read register failure!\n");
        retval = -1;
        goto finished;
    }

finished:
    if (restore_bpregs(pidno)) {
        plogger(log_error, "restore_bpregs failure!\n");
        retval = -1;
    }

    return retval;

    return 0;
}

ulong_t bfdelf::find_symbol(const std::string &src) const
{
    for (auto iter = symaddrs.begin(); iter != symaddrs.end(); iter++) {
        if (!(*iter)->symbname_get()->compare(src)) {
            return (*iter)->memaddr_get();
        }
    }

    return ulong_mask_t;
}

int32b_t bfdelf::push_symbols(bfd *abfd, int32b_t offset, const int8b_t *filename)
{
    long_t storage_needed = 0;
    int32b_t retval = 0;
    asymbol **symbol_table = nullptr;
    long_t number_of_symbols = 0;

    storage_needed = bfd_get_symtab_upper_bound(abfd);
    if (storage_needed < 0) {
        plogger(log_warning, "bfd_get_symtab_upper_bound failure!\n");
        retval = -1;
        goto dynsym;
    }

    if (0 == storage_needed) {
        plogger(log_warning, "no symbols!\n");
        retval = -1;
        goto dynsym;
    }

    symbol_table = new asymbol*[storage_needed];
    number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
    if (number_of_symbols < 0) {
        retval = -1;
        plogger(log_warning, "bfd_canonicalize_symtab failure!\n");
        goto dynsym;
    }

    for (long_t i = 0; i < number_of_symbols; i++) {
        asymbol *asym = symbol_table[i];
        const int8b_t *sym_name = bfd_asymbol_name(asym);
        int32b_t symclass = bfd_decode_symclass(asym);
        int32b_t sym_value = bfd_asymbol_value(asym) + offset;

        if ('\0' == *sym_name) {
            continue;
        }

        if (bfd_is_undefined_symclass(symclass)) {
            continue;
        }
        //plogger(log_debug, "[%s] start addr[%p], %s = %p\n", filename, offset, sym_name, sym_value);
        symaddrs.push_back(std::shared_ptr<symbol>(new symbol(std::shared_ptr<std::string>(new std::string(sym_name)), sym_value)));
    }
    retval = 0;

dynsym:
    if (symbol_table) {
        delete []symbol_table;
        symbol_table = nullptr;
    }

    storage_needed = bfd_get_dynamic_symtab_upper_bound(abfd);
    if (storage_needed < 0) {
        plogger(log_warning, "bfd_get_dynamic_symtab_upper_bound failure!\n");
        retval = -1;
        goto out;
    }
    if (storage_needed == 0) {
        retval = -1;
        plogger(log_warning, "no symbols!\n");
        goto out;
    }

    symbol_table = new asymbol*[storage_needed];
    number_of_symbols = bfd_canonicalize_dynamic_symtab(abfd, symbol_table);
    if (number_of_symbols < 0) {
        retval = -1;
        plogger(log_warning, "bfd_canonicalize_symtab failure!\n");
        goto out;
    }

    for (long_t i = 0; i < number_of_symbols; i++) {
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

        //plogger(log_debug, "[%s] start addr[%p], %s = %p\n", filename, offset, sym_name, sym_value);
        symaddrs.push_back(std::shared_ptr<symbol>(new symbol(std::shared_ptr<std::string>(new std::string(sym_name)), sym_value)));
    }
    retval = 0;

out:
    if (symbol_table) {
        delete []symbol_table;
        symbol_table = nullptr;
    }

    return retval;
}
