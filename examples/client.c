/* Simple client
 * =============
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
#include <string.h>
#include <tcp-tap/server.h>

#undef  NDEBUG
#include <assert.h>

#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

#define PORT_NUMBER 6688
#define HOST_IP "localhost"

int main(int argc, char **argv)
{
    int rn = BUFF_SZ, sn;
    int fd,s;
    char buf[BUFF_SZ];

    s = open_client(PORT_NUMBER, "localhost");

    while (1) {
        printf("Pid=%d client ready %s %d, socket %d\n",
               getpid(), HOST_IP, PORT_NUMBER, s);
        while (rn > 0) {
            //rn = read(0, buf, BUFF_SZ);
			memset(buf,0,BUFF_SZ);
			scanf("%s",buf);
			rn=strnlen(buf,BUFF_SZ);
			fprintf(stderr,"Writing to socket\n");
            sn = write(s, buf, rn);
			fprintf(stderr,"Reading fronm socket\n");
            rn = read(s, buf, rn);
            rn = write(1, buf, rn);
            assert(rn == sn);
        }
        if (rn < 0) {
            perror("read() failed: ");
            close(s);
            exit(errno);
        }
    }

    close(fd);
    return 0;
}
