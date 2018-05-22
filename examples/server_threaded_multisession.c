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
#include <tcp-tap/server.h>

#undef  NDEBUG
#include <assert.h>

#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

#define PORT_NUMBER 6666
#define HOST_IP "localhost"

/* Handle session: Just echo back everything */
void *myThread(void *inarg)
{
    int rn, sn;
    int fd = (int)inarg;
    char buf[BUFF_SZ];
    while (1) {
        rn = read(fd, buf, BUFF_SZ);
        sn = write(fd, buf, rn);
        assert(rn == sn);
    }
}

int main(int argc, char **argv)
{
    int fd, s;
    pthread_t t_thread;

    s = init_server(PORT_NUMBER, "localhost");

    printf("Ready for multi-session echoing service: telnet %s %d\n", HOST_IP,
           PORT_NUMBER);
    while (1) {
        fd = open_server(s);
        assert(pthread_create(&t_thread, NULL, myThread, (void *)fd) == 0);
    }

    return 0;
}
