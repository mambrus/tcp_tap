/* Echo service (multisession threaded)
 * ====================================
 *
 * Wait for connect, then spawn a thread for each connection who will handle
 * that particular session, then go back wait for more connections.
 *
 * Any TCP-client can be used.
 *
 * Example of client-side usage:
 *
 *   telnet localhost 6666
 */
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <tcp-tap/clientserver.h>

#undef  NDEBUG
#include <liblog/assure.h>

#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

#define PORT_NUMBER 6666
#define HOST_IP "localhost"

/* Handle session: Just echo back everything */
void *myThread(void *inarg)
{
    int rn = BUFF_SZ, sn, tid;
    int fd = (intptr_t) inarg;
    char buf[BUFF_SZ];
    static int nt = 0;

    nt++;
    tid = nt;

    printf("Thread [%d] is handling session for fd [%d]\n", tid, fd);
    while (rn > 0) {
        rn = read(fd, buf, BUFF_SZ);
        printf(" %d: %d\n", tid, rn);
        sn = write(fd, buf, rn);
        ASSERT(rn == sn);
    }
    printf("Thread [%d] finished session for fd [%d]\n", tid, fd);
    if (rn < 0)
        perror("read() failed: ");

    close(fd);

    return NULL;
}

int main(int argc, char **argv)
{
    int fd, s;
    pthread_t t_thread;

    s = init_server(PORT_NUMBER, "localhost");

    while (1) {
        printf("Pid=%d ready for multi-session echoing service: telnet %s %d\n",
               getpid(), HOST_IP, PORT_NUMBER);

        fd = open_server(s);
        ASSERT(pthread_create
               (&t_thread, NULL, myThread, (void *)((intptr_t) fd)) == 0);
    }

    return 0;
}
