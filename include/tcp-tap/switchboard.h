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

/* Returns server socket, close this to close all servlets */
int switchboard_init(int port, const char *host, int echo);
void switchboard_die(int s);

#ifdef ANDROID
#  define Q_TO_SWTCH "/data/local/tmp/q_to_swtch"
#  define Q_FROM_SWTCH "/data/local/tmp/q_from_swtch"
#else
#  define Q_TO_SWTCH "/tmp/q_to_swtch"
#  define Q_FROM_SWTCH "/tmp/q_from_swtch"
#endif

#endif
