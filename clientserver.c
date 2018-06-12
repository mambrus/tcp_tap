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
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <tcp-tap/clientserver.h>
#include "local.h"

#undef  NDEBUG
#include <assert.h>

#define BACKLOG 5
#define MAX_RETRY 3
#define RETRY_US  20000000

/* Returns a valid socket fd that can block at accept(), i.e. wait for
 * client side connect() */
int init_server(int port, const char *hostname)
{
    int s, n;
    char name[NAME_MAX];
    struct hostent *hp;
    struct sockaddr_in lsin;
    int rc;

    if (!hostname ||
        !strncmp(hostname, "@HOSTNAME@", NAME_MAX) ||
        !strncmp(hostname, "@ANY@", NAME_MAX)
        )
        assert(gethostname(name, NAME_MAX) == 0);
    else
        strncpy(name, hostname, NAME_MAX);

    assert((hp = gethostbyname(name)) != NULL);

    assert((s = socket(AF_INET, SOCK_STREAM, 0)) >= 0);
    lsin.sin_family = AF_INET;
    lsin.sin_port = htons(port);

    memcpy(&lsin.sin_addr, hp->h_addr, hp->h_length);

    if (!strncmp(hostname, "@ANY@", NAME_MAX))
        lsin.sin_addr.s_addr = INADDR_ANY;

    for (n = 0; n < MAX_RETRY; n++) {
        int optval = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

        rc = bind(s, (struct sockaddr *)&lsin, sizeof(lsin));
        if (rc < 0) {
            LOGE("bind: " __FILE__ " +" STR(__LINE__) " %s", strerror(errno));
            LOGE("Retry: %d of %d\n", n + 1, MAX_RETRY);
            usleep(RETRY_US);
        } else
            break;
    }
    return s;
}

/* Waits for connection, then returns a fd that can be used for r/w */
int open_server(int s)
{
    struct sockaddr_in rsin;
    socklen_t fromlen;
    int rc, n;

    fromlen = sizeof(struct sockaddr_in);

    for (n = 0; n < MAX_RETRY; n++) {
        assert(listen(s, BACKLOG) >= 0);

        rc = accept(s, (struct sockaddr *)&rsin, &fromlen);
        if (rc < 0) {
            /* print error reason to stderr, but don't exit */
            LOGE("accept: " __FILE__ " +" STR(__LINE__) " %s", strerror(errno));
            LOGE("Retry %d of %d\n", n + 1, MAX_RETRY);
            usleep(RETRY_US);
        } else
            break;
    }
    return rc;
}

int open_client(int port, const char *hostname)
{
    struct sockaddr_in lsin;
    socklen_t s;
    int rc, n;
    struct hostent *hp;
    char name[NAME_MAX];

    strncpy(name, hostname, NAME_MAX);
    assert((hp = gethostbyname(name)) != NULL);

    assert((s = socket(AF_INET, SOCK_STREAM, 0)) >= 0);
    lsin.sin_family = AF_INET;
    lsin.sin_port = htons(port);

    memcpy(&lsin.sin_addr, hp->h_addr, hp->h_length);

    for (n = 0; n < MAX_RETRY; n++) {

        rc = connect(s, (struct sockaddr *)&lsin, sizeof(lsin));
        if (rc < 0) {
            /* print error reason to stderr, but don't exit */
            LOGE("connect: " __FILE__ " +" STR(__LINE__) " %s",
                 strerror(errno));
            LOGE("Retry %d of %d\n", n + 1, MAX_RETRY);
            usleep(RETRY_US);
        } else {
            rc = s;
            break;
        }
    }
    return rc;
}

/* Create a DGRAM socket as either client/server in the local domain */
int named_socket(int isserver, const char *filename)
{
    struct sockaddr_un name;
    int sock;
    size_t size;

    /* Create the socket. */
    sock = socket(PF_LOCAL, SOCK_DGRAM, 0);
    if (sock < 0) {
        LOGE("socket: " __FILE__ " +" STR(__LINE__) " %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Bind a name to the socket. */
    name.sun_family = AF_LOCAL;
    strncpy(name.sun_path, filename, sizeof(name.sun_path));
    name.sun_path[sizeof(name.sun_path) - 1] = '\0';

    /* The size of the address is
       the offset of the start of the filename,
       plus its length (not including the terminating null byte).
       Alternatively you can just do:
       size = SUN_LEN (&name);
     */
    size = (offsetof(struct sockaddr_un, sun_path)
            + strlen(name.sun_path));

    if (isserver) {
        if (bind(sock, (struct sockaddr *)&name, size) < 0) {
            LOGE("bind: " __FILE__ " +" STR(__LINE__) " %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    } else {
        if (connect(sock, (struct sockaddr *)&name, size) < 0) {
            LOGE("connect: " __FILE__ " +" STR(__LINE__) " %s",
                 strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    return sock;
}

/* Receive a valid file-descriptor from another process */
ssize_t read_fd(int sock, void *buf, ssize_t bufsize, int *fd)
{
    ssize_t size;

    if (fd) {
        struct msghdr msg;
        struct iovec iov;
        union {
            struct cmsghdr cmsghdr;
            char control[CMSG_SPACE(sizeof(int))];
        } cmsgu;
        struct cmsghdr *cmsg;

        iov.iov_base = buf;
        iov.iov_len = bufsize;

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        size = recvmsg(sock, &msg, 0);
        if (size < 0) {
            LOGE("recvmsg: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if ((msg.msg_flags & MSG_TRUNC) || (msg.msg_flags & MSG_CTRUNC)) {
            LOGE("control message truncated");
            exit(EXIT_FAILURE);
        }
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
            if (cmsg->cmsg_level != SOL_SOCKET) {
                LOGE("invalid cmsg_level %d\n", cmsg->cmsg_level);
                exit(EXIT_FAILURE);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS) {
                LOGE("invalid cmsg_type %d\n", cmsg->cmsg_type);
                exit(EXIT_FAILURE);
            }

            *fd = *((int *)CMSG_DATA(cmsg));
            LOGD("received fd %d\n", *fd);
        } else
            *fd = -1;
    } else {
        size = read(sock, buf, bufsize);
        if (size < 0) {
            LOGE("read: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return size;
}

/* Transfer a file-descriptor to another process */
ssize_t write_fd(int sock, void *buf, ssize_t buflen, int fd)
{
    ssize_t size;
    struct msghdr msg;
    struct iovec iov;
    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    struct cmsghdr *cmsg;

    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (fd != -1) {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        LOGD("passing fd %d\n", fd);
        *((int *)CMSG_DATA(cmsg)) = fd;
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        LOGE("not passing fd\n");
    }

    size = sendmsg(sock, &msg, 0);

    if (size < 0)
        LOGE("sendmsg: %s", strerror(errno));
    return size;
}
