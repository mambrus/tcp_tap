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
#include <netdb.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <tcp-tap/clientserver.h>

#undef  NDEBUG
#include <assert.h>

#define BACKLOG 5
#define MAX_RETRY 3
#define RETRY_US  20000000

/* Returns a valid socket fd on connection */
int init_server(int port, const char *hostname)
{
    int s, n;
    char name[PATH_MAX];
    struct hostent *hp;
    struct sockaddr_in lsin;
    int rc;

    if (!hostname ||
        !strncmp(hostname, "@HOSTNAME@", PATH_MAX) ||
        !strncmp(hostname, "@ANY@", PATH_MAX)
        )
        assert(gethostname(name, PATH_MAX) == 0);
    else
        strncpy(name, hostname, PATH_MAX);

    assert((hp = gethostbyname(name)) != NULL);

    assert((s = socket(AF_INET, SOCK_STREAM, 0)) >= 0);
    lsin.sin_family = AF_INET;
    lsin.sin_port = htons(port);

    memcpy(&lsin.sin_addr, hp->h_addr, hp->h_length);

    if (!strncmp(hostname, "@ANY@", PATH_MAX))
        lsin.sin_addr.s_addr = INADDR_ANY;

    for (n = 0; n < MAX_RETRY; n++) {
        int optval = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

        rc = bind(s, (struct sockaddr *)&lsin, sizeof(lsin));
        if (rc < 0) {
            perror("bind: ");
            fprintf(stderr, "Retry: %d of %d\n", n + 1, MAX_RETRY);
            //close(s);
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
            perror("accept: ");
            fprintf(stderr, "Retry %d of %d\n", n + 1, MAX_RETRY);
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
    char name[PATH_MAX];

    strncpy(name, hostname, PATH_MAX);
    assert((hp = gethostbyname(name)) != NULL);

    assert((s = socket(AF_INET, SOCK_STREAM, 0)) >= 0);
    lsin.sin_family = AF_INET;
    lsin.sin_port = htons(port);

    memcpy(&lsin.sin_addr, hp->h_addr, hp->h_length);

    for (n = 0; n < MAX_RETRY; n++) {

        rc = connect(s, (struct sockaddr *)&lsin, sizeof(lsin));
        if (rc < 0) {
            /* print error reason to stderr, but don't exit */
            perror("connect: ");
            fprintf(stderr, "Retry %d of %d\n", n + 1, MAX_RETRY);
            usleep(RETRY_US);
        } else {
            rc = s;
            break;
        }
    }
    return rc;
}