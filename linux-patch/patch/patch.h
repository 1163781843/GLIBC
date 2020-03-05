#ifndef __PATCH_H__
#define __PATCH_H__

#include <sys/user.h>
#include <list>

class symaddr {
public:
    symaddr(std::string    *name, int addr);
    ~symaddr();

    std::string *symaddr_get_name(void) const;
    int symaddr_get_addr(void) const;
    void symaddr_show(void) const;
private:
    std::string *name;
    int addr;
};

class patch {
public:
    enum {
        patch_targpid = 0 << 1,
        patch_targbin = 1 << 1,
        patch_command = 1 << 2,
        patch_total   = patch_targpid | patch_targbin | patch_command,
    };

    patch();
    ~patch();

    int patch_symbol_init(pid_t pid, std::string &filenames);
    int patch_read_symbols(bfd *abfd, int offset, const char *filenames);
    int patch_attach(void);
    int patch_detach(void);
    int patch_parse_command(std::string &command);
    int patch_lookup_symaddr(const char *symbol) const;
    int patch_set_data(void);
    int patch_read_registers(struct user_regs_struct *registers);
    int patch_write_registers(struct user_regs_struct *registers);
    void patch_backup_registers(const struct user_regs_struct *registers);
    int patch_restore_registers(void) const;
    int patch_push_stack(struct user_regs_struct *registers, long value);
    int patch_continue(void) const;
private:
    std::list<symaddr *> symaddrs;
    pid_t targpid;
    std::string targname;
    int targsrcaddrinfo;
    int targdstaddrinfo;
    int targoffset;
    struct user_regs_struct orgregs;
};

#endif
