/***************************************************************************
 *   Copyright (C) 2011 by Michael Ambrus                                  *
 *   ambrmi09@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
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

/* Just echo back everything */
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

/* Wait for connect, then spawn a thread for each connection and then wait
 * for more connections */
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
