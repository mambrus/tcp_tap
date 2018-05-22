/* Simple chat server
 * ==================
 *
 * Shich-board shuffles all from all - to all
 *
 * >>> This example is WIP <<<
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tcp-tap/switchboard.h>
#include <tcp-tap/server.h>
#include <stdlib.h>
#include <errno.h>

#undef  NDEBUG
#include <assert.h>

/* The size of each buffer used for transfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

#define PORT_NUMBER 1976
#define HOST_IP "localhost"

int main(int argc, char **argv)
{
    printf("Chat with me via stdin/stdout (I will echo) \n");
    printf("Others may join the group via: telnet %s %d\n", HOST_IP,
           PORT_NUMBER);

    switchboard_init(PORT_NUMBER, HOST_IP, 1);

    while (1) {
        sleep(1000);
    }

    return 0;
}
