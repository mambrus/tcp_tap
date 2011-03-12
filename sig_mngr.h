#ifndef sig_mngr_h
#define sig_mngr_h

#include <unistd.h>

int sig_mngr_init(pid_t child_pid, int log_fd);


#endif
