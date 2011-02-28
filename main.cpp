#include <stdio.h>
#include <limits.h>
#include <unistd.h>

/* Where to put intermediate files and pipes */
char log_path[PATH_MAX]="/tmp/tcp_tap";

/* stdin,stdout,stderr for the native process */
char stdin_name[PATH_MAX]="stdin";
char stdout_name[PATH_MAX]="stdout";
char stderr_name[PATH_MAX]="stderr";

/* Name of the main process to run */
char stdout_err[PATH_MAX]="stderr";

int main(int argc, char **argv) {
  int myPipe[2];
  
  pipe(myPipe);

  id (fork() == 0){
	  /*Child*/
  }

  return 0;
}
