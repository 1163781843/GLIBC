#include <logger.h>
#include <patch.h>
#include <config.h>

#include <iostream>
#include <string>
#include <memory>

static void_t usage_help(int8b_t *prog)
{
    fprintf(stdout, "Usage:\n"
        "    %s -p <target-process-pid> -e <target-process-binary-file> -c <asm instruction> [-d dynamic path]\n\n"
        "    asm instruction: jmp src_func_addr dst_func_addr\n\n", prog);
}

int32b_t main(int32b_t argc, int8b_t * const *argv)
{
    int32b_t retval = 0;
    int8b_t c = 0;
    uint32b_t flags = 0;
    pid_t pidno = -1;
    std::string exename;
    std::string dynlib;
    std::string command;

    while (-1 != (c = getopt(argc, argv, "p:e:d:c:"))) {
        switch (c) {
        case 'p':
            pidno = atoi(optarg);
            if (pidno <= 0) {
                plogger(log_warning, "Invalid pid[%d], please check it!\n", pidno);
                return -1;
            }
            flags |= patch::patch_targpid;
            break;
        case 'e':
            exename = std::string(optarg);
            flags |= patch::patch_targbin;
            break;
        case 'd':
            dynlib = std::string(optarg);
            flags |= patch::patch_dynlib;
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
        usage_help(argv[0]);
        return -1;
    }

    std::unique_ptr<patch> init(new patch(pidno, exename, command, dynlib));

    if (init->readsymbols(pidno)) {
        plogger(log_error, "readsymbols failure!\n");
        return -1;
    }

    if (init->attach(pidno)) {
        plogger(log_error, "attach failure!\n");
        return -1;
    }

    if ((flags & patch::patch_dynlib) && init->patch_load_dynso()) {
        retval = -1;
        plogger(log_error, "patch_load_dynso failure!\n");
        goto finished;
    }

    if (init->patch_parse_cmd()) {
        retval = -1;
        plogger(log_error, "patch_parse_cmd failure!\n");
        goto finished;
    }

    if (init->patch_jump_inject()) {
        retval = -1;
        plogger(log_error, "patch_parse_cmd failure!\n");
        goto finished;
    }

    init->detach(pidno);

    return retval;

finished:
    init->unload_dynso(pidno);

    init->detach(pidno);

    return retval;
}
