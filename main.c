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

/* The maximum numbers of arguments in child we handle*/
#define MAX_ARGS 50

/* The size of each buffer used for transfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

/* File-descriptor for READ end of a pipe */
#define PIPE_RD( P ) ( P[0] )
/* File-descriptor for WRITE end of a pipe */
#define PIPE_WR( P ) ( P[1] )

/* Environment overloadable variables. Note: NEVER change these in code
 * unless bug is found as they are considered trusted and safe, and as they
 * will allow tcp_tap to start without any wrapping scripts (important when
 * debugging and testing). Always use the corresponding environment variable
 * */

/* Name of the main process to run */
char execute_bin[NAME_MAX] = "/bin/sh";

/* Port number */
char port[NAME_MAX] = "6969";

/* FIFO(s) pre-name */
char fifo_prename[NAME_MAX] = FIFO_DIR "/tcptap-swtchbrd_";

/* listen at NIC bound to this name (human readable name or
 * IP-address). Aditionaly two special names:
 * @HOSTNAME@: Look up the primary interface bound to this name
 * @ANY@: Allow connection to any of the servers IF
 * */
char nic_name[NAME_MAX] = "127.0.0.1";

/* TTY attibutes to be saved and resored upon entry, exit */
struct {
    int tampered;
    struct termios tty_attr;
} static tty[3] = {
    {
     .tampered = 0}
};

#define SETFROMENV( envvar, locvar, buf_max)                \
{                                                           \
    char *ts;                                               \
    if ((ts=getenv(#envvar)) != NULL ) {                    \
        int l;                                              \
        memset(locvar,0,buf_max);                           \
        l=strnlen(ts,buf_max);                              \
        memcpy(locvar,ts,l<buf_max?l:buf_max);              \
    }                                                       \
}

/* Transfer types */
struct data_link {
    int read_from;
    int write_to;
    char *buffer;
};

/* local functions */
static void tty_raw_mode(int fd);

/* Threads handing shuffling of data */

/* Transfers stdin and sends to child (and TCP sockes, if any) */
void *to_child(void *arg)
{
    int i, j, wfd;
    struct data_link *lp = (struct data_link *)arg;

    ASSERT((wfd = open(switchboard_fifo_names()->in_name, O_WRONLY)) >= 0);
    if (isatty(lp->read_from))
        tty_raw_mode(lp->read_from);

    while (1) {
        i = read(lp->read_from, lp->buffer, BUFF_SZ);
        if (i < 0) {
            LOGE("Failed reading from fd=[%d] in %s: " __FILE__ ":"
                 STR(__LINE__), __func__, lp->read_from);
            exit(EXIT_FAILURE);
        }
        LOGV("R(%s): [%s]\n", __func__, lp->buffer);

        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
        j = write(wfd, lp->buffer, i);
        ASSERT(i == j);
    }
    return NULL;
}

/* Transfers output from child and sends to stdout(and TCP socket, if any) */
void *to_parent(void *arg)
{
    int i, j, wfd;
    struct data_link *lp = (struct data_link *)arg;

    ASSERT((wfd = open(switchboard_fifo_names()->in_name, O_WRONLY)) >= 0);

    while (1) {
        i = read(lp->read_from, lp->buffer, BUFF_SZ);
        if (i < 0) {
            LOGE("Failed reading from fd=[%d] in %s: " __FILE__ ":"
                 STR(__LINE__), __func__, lp->read_from);
            exit(-1);
        }
        LOGV("R(%s): [%s]\n", __func__, lp->buffer);

        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
        j = write(wfd, lp->buffer, i);
        ASSERT(i == j);
    }
    return NULL;
}

/* Transfers from TCP and sends to child */
void *from_tcp(void *arg)
{
    int i, j, rfd;
    struct data_link *lp = (struct data_link *)arg;

    ASSERT((rfd = open(switchboard_fifo_names()->out_name, O_RDONLY)) >= 0);

    while (1) {
        i = read(rfd, lp->buffer, BUFF_SZ);
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
        LOGV("R(%s): [%s]\n", __func__, lp->buffer);

        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
    }
    return NULL;
}

static void tty_raw_mode(int fd)
{
    struct termios tty_attr;

    if (fd >= 0 && fd <= 3)
        tty[fd].tampered++;

    tcgetattr(fd, &tty_attr);

    /* Set raw mode. */
    tty_attr.c_lflag &= (~(ICANON /*| ECHO */ ));
    tty_attr.c_cc[VTIME] = 0;
    tty_attr.c_cc[VMIN] = 1;

    ASSERT(tcsetattr(fd, TCSANOW, &tty_attr) != -1);
}

#ifndef HAVE_ISATTY_S
static int isatty(int fd)
{
    struct winsize wsz;
    return !__syscall(SYS_ioctl, fd, TIOCGWINSZ, &wsz);
}
#endif

int main(int argc, char **argv)
{
    int pipe2child[2];
    int pipe2parent[2];
    char buf_to_child[BUFF_SZ];
    char buf_to_parent[BUFF_SZ];
    char *exec_args[MAX_ARGS] = { NULL };
    int childpid, wpid, status;
    int i, s;
    pthread_t pt_to_child;
    pthread_t pt_to_parent;
    pthread_t pt_from_tcp;
    struct data_link link_to_child;
    struct data_link link_to_parent;
    int v = 0, size = argc - 1;
    char *cmd;

    ASSERT(argc < MAX_ARGS);

    /* We're passing sockes as arguments to threads as pass by value. Make
     * sure they fit */
    ASSERT(sizeof(void *) >= sizeof(int));

    /* Back-up tty settings */
    for (i = 0; i < 3; i++) {
        if (isatty(i))
            tcgetattr(i, &(tty[i].tty_attr));
    }

    SETFROMENV(TCP_TAP_EXEC, execute_bin, NAME_MAX);
    SETFROMENV(TCP_TAP_PORT, port, NAME_MAX);
    SETFROMENV(TCP_TAP_NICNAME, nic_name, NAME_MAX);
    SETFROMENV(TCP_TAP_FIFO_PRE_NAME, fifo_prename, NAME_MAX);

    cmd = (char *)malloc(v);
    for (i = 1; i <= size; i++) {
        cmd = (char *)realloc(cmd, (v + strlen(argv[i])));
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }
    LOGD("tcp-tap isatty detect {0:%d} {1:%d} {2:%d}\n", isatty(0), isatty(1),
         isatty(2));
    LOGI("tcp-tap starts [%d]: %s %s\n", argc, execute_bin, cmd);
    LOGI("tcp-tap socket [%s:%d]\n", nic_name, port);
    free(cmd);

    pipe(pipe2child);
    pipe(pipe2parent);

    exec_args[0] = execute_bin;
    for (i = 1; i < argc; i++) {
        exec_args[i] = argv[i];
    }

    ASSERT((childpid = fork()) >= 0);

    if (childpid == 0) {
        /* Child excutes this */
        LOGD("CHILD: isatty detect part 1 {0:%d} {1:%d} {2:%d}\n",
             isatty(0), isatty(1), isatty(2));
        LOGI("CHILD: Will execute:\n");
        for (i = 0; i < argc; i++) {
            LOGI("   [%s]\n", exec_args[i]);
        }

        close(0);
        dup(PIPE_RD(pipe2child));

        close(1);
        dup(PIPE_WR(pipe2parent));

        close(2);
        dup(PIPE_WR(pipe2parent));

        close(PIPE_RD(pipe2child));
        close(PIPE_WR(pipe2child));
        close(PIPE_RD(pipe2parent));
        close(PIPE_WR(pipe2parent));

        LOGD("tcp-tap isatty child detect part 2 {0:%d} {1:%d} {2:%d}\n",
             isatty(0), isatty(1), isatty(2));
        execv(execute_bin, exec_args);

        /* Should never execute */
        LOGE("exec error:" __FILE__ " +" STR(__LINE__) " %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Parent executes this */
    sig_mngr_init(childpid);
    close(PIPE_RD(pipe2child));
    close(PIPE_WR(pipe2parent));

    LOGI("PARENT: handle execution of:\n");
    for (i = 0; i < argc; i++) {
        LOGI("   [%s]\n", exec_args[i]);
    }

    link_to_child.read_from = 0;
    link_to_child.write_to = PIPE_WR(pipe2child);
    link_to_child.buffer = buf_to_child;

    link_to_parent.read_from = PIPE_RD(pipe2parent);
    link_to_parent.write_to = 1;
    link_to_parent.buffer = buf_to_parent;

    s = switchboard_init(atoi(port), nic_name, 1, fifo_prename);
    ASSERT(pthread_create(&pt_to_child, NULL, to_child, &link_to_child) == 0);
    ASSERT(pthread_create(&pt_to_parent, NULL, to_parent, &link_to_parent) ==
           0);
    ASSERT(pthread_create(&pt_from_tcp, NULL, from_tcp, &link_to_child) == 0);

    do {
        //wpid=waitpid( /*childpid*/ /*0*/ -1, &status, WUNTRACED );
        wpid = waitpid(childpid, &status, WUNTRACED);
        LOGD("CHILD: Wants to exit on signal %d\n", wpid);
        ASSERT(wpid >= 0);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    LOGD("CHILD: Exits on signal %d\n", wpid);

    //while ( wait((int*)0) != childpid );

    LOGD("PARENT: Is exiting.\n");

    pthread_cancel(pt_from_tcp);
    switchboard_die(s);
    pthread_cancel(pt_to_parent);
    pthread_cancel(pt_to_child);

    close(PIPE_WR(pipe2child));
    close(PIPE_RD(pipe2parent));

    /* Restore tty settings */
    for (i = 0; i < 3; i++) {
        if (isatty(i) && tty[i].tampered) {
            LOGD("Restore tty[%d] tampered [%d] times\n", i, tty[i].tampered);
            tcsetattr(i, TCSANOW, &(tty[i].tty_attr));
        }
    }
    LOGI("PARENT: Thank you for the fish!\n");
    return 0;
}
