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
#include <tcp-tap/switchboard.h>
#include <tcp-tap/server.h>
#include "tcp-tap_config.h"

#undef  NDEBUG
#include <assert.h>

/* The size of each buffer used for transfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

void *from_stdin(void *arg)
{
    int rn, sn;
    int wfd;
    char buf[BUFF_SZ];

    usleep(100000);
    assert((wfd = open(Q_TO_SWTCH, O_WRONLY)) >= 0);

    while (1) {
        rn = read(1, buf, BUFF_SZ);
        sn = write(wfd, buf, rn);
        assert(rn == sn);
    }
}

void *to_stdout(void *arg)
{
    int rn, sn;
    int rfd;
    char buf[BUFF_SZ];

    usleep(100000);
    assert((rfd = open(Q_FROM_SWTCH, O_RDONLY)) >= 0);

    while (1) {
        rn = read(rfd, buf, BUFF_SZ);
        sn = write(2, buf, rn);
        assert(rn == sn);
    }
}

int main(int argc, char **argv)
{
    pthread_t thread1, thread2;

    assert(pthread_create(&thread1, NULL, from_stdin, NULL) == 0);
    assert(pthread_create(&thread2, NULL, to_stdout, NULL) == 0);

    switchboard_start(6666, "localhost", 1);

    return 0;
}
