/* Simple AF_UNIX, DGRAM write
 * ============================
 *
 * Program takes 1 argument, send_name
 *
 * ex:
 *
 * ./local_domain_write send_name
 *
 */
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <tcp-tap/clientserver.h>

#undef  NDEBUG
#include <liblog/assure.h>

#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

int main(int argc, char **argv)
{
    int rn = BUFF_SZ, sn;
    int s;
    char buf[BUFF_SZ];

    if (argc != 2) {
        fprintf(stderr, "[%s] needs 1 argument: send_name\n", argv[0]);
        fflush(stderr);
        exit(1);
    }

    s = named_socket(0, argv[1]);

    while (1) {
        printf("Pid=%d client ready %s, socket %d\n", getpid(), argv[1], s);
        while (rn > 0) {
            fprintf(stderr, "Reading from stdin\n");
            rn = read(0, buf, BUFF_SZ);
            fprintf(stderr, "Writing to socket\n");
            sn = write(s, buf, rn);
            ASSERT(rn == sn);
        }
        if (rn < 0) {
            perror("read() failed: ");
            close(s);
            exit(errno);
        }
    }

    close(s);
    return 0;
}
