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


#ifdef TEST
/* Environment overloadable variables */

/* Port number */
char port_number[PATH_MAX]="6666";
#endif //TEST


#ifdef TEST
int main(int argc, char **argv) {
	int port,s,sfd;
	char hostname[PATH_MAX];
	struct hostent *hp;
	struct sockaddr_in lsin, rsin;

	port=atoi(port_number);
	
	assert(gethostname(hostname, sizeof(hostname) > 0));
	sprintf(hostname,"%s","mambrus-laptop");

	assert( (hp = gethostbyname(hostname)) != NULL );

	assert( (s=socket(AF_INET, SOCK_STREAM,0)) >= 0 );
	lsin.sin_family = AF_INET;
	lsin.sin_port=htons(port);
	memcpy(&lsin.sin_addr,hp->h_addr,hp->h_length);
	assert(bind(s, &lsin, sizeof(lsin) ) >= 0);

	assert(listen(s, 5) >= 0);

	sfd = accept(s,&rsin
	

	printf("Hostname: [%s], Port: [%d]\n",hostname,port);
	return 0;
}
#endif //TEST
