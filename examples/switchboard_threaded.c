/* Threaded chat server
 * ====================
 *
 * Shich-board shuffles all from all - to all
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tcp-tap/switchboard.h>
#include <tcp-tap/server.h>
#include "config.h"

#undef  NDEBUG
#include <assert.h>

/* The size of each buffer used for transfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

#define PORT_NUMBER 1974
#define HOST_IP "localhost"

void *from_stdin(void *arg)
{
    int rn = BUFF_SZ, sn;
    int wfd;
    char buf[BUFF_SZ];

    usleep(100000);
    assert((wfd = open(Q_TO_SWTCH, O_WRONLY)) >= 0);

    while (rn > 0) {
        rn = read(1, buf, BUFF_SZ);
        sn = write(wfd, buf, rn);
        assert(rn == sn);
    }

    if (rn < 0)
        perror("read() failed: ");
    close(wfd);
    return NULL;
}

void *to_stdout(void *arg)
{
    int rn = BUFF_SZ, sn;
    int rfd;
    char buf[BUFF_SZ];

    usleep(100000);
    assert((rfd = open(Q_FROM_SWTCH, O_RDONLY)) >= 0);

    while (rn > 0) {
        rn = read(rfd, buf, BUFF_SZ);
        sn = write(2, buf, rn);
        assert(rn == sn);
    }

    if (rn < 0)
        perror("read() failed: ");

    close(rfd);
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t thread1, thread2;

    assert(pthread_create(&thread1, NULL, from_stdin, NULL) == 0);
    assert(pthread_create(&thread2, NULL, to_stdout, NULL) == 0);

    printf("Chat with me via stdin/stdout (I will echo) \n");
    printf("Others may join the group via: telnet %s %d\n", HOST_IP,
           PORT_NUMBER);

    switchboard_init(PORT_NUMBER, HOST_IP, 1);

    while (1) {
        sleep(1000);
    }

    return 0;
}
