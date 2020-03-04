#ifndef __PATCH_H__
#define __PATCH_H__

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
    int patch_attach_targprocess(void);
    int patch_detach_targprocess(void);
    int patch_parse_command(std::string &command);
    int patch_lookup_symaddr(const char *symbol);
    int patch_set_data(void);
    void *patch_get_target_addr(pid_t pid, const char *modname, void *localaddr);
private:
    void *patch_get_modbase_address(pid_t pid, const char *modname);
    std::list<symaddr *> symaddrs;
    pid_t targpid;
    std::string targname;
    int targsrcaddrinfo;
    int targdstaddrinfo;
    int targoffset;
};

#endif
