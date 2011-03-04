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

#ifdef TEST2
/* Environment overloadable variables */

/* Port number */
char port_number[PATH_MAX]="6666";
#endif //TEST

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
	int s;

	ss.ea = echoall;
	s=init_server(port,hostname);
	ss.s=s;
	return s;
}

static void write_toall(const char *buf, int len) {
	int sn;

	struct serv_node *ln=ss.serv_list;
	if (ss.ea) {
		while (ln) {
			sn=write(ln->fd,buf,len);
			assert(len==sn);
			ln=ln->next;
		}
	}
}

static void *shuffleThread(void *inarg){
	int rn, sn;
	int fdo,fd = (int)inarg;
	char buf[BUFF_SZ];
	assert((fdo=open(Q_FROM_SWTCH,O_WRONLY)) >=0);

	while(1) {
		rn=read(fd,buf,BUFF_SZ);
		write_toall(buf,rn);
		write(fdo,buf,rn);
		assert(sn=rn);
	}
}

void *to_swtch_thread(void *arg) {
	char buf[BUFF_SZ];
	int rn,fd;

	assert((fd=open(Q_TO_SWTCH,O_RDONLY)) >=0);

	while (1) {
		rn=read(fd,buf,BUFF_SZ);
		write_toall(buf,rn);
	}
	return NULL;
}


/* Just echo back everything */
int switchboard_start(int port, const char *host, int echo){
	int fd,s;
	struct serv_node *tn,**lnp,*lp;
	pthread_t			to_swtch;

	assert (pthread_create(&to_swtch,  NULL, to_swtch_thread,  NULL) == 0);
	
	s=init_switchboard(port,host,echo);
	//ss.ea = echo;
	//s=init_server(6666,"localhost");
	//ss.s=s;

	while(1) {
		assert((fd=open_server(s)) >= 0);
		tn=malloc(sizeof(struct serv_node));
		tn->fd=fd;
		tn->next=NULL;

		lnp=&ss.serv_list;
		while(*lnp){
			lp=(struct serv_node*)(*lnp);
			lnp=&(lp->next);
			//lnp=&(*((struct serv_node*)(*lnp)).next);
		}

		*lnp=tn;
		
		ss.n++;
		assert (pthread_create(&tn->thread,  NULL, shuffleThread,  (void*)fd) == 0);
		//sleep(10);
	}
	
	return 0;
}

#ifdef TEST_SWITCH
int main(int argc, char **argv) {
	int rfd,wfd;

	mkfifo(Q_TO_SWTCH,0777);
	mkfifo(Q_FROM_SWTCH,0777);

	assert((rfd=open(Q_FROM_SWTCH,O_RDONLY)) >=0);
	assert((wfd=open(Q_TO_SWTCH,O_WRONLY)) >=0);

	close(0);
	dup(rfd);

	close(1);
	dup(wfd);


	switchboard_start(6666,"localhost",1);

	return 0;
}
#endif //TEST_SWITCH

#ifdef TEST2

void *myThread(void *inarg){
	int rn, sn;
	int fd = (int)inarg;
	char buf[BUFF_SZ];
	while(1) {
		rn=read(fd,buf,BUFF_SZ);
		sn=write(fd,buf,rn);
		assert(rn==sn);
	}
}

/* Just echo back everything */
int main(int argc, char **argv) {
	int fd,s;
	int port;
	pthread_t t_thread;

	port=atoi(port_number);
	char buf[BUFF_SZ];

	s=init_server(port,"localhost");
	while(1) {
		fd=open_server(s);
		assert (pthread_create(&t_thread,  NULL, myThread,  (void*)fd) == 0);
		//sleep(10);
	}
	
	return 0;
}
#endif //TEST2
