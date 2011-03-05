#ifndef switchboard_h
#define switchboard_h

#include <mqueue.h>

/* Returns server socket, close this to close all servlets */
int switchboard_init(int port, const char *host, int echo);
void switchboard_die(int s);

#define Q_TO_SWTCH "/tmp/q_to_swtch"
#define Q_FROM_SWTCH "/tmp/q_from_swtch"

#endif
