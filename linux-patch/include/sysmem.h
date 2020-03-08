#ifndef __SYSMEM_H__
#define __SYSMEM_H__

#include <sys/user.h>
#include <config.h>

class sysmem {
public:
    sysmem();
    ~sysmem();
    int32b_t read_regs(pid_t pidno, struct user_regs_struct *regs);
    int32b_t write_regs(pid_t pidno, struct user_regs_struct *regs);
    int32b_t attach(pid_t pidno) const;
    int32b_t detach(pid_t pidno) const;
    ulong_t get_curregs_espreg(void) const;
    int32b_t backup_bpregs(struct user_regs_struct *regs);
    int32b_t backup_curregs(struct user_regs_struct *regs);
    int32b_t restore_bpregs(pid_t pidno) const;
    int32b_t prcontinue(pid_t pidno) const;
    int32b_t push_data(pid_t pidno, const ulong_t memaddr, const ulong_t value);
    int32b_t push_data(pid_t pidno, struct user_regs_struct *regs, const ulong_t value);
    ulong_t push_data(pid_t pidno, struct user_regs_struct *regs, const int8b_t *value, ulong_t len);
    ulong_t read_mem_data(pid_t pidno, ulong_t memaddr);
private:
    struct user_regs_struct curregs;
    struct user_regs_struct bpregs;
};

#endif
