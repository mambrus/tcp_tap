#ifndef server_h
#define server_h
/* Returns a valid socket fd on connection */
int init_server(int port, const char *hostname);
int open_server(int s);
#endif
