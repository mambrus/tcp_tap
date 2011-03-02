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
	int port;
	char hostname[PATH_MAX];
	struct hostent *hp;

	port=atoi(port_number);
	
	assert(gethostname(hostname, sizeof(hostname) > 0));
	sprintf(hostname,"%s","mambrus-laptop");

	assert((hp = gethostbyname(hostname)) != NULL);

	printf("Hostname: [%s], Port: [%d]\n",hostname,port);
	return 0;
}
#endif //TEST
