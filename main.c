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

/* flags for all the logs*/
//#define LFLAGS ( O_WRONLY | O_APPEND | O_CREAT /*| O_CLOEXEC |*/ O_SYNC )
#define LFLAGS ( O_WRONLY | O_APPEND | O_CREAT | O_SYNC )

/* modes for all the logs */
#define LMODES 0777

/* Environment overloadable variables. Note: NEVER change these in code
 * unless bug is found as they are considered trusted and safe, and as they
 * will allow tcp_tap to start without any wrapping scripts (important when
 * debugging and testing). Always use the corresponding environment variable
 * */

/* stdin,stdout,stderr for the managed sub-process. Used for debugging. Can
 * safely be rerouted to /dev/null */
char stdin_name[NAME_MAX] = "/tmp/tcp_tap_stdin";
char stdout_name[NAME_MAX] = "/tmp/tcp_tap_stdout";
char stderr_name[NAME_MAX] = "/tmp/tcp_tap_stderr";

/* Some log-files */
char parent_log_name[NAME_MAX] = "/tmp/tcp_tap_parent.log";
char child_log_name[NAME_MAX] = "/tmp/tcp_tap_child.log";   //stderr for the child

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
    int log_to;
    char *buffer;
};

/* Threads handing shuffling of data */

/* Transfers stdin and sends to child (and TCP sockes, if any) */
void *to_child(void *arg)
{
    int i, j, wfd;
    struct data_link *lp = (struct data_link *)arg;

    ASSERT((wfd = open(switchboard_fifo_names()->in_name, O_WRONLY)) >= 0);

    while (1) {
        i = read(lp->read_from, lp->buffer, BUFF_SZ);
        if (i < 0) {
            LOGE("Failed reading from file-descriptor" __FILE__ ":"
                 STR(__LINE__) ": ");
            exit(EXIT_FAILURE);
        }
        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
        j = write(lp->log_to, lp->buffer, i);
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
            LOGE("Failed reading from pipe: " __FILE__ " +" STR(__LINE__)
                 " ");
            exit(-1);
        }
        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
        j = write(lp->log_to, lp->buffer, i);
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
            LOGE("Failed reading from TCP: " __FILE__ " +" STR(__LINE__) " ");
            exit(-1);
        }
        j = write(lp->write_to, lp->buffer, i);
        ASSERT(i == j);
        j = write(lp->log_to, lp->buffer, i);
        ASSERT(i == j);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int pipe_to_child[2];
    int pipe_to_parent[2];
    int parent_log_fd, child_err_fd;
    int stdinlog_fd, stdoutlog_fd, stderrlog_fd;
    char *exec_args[MAX_ARGS] = { NULL };
    char buf_to_child[BUFF_SZ];
    char buf_to_parent[BUFF_SZ];
    int childpid, wpid, status;
    int i, j, k, s;
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

    SETFROMENV(TCP_TAP_EXEC, execute_bin, NAME_MAX);
    SETFROMENV(TCP_TAP_PORT, port, NAME_MAX);
    SETFROMENV(TCP_TAP_NICNAME, nic_name, NAME_MAX);
    SETFROMENV(TCP_TAP_LOG_STDIN, stdin_name, NAME_MAX);
    SETFROMENV(TCP_TAP_LOG_STDOUT, stdout_name, NAME_MAX);
    SETFROMENV(TCP_TAP_LOG_STDERR, stderr_name, NAME_MAX);
    SETFROMENV(TCP_TAP_LOG_PARENT, parent_log_name, NAME_MAX);
    SETFROMENV(TCP_TAP_LOG_CHILD, child_log_name, NAME_MAX);
    SETFROMENV(TCP_TAP_FIFO_PRE_NAME, fifo_prename, NAME_MAX);

    ASSERT((stdinlog_fd = open(stdin_name, LFLAGS, LMODES)) > 0);
    ASSERT((stdoutlog_fd = open(stdout_name, LFLAGS, LMODES)) > 0);
    ASSERT((stderrlog_fd = open(stderr_name, LFLAGS, LMODES)) > 0);
    ASSERT((child_err_fd = open(child_log_name, LFLAGS, LMODES)) > 0);
    ASSERT((parent_log_fd = open(parent_log_name, LFLAGS, LMODES)) > 0);

    cmd = (char *)malloc(v);
    for (i = 1; i <= size; i++) {
        cmd = (char *)realloc(cmd, (v + strlen(argv[i])));
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }
    LOGI("tcp-tap starts [%d]: %s %s\n", argc, execute_bin, cmd);
    LOGI("tcp-tap socket [%s:%d]\n", nic_name, port);
    free(cmd);

    close(2);
    dup(stderrlog_fd);

    pipe(pipe_to_child);
    pipe(pipe_to_parent);

    exec_args[0] = execute_bin;
    for (i = 1; i < argc; i++) {
        exec_args[i] = argv[i];
    }

    ASSERT((childpid = fork()) >= 0);

    if (childpid == 0) {
        /* Child excutes this */
        sprintf(buf_to_child, "Child will execute:\n");
        k = write(child_err_fd, buf_to_child, strnlen(buf_to_child, BUFF_SZ));
        for (i = 0; i < argc; i++) {
            j = snprintf(buf_to_child, BUFF_SZ, "%s\n", exec_args[i]);
            k = write(child_err_fd, buf_to_child,
                      strnlen(buf_to_child, BUFF_SZ));
            if (i)
                ASSERT(k == j);
        }
        sprintf(buf_to_child, "=========X=========X=========X=========X\n");
        write(child_err_fd, buf_to_child, strnlen(buf_to_child, BUFF_SZ));
        sprintf(buf_to_child, "This is now stderr:\n");
        k = write(child_err_fd, buf_to_child, strnlen(buf_to_child, BUFF_SZ));
        sprintf(buf_to_child, "=========X=========X=========X=========X\n");
        write(child_err_fd, buf_to_child, strnlen(buf_to_child, BUFF_SZ));

        close(0);
        dup(pipe_to_child[0]);

        close(1);
        dup(pipe_to_parent[1]);

        close(2);
        dup(pipe_to_parent[1]);

        close(pipe_to_child[0]);
        close(pipe_to_child[1]);
        close(pipe_to_parent[0]);
        close(pipe_to_parent[1]);

        execv(execute_bin, exec_args);

        /* Should never execute */
        LOGE("exec error:" __FILE__ " +" STR(__LINE__) " %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Parent executes this */

    sig_mngr_init(childpid);

    k = sprintf(buf_to_parent, "Parent handles execution of:\n");
    k = write(parent_log_fd, buf_to_parent, strnlen(buf_to_parent, BUFF_SZ));
    for (i = 0; i < argc; i++) {
        j = snprintf(buf_to_parent, BUFF_SZ, "%s\n", exec_args[i]);
        k = write(parent_log_fd, buf_to_parent,
                  strnlen(buf_to_parent, BUFF_SZ));
        ASSERT(k == j);
    }
    sprintf(buf_to_parent, "=========Y=========Y=========Y=========Y\n");
    write(parent_log_fd, buf_to_parent, strnlen(buf_to_parent, BUFF_SZ));

    link_to_child.read_from = 0;
    link_to_child.write_to = pipe_to_child[1];
    close(pipe_to_child[0]);
    link_to_child.log_to = stdinlog_fd;
    link_to_child.buffer = buf_to_child;

    link_to_parent.read_from = pipe_to_parent[0];
    close(pipe_to_parent[1]);
    link_to_parent.write_to = 1;
    link_to_parent.log_to = stdoutlog_fd;
    link_to_parent.buffer = buf_to_parent;

    s = switchboard_init(atoi(port), nic_name, 1, fifo_prename);
    ASSERT(pthread_create(&pt_to_child, NULL, to_child, &link_to_child) == 0);
    ASSERT(pthread_create(&pt_to_parent, NULL, to_parent, &link_to_parent) ==
           0);
    ASSERT(pthread_create(&pt_from_tcp, NULL, from_tcp, &link_to_child) == 0);

    do {
        //wpid=waitpid( /*childpid*/ /*0*/ -1, &status, WUNTRACED );
        wpid = waitpid(childpid, &status, WUNTRACED);
        ASSERT(wpid >= 0);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    //while ( wait((int*)0) != childpid );

    sprintf(buf_to_parent, "tcp_tap parent exiting. Thanks for the fish...\n");
    write(parent_log_fd, buf_to_parent, strnlen(buf_to_parent, BUFF_SZ));

    pthread_cancel(pt_from_tcp);
    switchboard_die(s);
    pthread_cancel(pt_to_parent);
    pthread_cancel(pt_to_child);

    sprintf(buf_to_child, "tcp_tap child has exitited. Bye bye!\n");
    write(child_err_fd, buf_to_child, strnlen(buf_to_child, BUFF_SZ));

    close(parent_log_fd);
    close(child_err_fd);
    close(stdinlog_fd);
    close(stdoutlog_fd);
    close(stderrlog_fd);
    close(pipe_to_child[1]);
    close(pipe_to_parent[0]);
    return 0;
}
