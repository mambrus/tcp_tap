/* Simple rsh example
 * ==================
 *
 * (Or anything else given as argument(s).)
 *
 * Use client that sends stdin exactly as is, telnet is not recommended.
 *
 * Depending on backend terminal characters will matter (cooked/raw stream).
 * GDB will handle either, but bash will not.
 *
 * Example of client use:
 *    nc localhost 1974
 * */
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <tcp-tap/clientserver.h>
#undef  NDEBUG
#include <liblog/assure.h>

#define PORT_NUMBER 1974
#define MAX_ARGS 50

/* Name of the main process to run */
char execute_bin[PATH_MAX] = "/bin/bash";

int main(int argc, char **argv)
{
    int i, childpid = 0, wpid, status;
    char *exec_args[MAX_ARGS] = {NULL};

	 ASSERT(argc < MAX_ARGS);

    exec_args[0] = execute_bin;
    for (i = 1; i < argc; i++) {
        exec_args[i] = argv[i];
    }

    ASSERT((childpid = fork()) >= 0);

    if (childpid == 0) {
        /* Child executes this */
        int s, fd;

        s = init_server(PORT_NUMBER, "@ANY@");
        fprintf(stderr, "Console service started at socket[port]: %d[%d]\n", s,
                PORT_NUMBER);

        while (1) {
            fd = open_server(s);
            fprintf(stderr,
                    "Servicing stdin/stdout at fd:%d for binary: %s\n", fd,
                    execute_bin);
            close(0);
            dup(fd);
            close(1);
            dup(fd);

            execv(execute_bin, exec_args);
        }

        /* Should never execute */
        perror("exec error");
        exit(-1);
    }

    /* Parent executes this */
    do {
        wpid = waitpid(childpid, &status, WUNTRACED);
        ASSERT(wpid >= 0);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    return status;
}
