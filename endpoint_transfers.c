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

/* Thread-based shuffling of data between two fd-based end-points This is a
   high abstraction data shuffler where end-points are pipes, stdio e.t.c. I.e.
   data is already serialized and joined if origination from several sources
   Note: heavy lifting mixing is in switch-board.c
*/

#include <stdio.h>
#include <termios.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <tcp-tap/switchboard.h>
#include "sig_mngr.h"
#include "tcp-tap_config.h"
#include "local.h"

#undef  NDEBUG
#include <liblog/assure.h>

/* The size of each buffer used for transfer in either direction */
#ifndef LINK_BUFF_SZ
#define LINK_BUFF_SZ 0x400
#endif

/* Default log ability for workers operating */
#define ENABLE_LOG_TO_CHILD_DFLT   1
#define ENABLE_LOG_TO_PARENT_DFLT  0
#define ENABLE_LOG_FROM_TCPS_DFLT  0

/* Transfers stdin and sends to child (and TCP sockes, if any) */
void *thread_to_child(void *arg)
{
    int i, j, wfd;
    struct data_link *lp = (struct data_link *)arg;
    char *pname = switchboard_fifo_names()->in_name;

    lp->enable_log = ENABLE_LOG_TO_CHILD_DFLT;
    lp->worker_name = __func__;
    LOGD("Worker [%s] starting, write pipe: %s\n", lp->worker_name, pname);

    ASSERT((wfd = open(pname, O_WRONLY)) >= 0);
    LOGD("Worker [%s] fds: (%d->{%d,%d})\n",
         lp->worker_name, lp->read_from, lp->write_to, wfd);

    while (1) {
        memset(lp->buffer, 0, LINK_BUFF_SZ);
        i = read(lp->read_from, lp->buffer, LINK_BUFF_SZ);
        if (i < 0) {
            LOGE("Failed reading from fd=[%d] in %s: " __FILE__ ":"
                 STR(__LINE__), __func__, lp->read_from);
            exit(EXIT_FAILURE);
        }
        if (lp->enable_log)
            LOGD("R(%s[%d]): [%s]\n", __func__, i, lp->buffer);

        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
        j = write(wfd, lp->buffer, i);
        ASSERT(i == j);
    }
    return NULL;
}

/* Transfers output from child and sends to stdout (and TCP sockets, if any) */
void *thread_to_parent(void *arg)
{
    int i, j, wfd;
    struct data_link *lp = (struct data_link *)arg;
    char *pname = switchboard_fifo_names()->in_name;

    lp->enable_log = ENABLE_LOG_TO_PARENT_DFLT;
    lp->worker_name = __func__;
    LOGD("Worker [%s] starting, write pipe: %s\n", lp->worker_name, pname);

    ASSERT((wfd = open(pname, O_WRONLY)) >= 0);
    LOGD("Worker [%s] fds: (%d->{%d,%d})\n",
         lp->worker_name, lp->read_from, lp->write_to, wfd);

    while (1) {
        memset(lp->buffer, 0, LINK_BUFF_SZ);
        i = read(lp->read_from, lp->buffer, LINK_BUFF_SZ);
        if (i < 0) {
            LOGE("Failed reading from fd=[%d] in %s: " __FILE__ ":"
                 STR(__LINE__), __func__, lp->read_from);
            exit(-1);
        }
        if (lp->enable_log)
            LOGD("R(%s[%d]): [%s]\n", __func__, i, lp->buffer);

        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
        j = write(wfd, lp->buffer, i);
        ASSERT(i == j);
    }
    return NULL;
}

/* Transfers from any TCP session (via pipe) and sends to child */
void *thread_from_tcps(void *arg)
{
    int i, j, rfd;
    struct data_link *lp = (struct data_link *)arg;
    char *pname = switchboard_fifo_names()->out_name;

    lp->enable_log = ENABLE_LOG_FROM_TCPS_DFLT;
    lp->worker_name = __func__;
    LOGD("Worker [%s] starting, read pipe: %s\n", lp->worker_name, pname);

    ASSERT((rfd = open(pname, O_RDONLY)) >= 0);
    LOGD("Worker [%s] fds: (%d->%d)\n", lp->worker_name, rfd, lp->write_to);

    while (1) {
        memset(lp->buffer, 0, LINK_BUFF_SZ);
        i = read(rfd, lp->buffer, LINK_BUFF_SZ);
        /*
           i--;
           lp->buffer[i]=0;
           lp->buffer[i-1]='\n';
         */
        if (i < 0) {
            LOGE("Failed reading from fd=[%d] in %s: " __FILE__ ":"
                 STR(__LINE__), __func__, rfd);
            exit(-1);
        }
        if (lp->enable_log)
            LOGD("R(%s[%d]): [%s]\n", __func__, i, lp->buffer);

        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
    }
    return NULL;
}

struct link *link_create(void *(*worker) (void *), int fd_from, int fd_to)
{
    struct link *tlink = malloc(sizeof(struct link));
    ASSERT(tlink);

    tlink->worker = worker;
    ASSERT(tlink->data_link =
           (struct data_link *)malloc(sizeof(struct data_link)));
    ASSERT(tlink->data_link->buffer = malloc(LINK_BUFF_SZ));

    tlink->data_link->read_from = fd_from;
    tlink->data_link->write_to = fd_to;
    LOGD("%s: %d->%d\n", __func__, tlink->data_link->read_from,
         tlink->data_link->write_to);

    ASSERT(pthread_create(&(tlink->thid), NULL, tlink->worker, tlink->data_link)
           == 0);
    return tlink;
}

void link_kill(struct link *link)
{
    LOGD("%s: %s (%d->%d)\n", __func__, link->data_link->worker_name,
         link->data_link->read_from, link->data_link->write_to);
    ASSERT(pthread_cancel(link->thid) == 0);
    ASSERT(pthread_join(link->thid, NULL) == 0);
    free(link->data_link->buffer);
    free(link->data_link);
    free(link);
}
