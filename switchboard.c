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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdlib.h>

#undef  NDEBUG
#include <assert.h>
#include "switchboard.h"
#include "server.h"

#define DC1 "\377\373\006\375\006\n"

/* The size of each buffer used for transfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

#ifdef TEST2
/* Environment overloadable variables */

/* Port number */
char port_number[PATH_MAX] = "6666";
#endif                          //TEST

struct serv_node {
    int fd;                     /* File descriptor */
    int id;                     /* Unique ID of node */
    struct serv_node *next;
    struct serv_node *prev;     /* Avoid need to search when disconect */
    pthread_t thread;
};

struct switch_struct {
    int s;                      /* Main socket */
    int n;                      /* Number of connected sessions */
    int i;                      /* Counter */
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
    .ea = 0,
    .serv_list = NULL
};

static void write_toall(const char *buf, int len);
static void *shuffleThread(void *arg);
static void *to_swtch_thread(void *arg);
static void *connect_mngmt_thread(void *arg);
static void disconnect_servlet(struct serv_node *node);

int init_switchboard(int port, const char *hostname, int echoall)
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
            //assert(len==sn);
            ln = ln->next;
        }
    }
}

/* Thread handling data from incoming data from it's own client
 * (i.e. from telnet incoming data
 */
static void *shuffleThread(void *inarg)
{
    int rn, sn;
    struct serv_node *node = (struct serv_node *)inarg;
    int fdo, fd = node->fd;
    char buf[BUFF_SZ];
    assert((fdo = open(Q_FROM_SWTCH, O_WRONLY)) >= 0);

    fprintf(stderr, "Session [%d] connected.\n", node->id);

    for (rn = 1; rn > 0;) {
        rn = read(fd, buf, BUFF_SZ);
        if (rn > 0) {
            write_toall(buf, rn);
            write(fdo, buf, rn);
            assert(sn = rn);
        }
    }
    if (rn == 0) {
        fprintf(stderr, "Session [%d] disconnected normally...\n", node->id);
    } else {
        perror("Session read error detected: ");
        fprintf(stderr, "Session [%d] now disconnecting.\n", node->id);
    }
    close(fdo);                 // We'll not send data to this queue/named paipe any more
    disconnect_servlet(node);
}

/* Thread handling data FROM this host and TO all connected 
 * sessions 
 */
static void *to_swtch_thread(void *arg)
{
    char buf[BUFF_SZ];
    int rn, fd;

    assert((fd = open(Q_TO_SWTCH, O_RDONLY)) >= 0);

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

/* Thread waits for new connections. When detected, the connection
 * is inserted in the switchboard list.
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
        assert(pthread_create(&tn->thread, NULL, shuffleThread, (void *)tn) ==
               0);
        //sleep(10);
    }

}

/* Just echo back everything */
int switchboard_init(int port, const char *host, int echo)
{
    int s;

    mkfifo(Q_TO_SWTCH, 0777);
    mkfifo(Q_FROM_SWTCH, 0777);

    s = init_switchboard(port, host, echo);
    //ss.ea = echo;
    //s=init_server(6666,"localhost");
    //ss.s=s;

    assert(pthread_create(&threads.to_swtch, NULL, to_swtch_thread, NULL) == 0);
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
}

#ifdef TEST_SWITCH

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
#endif                          //TEST_SWITCH

#ifdef TEST2

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

/* Just echo back everything */
int main(int argc, char **argv)
{
    int fd, s;
    int port;
    pthread_t t_thread;

    port = atoi(port_number);
    char buf[BUFF_SZ];

    s = init_server(port, "localhost");
    while (1) {
        fd = open_server(s);
        assert(pthread_create(&t_thread, NULL, myThread, (void *)fd) == 0);
        //sleep(10);
    }

    return 0;
}
#endif                          //TEST2
