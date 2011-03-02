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

#define BACKLOG 5


#ifdef TEST
/* Environment overloadable variables */

/* Port number */
char port_number[PATH_MAX]="6666";
#endif //TEST

/* The size of each buffer used for tranfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

/* Returns a valid socket fd on connection */
int open_server(int port, const char *hostname){
	int s,sfd,fromlen;
	char name[PATH_MAX];
	struct hostent *hp;
	struct sockaddr_in lsin, rsin;

	
	if (!hostname)
		assert(gethostname(name, sizeof(name) > 0));
	else
		strncpy(name,hostname,PATH_MAX);

	assert( (hp = gethostbyname(name)) != NULL );

	assert( (s=socket(AF_INET, SOCK_STREAM,0)) >= 0 );
	lsin.sin_family = AF_INET;
	lsin.sin_port=htons(port);
	memcpy(&lsin.sin_addr,hp->h_addr,hp->h_length);
	assert(bind(s, (struct sockaddr *)&lsin, sizeof(lsin) ) >= 0);

	assert(listen(s, BACKLOG) >= 0);

	sfd = accept(s,(struct sockaddr *)&rsin,&fromlen);
	return sfd;
}


#ifdef TEST

void *myThread(void *inarg){
	int rn, sn;
	int fd = (int)inarg;
	while(1) {
		rn=read(fd,buf,BUFF_SZ);
		sn=write(fd,buf,rn);
		assert(rn==sn);
	}
}

/* Just echo back everything */
int main(int argc, char **argv) {
	int fd;
	int port;

	port=atoi(port_number);
	char buf[BUFF_SZ];

	while(1) {
		fd=open_server(port,"localhost");
		assert (pthread_create(&t_thread,  NULL, myThread,  fd) == 0);
	}
	
	return 0;
}
#endif //TEST
