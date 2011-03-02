#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#undef  NDEBUG
#include <assert.h>


#ifdef TEST
int main(int argc, char **argv) {
	printf("Hello world\n");
	return 0;
}
#endif //TEST
