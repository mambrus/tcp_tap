/* Threaded chat server
 * ====================
 *
 * Shich-board shuffles all from all - to all
 *
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <tcp-tap/switchboard.h>
#include <tcp-tap/clientserver.h>
#include "config.h"

#undef  NDEBUG
#include <assert.h>

/* The size of each buffer used for transfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

#define PORT_NUMBER 1974
#define HOST_IP "localhost"
#define KILLWORD "@@@"

#define _XSTR( X )  #X
#define STR( X ) _XSTR( X )

struct appData {
    int quit;
    int ss;
} appData = {
.quit = 0,.ss = -1};

/* Compare string 1 (s2) against variants of s2 where different forms of EOL
 * may be appended. Any match is considered true, but only exact matches are
 * considered equal */
#define termn_cmp(s1, s2, sz) (                     \
    (strncmp(s1, s2, sz) == 0) ? 0 :                \
    (strncmp(s1, s2 STR( \r ), sz) == 0) ? 0 :      \
    (strncmp(s1, s2 STR( \n ), sz) == 0) ? 0 : 1    \
)

void *from_stdin_to_switch(void *arg)
{
    int rn = BUFF_SZ, sn;
    int wfd;
    char buf[BUFF_SZ];
    struct appData *ad = (struct appData *)arg;

    memset(buf, 0, BUFF_SZ);
    assert((wfd = open(switchboard_fifo_names()->in_name, O_WRONLY)) >= 0);

    while (rn > 0 && !ad->quit) {
        rn = read(0, buf, BUFF_SZ);
        sn = write(wfd, buf, rn);
        assert(rn == sn);
        if (termn_cmp(buf, KILLWORD, BUFF_SZ) == 0) {
            ad->quit = 1;
            printf("Killword from sdtin received\n");
        }
        memset(buf, 0, BUFF_SZ);
    }

    if (rn < 0)
        perror("read() failed: ");

    close(wfd);
    return NULL;
}

void *from_swtch_to_stdout(void *arg)
{
    int rn = BUFF_SZ, sn;
    int rfd;
    char buf[BUFF_SZ];
    struct appData *ad = (struct appData *)arg;

    memset(buf, 0, BUFF_SZ);
    assert((rfd = open(switchboard_fifo_names()->out_name, O_RDONLY)) >= 0);

    while (rn > 0 && !ad->quit) {
        rn = read(rfd, buf, BUFF_SZ);
        sn = write(1, buf, rn);
        assert(rn == sn);
        if (termn_cmp(buf, KILLWORD, BUFF_SZ) == 0) {
            ad->quit = 1;
            printf("Killword from switchboard received\n");
            /* Try kill the other (stdio-)thread too now as it's currently blocking */
            rn = write(0, buf, BUFF_SZ);
        }
        memset(buf, 0, BUFF_SZ);
    }

    if (rn < 0)
        perror("read() failed: ");

    close(rfd);
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t thread1, thread2;

    appData.ss = switchboard_init(PORT_NUMBER, HOST_IP, 1, NULL);

    printf("Chat with me via stdin/stdout (I will echo) \n");
    printf("Others may join the group via: telnet %s %d\n", HOST_IP,
           PORT_NUMBER);
    printf("Killword for either server or sessions to quit application %s\n",
           KILLWORD);
    printf("Note: There is no way for server to kick sessions."
           " Each client has to leave on it's own.\n");

    assert(pthread_create(&thread2, NULL, from_swtch_to_stdout, &appData) == 0);
    assert(pthread_create(&thread1, NULL, from_stdin_to_switch, &appData) == 0);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    /* Crean-up threads, close connections and unlink fifos. */
    switchboard_die(appData.ss);

    exit(0);
}
