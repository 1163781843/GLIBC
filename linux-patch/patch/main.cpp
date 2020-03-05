#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#include <bfd.h>

#include <logger.h>
#include <patch.h>

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

    if (init->patch_attach()) {
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
    init->patch_detach();

    return ret;
}

