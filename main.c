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

/* File-descriptor for READ end of a pipe */
#define PIPE_RD( P ) ( P[0] )
/* File-descriptor for WRITE end of a pipe */
#define PIPE_WR( P ) ( P[1] )

/* Syslog includes stderr or not */
#define INCLUDE_STDERR 1
#define NO_STDERR 0

/* Link-workers */
struct link *worker[3];

/* TTY attibutes to be saved and resored upon entry, exit */
struct {
    int tampered;
    struct termios tty_attr;
} static tty[3] = {
    {
     .tampered = 0}
};

/* local functions */
static void tty_raw_mode(int fd);

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
    char *exec_args[MAX_ARGS] = { NULL };
    int childpid, wpid, status;
    int i, s;
    int v = 0, size = argc - 1;
    char *cmd;
    struct env *env;

    log_syslog_config(INCLUDE_STDERR);  /* (Re-) configure sys-log */
    log_set_process_name(argv[0]);

    ASSERT(argc < MAX_ARGS);

    /* We're passing sockes as arguments to threads as pass by value. Make
     * sure they fit */
    ASSERT(sizeof(void *) >= sizeof(int));
    log_syslog_config(NO_STDERR);   /* (Re-) configure sys-log */

    env_int();
    env = env_get();

    /* Back-up tty settings */
    for (i = 0; i < 3; i++) {
        if (isatty(i))
            tcgetattr(i, &(tty[i].tty_attr));
    }
    if (isatty(0))
        tty_raw_mode(0);

    cmd = (char *)malloc(v);
    for (i = 1; i <= size; i++) {
        cmd = (char *)realloc(cmd, (v + strlen(argv[i])));
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }
    LOGI("PARENT: starts [%s]:%d [%s]\n", env->execute_bin, argc, cmd);
    LOGI("PARENT: socket [%s:%s]\n", env->nic_name, env->port);
    LOGD("PARENT: isatty detect {0:%d} {1:%d} {2:%d}\n", isatty(0), isatty(1),
         isatty(2));
    free(cmd);

    pipe(pipe2child);
    pipe(pipe2parent);

    LOGD("pipe fd:s : ch[0]=%d ch[1]=%d pa[0]=%d pa[1]=%d\n",
         PIPE_RD(pipe2child), PIPE_WR(pipe2child), PIPE_RD(pipe2parent),
         PIPE_WR(pipe2parent));

    exec_args[0] = env->execute_bin;
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

        LOGD("CHILD: isatty detect part 2 {0:%d} {1:%d} {2:%d}\n",
             isatty(0), isatty(1), isatty(2));
        execv(env->execute_bin, exec_args);

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

    s = switchboard_init(atoi(env->port), env->nic_name, 1, env->fifo_prename);
    worker[to_child] = link_create(thread_to_child, 0, PIPE_WR(pipe2child));
    worker[from_tcps] =
        link_create(thread_from_tcps, 0xDEAD, PIPE_WR(pipe2child));
    worker[to_parent] = link_create(thread_to_parent, PIPE_RD(pipe2parent), 1);

    do {
        //wpid=waitpid( /*childpid*/ /*0*/ -1, &status, WUNTRACED );
        wpid = waitpid(childpid, &status, WUNTRACED);
        LOGD("CHILD: Wants to exit on signal %d\n", status);
        ASSERT(wpid >= 0);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    LOGD("CHILD: Exits on signal %d\n", status);

    //while ( wait((int*)0) != childpid );

    LOGD("PARENT: Is exiting.\n");

    link_kill(worker[from_tcps]);
    switchboard_die(s);
    link_kill(worker[to_parent]);
    link_kill(worker[to_child]);

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
