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
#include <signal.h>
#include <errno.h>
#include "sig_mngr.h"
#include "local.h"

#undef  NDEBUG
#include <assert.h>
#include "sig_mngr.h"
#include "tcp-tap_config.h"

/* Install signal-handler for every imaginable signal. If tcp_tap receives any,
 * its default behaviour is to forward it to the child, unless corresponding
 * environment variable says differently.
 */

static pid_t _pid_child = -1;
#define DEF_FUN(X)                                          \
    void mngSig_##X( int sig ) {                            \
        signal( X, SIG_IGN);                                \
        LOGW("Forwarding signal [" #X "]\n");               \
        kill(_pid_child,sig);                               \
        signal( X, mngSig_##X );                            \
    }

DEF_FUN(SIGHUP)
    DEF_FUN(SIGINT)
    DEF_FUN(SIGQUIT)
    DEF_FUN(SIGILL)
    DEF_FUN(SIGTRAP)
    DEF_FUN(SIGABRT)
    DEF_FUN(SIGIOT)
    DEF_FUN(SIGBUS)
    DEF_FUN(SIGFPE)
    DEF_FUN(SIGKILL)
    DEF_FUN(SIGUSR1)
    DEF_FUN(SIGSEGV)
    DEF_FUN(SIGUSR2)
    DEF_FUN(SIGPIPE)
    DEF_FUN(SIGALRM)
    DEF_FUN(SIGTERM)
    DEF_FUN(SIGSTKFLT)
    DEF_FUN(SIGCHLD)
    DEF_FUN(SIGCONT)
    DEF_FUN(SIGSTOP)
    DEF_FUN(SIGTSTP)
    DEF_FUN(SIGTTIN)
    DEF_FUN(SIGTTOU)
    DEF_FUN(SIGURG)
    DEF_FUN(SIGXCPU)
    DEF_FUN(SIGXFSZ)
    DEF_FUN(SIGVTALRM)
    DEF_FUN(SIGPROF)
    DEF_FUN(SIGWINCH)
    DEF_FUN(SIGIO)
    DEF_FUN(SIGPOLL)
    DEF_FUN(SIGPWR)
    DEF_FUN(SIGSYS)
#ifdef HAVE_SIGUNUSED
    DEF_FUN(SIGUNUSED)
#endif
    DEF_FUN(SIGRTMIN)
    DEF_FUN(SIGRTMAX)
//DEF_FUN(SIGSWI)
    DEF_FUN(SIGSTKSZ)

/* Install sighandler conditioned by environment variable
 * (will be conditional, for now: unconditional)
 */
#define COND_SIGHNDLR_INSTALL(X)                            \
{                                                           \
    signal( X , mngSig_##X );                               \
}
int sig_mngr_init(pid_t pid_child)
{
    _pid_child = pid_child;

    COND_SIGHNDLR_INSTALL(SIGHUP);
    COND_SIGHNDLR_INSTALL(SIGINT);
    COND_SIGHNDLR_INSTALL(SIGQUIT);
    COND_SIGHNDLR_INSTALL(SIGILL);
    COND_SIGHNDLR_INSTALL(SIGTRAP);
    COND_SIGHNDLR_INSTALL(SIGABRT);
    COND_SIGHNDLR_INSTALL(SIGIOT);
    COND_SIGHNDLR_INSTALL(SIGBUS);
    COND_SIGHNDLR_INSTALL(SIGFPE);
    COND_SIGHNDLR_INSTALL(SIGKILL);
    COND_SIGHNDLR_INSTALL(SIGUSR1);
    COND_SIGHNDLR_INSTALL(SIGSEGV);
    COND_SIGHNDLR_INSTALL(SIGUSR2);
    COND_SIGHNDLR_INSTALL(SIGPIPE);
    COND_SIGHNDLR_INSTALL(SIGALRM);
    COND_SIGHNDLR_INSTALL(SIGTERM);
    COND_SIGHNDLR_INSTALL(SIGSTKFLT);
    COND_SIGHNDLR_INSTALL(SIGCHLD);
    COND_SIGHNDLR_INSTALL(SIGCONT);
    COND_SIGHNDLR_INSTALL(SIGSTOP);
    COND_SIGHNDLR_INSTALL(SIGTSTP);
    COND_SIGHNDLR_INSTALL(SIGTTIN);
    COND_SIGHNDLR_INSTALL(SIGTTOU);
    COND_SIGHNDLR_INSTALL(SIGURG);
    COND_SIGHNDLR_INSTALL(SIGXCPU);
    COND_SIGHNDLR_INSTALL(SIGXFSZ);
    COND_SIGHNDLR_INSTALL(SIGVTALRM);
    COND_SIGHNDLR_INSTALL(SIGPROF);
    COND_SIGHNDLR_INSTALL(SIGWINCH);
    COND_SIGHNDLR_INSTALL(SIGIO);
    COND_SIGHNDLR_INSTALL(SIGPOLL);
    COND_SIGHNDLR_INSTALL(SIGPWR);
    COND_SIGHNDLR_INSTALL(SIGSYS);
#ifdef HAVE_SIGUNUSED
    COND_SIGHNDLR_INSTALL(SIGUNUSED);
#endif
    COND_SIGHNDLR_INSTALL(SIGRTMIN);
    COND_SIGHNDLR_INSTALL(SIGRTMAX);
    //COND_SIGHNDLR_INSTALL(SIGSWI);
    COND_SIGHNDLR_INSTALL(SIGSTKSZ);

    return 0;
}

/*
SIGHUP
SIGINT
SIGQUIT
SIGILL
SIGTRAP
SIGABRT
SIGIOT
SIGBUS
SIGFPE
SIGKILL
SIGUSR1
SIGSEGV
SIGUSR2
SIGPIPE
SIGALRM
SIGTERM
SIGSTKFLT
SIGCHLD
SIGCONT
SIGSTOP
SIGTSTP
SIGTTIN
SIGTTOU
SIGURG
SIGXCPU
SIGXFSZ
SIGVTALRM
SIGPROF
SIGWINCH
SIGIO
SIGPOLL
SIGPWR
SIGSYS
SIGUNUSED
SIGRTMIN
SIGRTMAX
SIGSWI
SIGSTKSZ
 */
