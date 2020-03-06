#include <sysmem.h>
#include <logger.h>
#include <config.h>

sysmem::sysmem()
{}

sysmem::~sysmem()
{}

int32b_t sysmem::read_regs(pid_t pidno, struct user_regs_struct *regs)
{
    if (ptrace(PTRACE_GETREGS, pidno, NULL, regs) < 0) {
        plogger(log_error, "PTRACE_GETREGS failure!\n");
        return -1;
    }

#if defined(__linux__) && defined(__i386__)
    plogger(log_notice, "regs->eip[%p], regs->esp[%p], regs->ebx[%p], regs->eax[%p]\n",
        regs->eip, regs->esp, regs->ebx, regs->eax);
#elif defined(__linux__) && defined(__x86_64__)
#elif defined(__linux__) && defined(__arm__)
#elif defined(__linux__) && defined(__aarch64__)
#endif

    return 0;

}

int32b_t sysmem::write_regs(pid_t pidno, struct user_regs_struct *regs)
{
    if (ptrace(PTRACE_SETREGS, pidno, NULL, regs)) {
        plogger(log_error, "PRTACE_SETREGS failure!\n");
        return -1;
    }

    return 0;
}

int32b_t sysmem::attach(pid_t pidno) const
{
    if (ptrace(PTRACE_ATTACH, pidno, NULL, NULL) < 0) {
        plogger(log_error, "Attached to target '%d' failure!\n", pidno);
        return -1;
    }
    plogger(log_notice, "attach '%d' process success!\n", pidno);

    return 0;
}

int32b_t sysmem::detach(pid_t pidno) const
{
    ptrace(PTRACE_DETACH, pidno, NULL, NULL);

    plogger(log_notice, "detach '%d' process success!\n", pidno);

    return 0;
}

ulong_t sysmem::get_curregs_espreg(void) const
{
#if defined(__linux__) && defined(__i386__)
    return curregs.esp;
#elif defined(__linux__) && defined(__x86_64__)
#elif defined(__linux__) && defined(__arm__)
#elif defined(__linux__) && defined(__aarch64__)
#endif
}

int32b_t sysmem::backup_bpregs(struct user_regs_struct *regs)
{
    memcpy(&bpregs, regs, sizeof(bpregs));

    return 0;
}

int32b_t sysmem::backup_curregs(struct user_regs_struct *regs)
{
    memcpy(&curregs, regs, sizeof(curregs));

    return 0;
}

int32b_t sysmem::restore_bpregs(pid_t pidno) const
{
    if (ptrace(PTRACE_SETREGS, pidno, pidno, &bpregs)) {
        plogger(log_error, "PTRACE_SETREGS failure!\n");
        return -1;
    }

    return 0;
}

int32b_t sysmem::prcontinue(pid_t pidno) const
{
    if (ptrace(PTRACE_CONT, pidno, NULL, NULL)) {
        plogger(log_error, "PTRACE_CONT failure!\n");
        return -1;
    }

    wait(NULL);

    return 0;
}

int32b_t sysmem::push_data(pid_t pidno, struct user_regs_struct *regs, const ulong_t value)
{
    ulong_t stack_top_ptr = ulong_mask_t;

#if defined(__linux__) && defined(__i386__)
    stack_top_ptr = regs->esp -= sizeof(ulong_t);
#elif defined(__linux__) && defined(__x86_64__)
#elif defined(__linux__) && defined(__arm__)
#elif defined(__linux__) && defined(__aarch64__)
#endif

    if (ptrace(PTRACE_POKEDATA, pidno, stack_top_ptr, value)) {
        plogger(log_error, "PTRACE_POKEDATA failure!\n");
        return -1;
    }

    return 0;
}

ulong_t sysmem::push_data(pid_t pidno, struct user_regs_struct *regs, const int8b_t *value, ulong_t len)
{
    ulong_t stack_top_ptr = ulong_mask_t;
    ulong_t word = 0;

#if defined(__linux__) && defined(__i386__)
    stack_top_ptr = regs->esp;
#elif defined(__linux__) && defined(__x86_64__)
#elif defined(__linux__) && defined(__arm__)
#elif defined(__linux__) && defined(__aarch64__)
#endif
    stack_top_ptr -= len;
    stack_top_ptr -= stack_top_ptr % sizeof(ulong_t);

#if defined(__linux__) && defined(__i386__)
    regs->esp = stack_top_ptr;
#elif defined(__linux__) && defined(__x86_64__)
#elif defined(__linux__) && defined(__arm__)
#elif defined(__linux__) && defined(__aarch64__)
#endif

    for (ulong_t i = 0; i < len; i += sizeof(ulong_t)) {
        memcpy(&word, value + i, sizeof(ulong_t));

        if (ptrace(PTRACE_POKETEXT, pidno, stack_top_ptr + i, word)) {
            plogger(log_error, "PTRACE_POKEDATA failure!\n");
            return ulong_mask_t;
        }
    }

    return stack_top_ptr;
}
