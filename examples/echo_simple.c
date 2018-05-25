/* Echo service (single session)
 * =============================
 *
 * Wait for connect
 * Handle session until session terminates
 * Repeat
 *
 * Any TCP-client can be used.
 *
 * Example of client-side usage:
 *
 *   telnet localhost 6688
 */
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <tcp-tap/clientserver.h>

#undef  NDEBUG
#include <assert.h>

#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

#define PORT_NUMBER 6688
#define HOST_IP "localhost"

/* Handle session: Just echo back everything */
int main(int argc, char **argv)
{
    int rn = BUFF_SZ, sn;
    int fd, s;
    char buf[BUFF_SZ];

    s = init_server(PORT_NUMBER, "localhost");

    while (1) {
        printf("Pid=%d ready for echoing service: telnet %s %d\n",
               getpid(), HOST_IP, PORT_NUMBER);
        fd = open_server(s);
        while (rn > 0) {
            rn = read(fd, buf, BUFF_SZ);
            sn = write(1, buf, rn);
            assert(rn == sn);
            sn = write(fd, buf, rn);
            assert(rn == sn);
        }
        if (rn < 0) {
            perror("read() failed: ");
            close(fd);
            exit(errno);
        }
    }

    close(fd);
    return 0;
}
