/* Simple AF_UNIX, DGRAM p2p chat
 * ==============================
 *
 * Program takes 2 argument, our_name & rmt_name. Same program is used in
 * both ends.
 *
 * To chat, start 2 processes, Reverse the arguments for the second instance
 *
 * ex:
 *
 * ./local_domain first second
 * ./local_domain second first
 *
 */
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <tcp-tap/clientserver.h>

#undef  NDEBUG
#include <liblog/assure.h>

#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

/* Handle incoming session: Just print to stdout */
void *servletThread(void *inarg)
{
    int rn = BUFF_SZ, sn, tid;
    int fd;
    char buf[BUFF_SZ];
    static int nt = 0;

    nt++;
    tid = nt;

    fd = named_socket(1, inarg);

    fprintf(stderr, "Thread [%d] is handling session for fd [%d]\n", tid, fd);
    while (rn > 0) {
        rn = read(fd, buf, BUFF_SZ);
        sn = write(1, buf, rn);
        ASSERT(rn == sn);
    }
    fprintf(stderr, "Thread [%d] finished session for fd [%d]\n", tid, fd);
    if (rn < 0)
        perror("read() failed: ");

    close(fd);

    return NULL;
}

int main(int argc, char **argv)
{
    int rn = BUFF_SZ, sn, connected;
    int s;
    char buf[BUFF_SZ];
    char *our_name, *rmt_name;
    pthread_t t_thread;

    if (argc != 3) {
        fprintf(stderr, "[%s] needs two arguments: our_name & rmt_name\n",
                argv[0]);
        fflush(stderr);
        exit(1);
    }

    our_name = argv[1];
    rmt_name = argv[2];

    ASSERT(pthread_create(&t_thread, NULL, servletThread, our_name) == 0);

    while (1) {
        printf("Pid=%d client ready %s, our_name=%s, rmt_name=%s\n",
               getpid(), argv[0], our_name, rmt_name);
        for (rn = BUFF_SZ, connected = 0; rn > 0;) {
            fprintf(stderr, "Reading from stdin\n");
            rn = read(0, buf, BUFF_SZ);

            /* Connection postponed to here. Both processes need to be
             * listening (i.e. bind()) first. Connect only once however. */
            if (!connected) {
                s = named_socket(0, rmt_name);
                connected = 1;
            }

            fprintf(stderr, "Writing to socket\n");
            sn = write(s, buf, rn);
            ASSERT(rn == sn);
        }
        if (rn < 0) {
            perror("read() failed: ");
            exit(errno);
        }
        close(s);
    }

    return 0;
}
