#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#undef  NDEBUG
#include <assert.h>

/* The maximum numbers of arguments in chaild we handle*/
#define MAX_ARGS 50

/* The size of each buffer used for tranfer in either direction */
#define BUFF_SZ 0x400

/* flags for all the logs*/
#define LFLAGS ( O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_SYNC )


/* Reason for the below names to be arrays and not defines is that they are to be
 * read from environment variables in future*/

/* Where to put intermediate files and pipes */
char log_path[PATH_MAX]="/tmp/tcp_tap";

/* stdin,stdout,stderr for the native process */
char stdin_name[PATH_MAX]="stdin";
char stdout_name[PATH_MAX]="stdout";
char stderr_name[PATH_MAX]="stderr";

/* Name of the main process to run */
char execute_bin[PATH_MAX]="arm-eabi-gdb";

/* Some log-files */
char parent_log_name[PATH_MAX]="parent.log";
char child_log_name[PATH_MAX]="child.log";    //stderr for the child piped here

/* Transfer types */
struct data_link {
    int read_from;
    int send_to;
    char *buffer;
};

/* Threads handing shuffling of data */

/* Transfers stdin and sends to child (and TCP sockes, if any) */
void *to_child(void *){
    return NULL;
}

/* Transfers output from child and sends to stdout(and TCP socket, if any) */
void *to_parent(void *){
    return NULL;
}

int main(int argc, char **argv) {
	int pipefd[2];
	int parent_log_fd, child_err_fd;
	int stdinlog_fd, stdoutlog_fd, stderrlog_fd;
	char tname[PATH_MAX];
	char *exec_args[MAX_ARGS];
	char buf_to_child[BUFF_SZ];
	char buf_to_parent[BUFF_SZ];
	int i,j,k;
	pthread_t pt_to_child;
	pthread_t pt_to_parent;
	struct data_link link_to_child;
	struct data_link link_to_parent;

	assert(argc<MAX_ARGS);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,stdin_name);
	assert(stdinlog_fd=open(tname,LFLAGS) > 0);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,stdout_name);
	assert(stdoutlog_fd=open(tname,LFLAGS) > 0);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,stderr_name);
	assert(stderrlog_fd=open(tname,LFLAGS) > 0);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,child_log_name);
	assert(child_err_fd=open(tname,LFLAGS) > 0);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,parent_log_name);
	assert(parent_log_fd=open(tname,LFLAGS) > 0);

	pipe(pipefd);

	memset(exec_args,0,MAX_ARGS); /* Makes sure to null terminate arg-list */
	exec_args[0]=execute_bin;
	for (i=1;i<argc;i++) {
		exec_args[i]=argv[i];
		j=snprintf(buf_to_parent, BUFF_SZ, "%s\n", argv[i]);
		assert(j==(signed)(strnlen(argv[i]-1,BUFF_SZ)));
		k=write(parent_log_fd, buf_to_parent,BUFF_SZ);
		assert(k==(signed)(strnlen(argv[i]-1,BUFF_SZ)));
	}

	if (fork() == 0) {
		/* Child excutes this */

	    	close(0);
		dup(pipefd[0]);

		close(1);
		dup(pipefd[1]);

		close(2);
		dup(child_err_fd);

		execv(execute_bin,exec_args);

		/* Should never execute */
		perror("exec error");
		exit(-1);
  	}
	link_to_child.read_from=0;
	link_to_child.send_to=0;
	link_to_child.buffer=NULL;

	link_to_parent.read_from=0;
	link_to_parent.send_to=0;
	link_to_parent.buffer=NULL;

	assert (pthread_create(&pt_to_child,  NULL, to_child,  &link_to_child) == 0);
	assert (pthread_create(&pt_to_parent, NULL, to_parent, &link_to_parent) == 0);
	assert (pthread_join(pt_to_parent, NULL) == 0);
	assert (pthread_join(pt_to_child, NULL) == 0);
	return 0;
}
