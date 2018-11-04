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
#include <stdlib.h>
#include <string.h>
#include <tcp-tap/switchboard.h>
#include "tcp-tap_config.h"
#include "local.h"

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

struct env env = {
/* *INDENT-OFF* */
    .execute_bin = "/bin/sh",
    .port = "6969",
    .fifo_prename = FIFO_DIR "/tcptap-swtchbrd_",
    .nic_name = "127.0.0.1",
/* *INDENT-ON* */
};

void env_int(void)
{
    SETFROMENV(TCP_TAP_EXEC, env.execute_bin, NAME_MAX);
    SETFROMENV(TCP_TAP_PORT, env.port, NAME_MAX);
    SETFROMENV(TCP_TAP_NICNAME, env.nic_name, NAME_MAX);
    SETFROMENV(TCP_TAP_FIFO_PRE_NAME, env.fifo_prename, NAME_MAX);
}

struct env *env_get()
{
    return &env;
}
