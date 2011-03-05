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

struct threads_t {
	pthread_t			to_swtch;
	pthread_t			mngmt;
} threads;

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

void * switchboard_start_mngmt(void *S){
	int s = (int)S;
	int fd;
	struct serv_node *tn,**lnp,*lp;


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

}

/* Just echo back everything */
int switchboard_init(int port, const char *host, int echo){
	int s;

	mkfifo(Q_TO_SWTCH,0777);
	mkfifo(Q_FROM_SWTCH,0777);


	s=init_switchboard(port,host,echo);
	//ss.ea = echo;
	//s=init_server(6666,"localhost");
	//ss.s=s;

	assert (pthread_create(&threads.to_swtch,  NULL, to_swtch_thread,  NULL) == 0);
	assert (pthread_create(&threads.mngmt,  NULL, switchboard_start_mngmt, (void*)s ) == 0);
	return s;
}

void switchboard_die(int s) {
	struct serv_node *ln=ss.serv_list;

	pthread_cancel(threads.mngmt);
	pthread_cancel(threads.to_swtch);

	if (ss.ea) {
		while (ln) {
			pthread_cancel(ln->thread);
			assert(close(ln->fd) == 0);
			ln=ln->next;
		}
	}

	assert(close(s) == 0);
}

#ifdef TEST_SWITCH

void *from_stdin(void *arg){
	int rn,sn;
	int wfd;
	char buf[BUFF_SZ];

	usleep(100000);
	assert((wfd=open(Q_TO_SWTCH,O_WRONLY)) >=0);

	while(1){
		rn=read(1,buf,BUFF_SZ);
		sn=write(wfd,buf,rn);
		assert(rn==sn);
	}
}

void *to_stdout(void *arg){
	int rn,sn;
	int rfd;
	char buf[BUFF_SZ];

	usleep(100000);
	assert((rfd=open(Q_FROM_SWTCH,O_RDONLY)) >=0);

	while(1){
		rn=read(rfd,buf,BUFF_SZ);
		sn=write(2,buf,rn);
		assert(rn==sn);
	}
}

int main(int argc, char **argv) {
	pthread_t thread1,thread2;

	assert (pthread_create(&thread1,  NULL, from_stdin,  NULL) == 0);
	assert (pthread_create(&thread2,  NULL, to_stdout,  NULL) == 0);

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
