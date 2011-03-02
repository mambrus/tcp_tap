#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>

#undef  NDEBUG
#include <assert.h>
#include "switchboard.h"

/* The size of each buffer used for tranfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

struct serv_node {
	int					fd;		/* File descriptor */
	struct serv_node*	next;
	pthread_t			thread;
};

struct switch_struct {
	int					s;
	int					n;     /* Number of connections */
	int					ea;    /* Do echo to all */
	struct serv_node	*serv_list;
};

struct switch_struct ss = {
	.s			= 0,
	.n			= 0,
	.ea			= 0,
	.serv_list	= NULL
};

int init_switchboard(int port, const char *hostname, int echoall){
	ss.ea = echoall;
	ss.s=init_server(port,"localhost");
	return ss.s;
}

static void *myThread(void *inarg){
	int rn, sn;
	int fd = (int)inarg;
	char buf[BUFF_SZ];
	while(1) {
		struct serv_node *ln=ss.serv_list;
		rn=read(fd,buf,BUFF_SZ);
		if (ss.ea) {
			while (ln->next) {
				sn=write(ln->fd,buf,rn);
				assert(rn==sn);
				ln=ln->next;
			}
		}
	}
}

#ifdef TEST_SWITCH

/* Just echo back everything */
int main(int argc, char **argv) {
	int fd,s;
	struct serv_node *tn,*ln;

	s=init_switchboard(6666,"localhost",1);
	while(1) {
		fd=open_server(s);
		tn=malloc(sizeof(struct serv_node));
		tn->next=NULL;

		ln=ss.serv_list;
		while(ln->next)
			ln=ln->next;

		ln->next=tn;

		ss.n++;
		assert (pthread_create(&ln->thread,  NULL, myThread,  (void*)fd) == 0);
		//sleep(10);
	}
	
	return 0;
}
#endif //TEST_SWITCH
