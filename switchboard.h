#ifndef switchboard_h
#define switchboard_h

/* Returns server socket, close this to close all servlets */
int init_switchboard(int port, const char *hostname, int echoall);

#endif
