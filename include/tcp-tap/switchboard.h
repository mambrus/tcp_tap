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
#ifndef switchboard_h
#define switchboard_h

struct switch_fifo {
    char *in_name;
    char *out_name;
};

/* Returns server socket close this to close all servlets
 *
 * fifo_prename is an optional identifier string. If NULL, a default will be
 * used */
int switchboard_init(int port, const char *host, int echo,
                     const char *fifo_prename);
void switchboard_die(int s);

/* Get full path/suffix FIFO-names in use to/from switch-board */
struct switch_fifo *switchboard_fifo_names();

#ifdef ANDROID
#  define FIFO_DIR "/data/local/tmp"
#else
#  define FIFO_DIR "/tmp"
#endif

#endif
