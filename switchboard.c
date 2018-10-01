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
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdlib.h>

#undef  NDEBUG
#include <assert.h>
#include <tcp-tap/switchboard.h>
#include <tcp-tap/clientserver.h>
#include "tcp-tap_config.h"
#include "local.h"

/* The size of each buffer used for transfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

/* FIFO-names (named pipes) in use per process/instance. Main mechanism for
 * communicating with the switchboard */
struct switch_fifo switch_fifo;

struct serv_node {
    int fd;                     /* File descriptor */
    int id;                     /* Unique ID of node */
    struct serv_node *next;
    struct serv_node *prev;     /* Avoid need to search when disconnect */
    pthread_t thread;
};

struct switch_struct {
    int s;                      /* Main socket */
    int n;                      /* Number of connected sessions */
    int i;                      /* Total counter */
    int ea;                     /* Do echo to all */
    struct serv_node *serv_list;
};

struct threads_t {
    pthread_t to_swtch;
    pthread_t mngmt;
} threads;

struct switch_struct ss = {
    .s = 0,
    .n = 0,
    .i = 0,
    .ea = 0,
    .serv_list = NULL
};

static void write_toall(const char *buf, int len);
static void *in_session_thread(void *arg);
static void *handle_in_fifo_thread(void *arg);
static void *connect_mngmt_thread(void *arg);
static void disconnect_servlet(struct serv_node *node);

static int init_switchboard(int port, const char *hostname, int echoall)
{
    int s;

    ss.ea = echoall;
    s = init_server(port, hostname);
    ss.s = s;
    return s;
}

static void write_toall(const char *buf, int len)
{
    int sn;

    struct serv_node *ln = ss.serv_list;
    if (ss.ea) {
        while (ln) {
            sn = write(ln->fd, buf, len);
            assert(len == sn);
            ln = ln->next;
        }
    }
}

/* Provide outside access to the internal fifo-names in use.
 */
struct switch_fifo *switchboard_fifo_names()
{
    return &switch_fifo;
}

/* Handling incoming data from own session. I.e. one thread per connection
 * handle it's in-data. Read data is written both to fifo-out (i.e. back to
 * connecting process) and to all other connected sessions.
 */
static void *in_session_thread(void *inarg)
{
    int rn, sn;
    struct serv_node *node = (struct serv_node *)inarg;
    int fdo, fd = node->fd;
    char buf[BUFF_SZ];
    assert((fdo = open(switch_fifo.out_name, O_WRONLY)) >= 0);

    fprintf(stderr, "Session [%d] connected.\n", node->id);

    for (rn = 1; rn > 0;) {
        rn = read(fd, buf, BUFF_SZ);
        if (rn > 0) {
            write_toall(buf, rn);
            sn = write(fdo, buf, rn);
            assert(sn == rn);
        }
    }
    if (rn == 0) {
        fprintf(stderr, "Session [%d] disconnected normally...\n", node->id);
    } else {
        perror("Session read error detected: "__FILE__ " +" STR(__LINE__) " ");
        fprintf(stderr, "Session [%d] now disconnecting.\n", node->id);
    }
    close(fdo);                 /* Release resource */
    disconnect_servlet(node);

    return NULL;
}

/* Thread reading data FROM in_fifo and writes TO all connected
 * socket sessions.
 */
static void *handle_in_fifo_thread(void *arg)
{
    char buf[BUFF_SZ];
    int rn, fd;

    assert((fd = open(switch_fifo.in_name, O_RDONLY)) >= 0);

    while (1) {
        rn = read(fd, buf, BUFF_SZ);
        write_toall(buf, rn);
    }
    return NULL;
}

/* Close a connection and unlink it from the list */

/* FIXME >>> Possible race condition here <<< FIXME */
static void disconnect_servlet(struct serv_node *node)
{
    if (ss.n == 1) {
        assert((node->prev == NULL) && (node->next == NULL));
    }
    if (node->prev == NULL && node->next == NULL) {
        assert(ss.n == 1);
        ss.serv_list = NULL;
    } else {
        if (node->prev) {
            node->prev->next = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        }
    }
    free(node);
    ss.n--;
}

/* Thread waits for new connections. When detected, an in_session thread is
 * created per each connection and the connection is inserted in the
 * switchboard list.
 */
static void *connect_mngmt_thread(void *arg)
{
    int fd;
    struct serv_node *tn, **lnp, *lp = NULL;
    int s = (long)arg;

    while (1) {
        assert((fd = open_server(s)) >= 0);
        tn = malloc(sizeof(struct serv_node));
        ss.i++;
        tn->id = ss.i;
        tn->fd = fd;
        tn->next = NULL;
        tn->prev = NULL;
        lp = NULL;

        lnp = &ss.serv_list;
        while (*lnp) {
            lp = (struct serv_node *)(*lnp);
            lnp = &(lp->next);
            //lnp=&(*((struct serv_node*)(*lnp)).next);
        }

        *lnp = tn;
        (*lnp)->prev = lp;

        ss.n++;
        assert(pthread_create(&tn->thread, NULL, in_session_thread, (void *)tn)
               == 0);
        //sleep(10);
    }
}

int switchboard_init(int port, const char *host, int echo, const char *prename)
{
    int s;
    char tprename[PATH_MAX];
    char tin_name[PATH_MAX];
    char tout_name[PATH_MAX];
    int slen;
    int pid = getpid();

    memset(tprename, 0, PATH_MAX);
    memset(tin_name, 0, PATH_MAX);
    memset(tout_name, 0, PATH_MAX);

    /* Constructing fifo-names (build-up) */
    if (prename == NULL) {
        strncpy(tprename, FIFO_DIR "/fifo_switchboard", PATH_MAX);
    } else {
        strncpy(tprename, FIFO_DIR "/", PATH_MAX);
        slen = strnlen(tprename, PATH_MAX);
        strncpy(&tprename[slen], prename, PATH_MAX - slen);
    }

    snprintf(tin_name, PATH_MAX, "%s_%s_%d", tprename, "in", pid);
    snprintf(tout_name, PATH_MAX, "%s_%s_%d", tprename, "out", pid);

    switch_fifo.in_name = strndup(tin_name, PATH_MAX);
    switch_fifo.out_name = strndup(tout_name, PATH_MAX);

    unlink(switch_fifo.in_name);
    unlink(switch_fifo.out_name);

    mkfifo(switch_fifo.in_name, 0777);
    mkfifo(switch_fifo.out_name, 0777);

    s = init_switchboard(port, host, echo);

    assert(pthread_create(&threads.to_swtch, NULL, handle_in_fifo_thread, NULL)
           == 0);

    assert(pthread_create
           (&threads.mngmt, NULL, connect_mngmt_thread,
            (void *)((intptr_t) s)) == 0);
    return s;
}

void switchboard_die(int s)
{
    struct serv_node *ln = ss.serv_list;

    pthread_cancel(threads.mngmt);
    pthread_cancel(threads.to_swtch);

    if (ss.ea) {
        while (ln) {
            pthread_cancel(ln->thread);
            assert(close(ln->fd) == 0);
            ln = ln->next;
        }
    }

    assert(close(s) == 0);
    unlink(switch_fifo.in_name);
    unlink(switch_fifo.out_name);
    free(switch_fifo.in_name);
    free(switch_fifo.out_name);
}
