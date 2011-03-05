#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "switchboard.h"

#undef  NDEBUG
#include <assert.h>

/* The maximum numbers of arguments in chaild we handle*/
#define MAX_ARGS 50

/* The size of each buffer used for tranfer in either direction */
#ifndef BUFF_SZ
#define BUFF_SZ 0x400
#endif

/* flags for all the logs*/
//#define LFLAGS ( O_WRONLY | O_APPEND | O_CREAT /*| O_CLOEXEC |*/ O_SYNC )
#define LFLAGS ( O_WRONLY | O_APPEND | O_CREAT | O_SYNC )

/* modes for all the logs */
#define LMODES 0777

/* Environment overloadable variables */

/* stdin,stdout,stderr for the native process */
char stdin_name[PATH_MAX]="/tmp/tcp_tap_stdin";
char stdout_name[PATH_MAX]="/tmp/tcp_tap_stdout";
char stderr_name[PATH_MAX]="/tmp/tcp_tap_stderr";

/* Some log-files */
char parent_log_name[PATH_MAX]="/tmp/tcp_tap_parent.log";
char child_log_name[PATH_MAX]="/tmp/tcp_tap_child.log";	//stderr for the child

/* Name of the main process to run */
char execute_bin[PATH_MAX]="/skiff/bin/arm-hixs-elf-gdb";

/* Port number */
char port[PATH_MAX]="6969";

#define SETFROMENV( envvar, locvar, buf_max)				\
{															\
	char *ts;												\
	if ((ts=getenv(#envvar)) != NULL ) {					\
		int l;												\
		memset(locvar,0,buf_max);							\
		l=strnlen(ts,buf_max);								\
		memcpy(locvar,ts,l<buf_max?l:buf_max);				\
	}														\
}

/* Transfer types */
struct data_link {
	int read_from;
	int write_to;
	int log_to;
	char *buffer;
};

/* Threads handing shuffling of data */

/* Transfers stdin and sends to child (and TCP sockes, if any) */
void *to_child(void *arg){
	int i,j,wfd;
	struct data_link *lp = (struct data_link *)arg;

	assert((wfd=open(Q_TO_SWTCH,O_WRONLY)) >=0);

	while (1){
		i=read(lp->read_from, lp->buffer, BUFF_SZ);
		if (i<0) {
			perror("Failed reading from pipe");
			exit(-1);
		}
		j=write(lp->write_to, lp->buffer, i);
		assert(i==j);
		j=write(lp->log_to, lp->buffer, i);
		assert(i==j);
		j=write(wfd, lp->buffer, i);
		assert(i==j);
	}
	return NULL;
}

/* Transfers output from child and sends to stdout(and TCP socket, if any) */
void *to_parent(void *arg){
	int i,j,wfd;
	struct data_link *lp = (struct data_link *)arg;

	assert((wfd=open(Q_TO_SWTCH,O_WRONLY)) >=0);

	while (1){
		i=read(lp->read_from, lp->buffer, BUFF_SZ);
		if (i<0) {
			perror("Failed reading from pipe");
			exit(-1);
		}
		j=write(lp->write_to, lp->buffer, i);
		assert(i==j);
		j=write(lp->log_to, lp->buffer, i);
		assert(i==j);
		j=write(wfd, lp->buffer, i);
		assert(i==j);
	}
	return NULL;
}

/* Transfers from TCP and sends to child */
void *from_tcp(void *arg){
	int i,j,rfd;
	struct data_link *lp = (struct data_link *)arg;
	char tbuf[BUFF_SZ];

	assert((rfd=open(Q_FROM_SWTCH,O_RDONLY)) >=0);

	while (1){
		i=read(rfd, lp->buffer, BUFF_SZ);
		/*
		i--;
		lp->buffer[i]=0;
		lp->buffer[i-1]='\n';
		*/
		if (i<0) {
			perror("Failed reading from TCP");
			exit(-1);
		}
		j=write(lp->write_to, lp->buffer, i);
		assert(i==j);
		j=write(lp->log_to, lp->buffer, i);
		assert(i==j);
	}
	return NULL;
}

int main(int argc, char **argv) {
	int pipe_to_child[2];
	int pipe_to_parent[2];
	int parent_log_fd, child_err_fd;
	int stdinlog_fd, stdoutlog_fd, stderrlog_fd;
	char tname[PATH_MAX];
	char *exec_args[MAX_ARGS];
	char buf_to_child[BUFF_SZ];
	char buf_to_parent[BUFF_SZ];
	int pid,i,j,k;
	pthread_t pt_to_child;
	pthread_t pt_to_parent;
	pthread_t pt_from_tcp;
	struct data_link link_to_child;
	struct data_link link_to_parent;

	assert(argc<MAX_ARGS);

	SETFROMENV( TCP_TAP_EXEC,		execute_bin,		PATH_MAX);
	SETFROMENV( TCP_TAP_PORT,		port,				PATH_MAX);
	SETFROMENV( TCP_TAP_LOG_STDIN,	stdin_name,			PATH_MAX);
	SETFROMENV( TCP_TAP_LOG_STDOUT,	stdout_name,		PATH_MAX);
	SETFROMENV( TCP_TAP_LOG_STDERR,	stderr_name,		PATH_MAX);
	SETFROMENV( TCP_TAP_LOG_PARENT,	parent_log_name,	PATH_MAX);
	SETFROMENV( TCP_TAP_LOG_CHILD,	child_log_name,		PATH_MAX);

	assert((stdinlog_fd=open(stdin_name, LFLAGS, LMODES)) > 0);
	assert((stdoutlog_fd=open(stdout_name, LFLAGS, LMODES)) > 0);
	assert((stderrlog_fd=open(stderr_name, LFLAGS, LMODES)) > 0);
	assert((child_err_fd=open(child_log_name, LFLAGS, LMODES)) > 0);
	assert((parent_log_fd=open(parent_log_name, LFLAGS, LMODES)) > 0);

	pipe(pipe_to_child);
	pipe(pipe_to_parent);

	memset(exec_args,0,MAX_ARGS); /* Makes sure to null terminate arg-list */
	exec_args[0]=execute_bin;
	for (i=1;i<argc;i++) {
		exec_args[i]=argv[i];
	}

	pid = fork();

	if (pid == 0) {
		/* Child excutes this */
		sprintf(buf_to_child, "Child will execute:\n");
		k=write(child_err_fd, buf_to_child, strnlen(buf_to_child,BUFF_SZ));
		for (i=0;i<argc;i++) {
			j=snprintf(buf_to_child, BUFF_SZ, "%s\n", exec_args[i]);
			k=write(child_err_fd, buf_to_child, strnlen(buf_to_child,BUFF_SZ));
			if (i)
				assert(k==j);
		}
		sprintf(buf_to_child,"=========X=========X=========X=========X\n");
		write(child_err_fd, buf_to_child, strnlen(buf_to_child,BUFF_SZ));
		sprintf(buf_to_child, "This is now stderr:\n");
		k=write(child_err_fd, buf_to_child, strnlen(buf_to_child,BUFF_SZ));
		sprintf(buf_to_child,"=========X=========X=========X=========X\n");
		write(child_err_fd, buf_to_child, strnlen(buf_to_child,BUFF_SZ));

		close(0);
		dup(pipe_to_child[0]);

		close(1);
		dup(pipe_to_parent[1]);

		close(2);
		dup(child_err_fd);

		close(pipe_to_child[0]);
		close(pipe_to_child[1]);
		close(pipe_to_parent[0]);
		close(pipe_to_parent[1]);

		execv(execute_bin,exec_args);

		/* Should never execute */
		perror("exec error");
		exit(-1);
  	}

	/* Parent executes this */
	k=sprintf(buf_to_parent,"Parent handles execution of:\n");
	k=write(parent_log_fd, buf_to_parent, strnlen(buf_to_parent,BUFF_SZ));
	for (i=0;i<argc;i++) {
		j=snprintf(buf_to_parent, BUFF_SZ, "%s\n", exec_args[i]);
		k=write(parent_log_fd, buf_to_parent, strnlen(buf_to_parent,BUFF_SZ));
			assert(k==j);
	}
	sprintf(buf_to_parent,"=========Y=========Y=========Y=========Y\n");
	write(parent_log_fd, buf_to_parent, strnlen(buf_to_parent,BUFF_SZ) );

	link_to_child.read_from=0;
	link_to_child.write_to=pipe_to_child[1];
	close(pipe_to_child[0]);
	link_to_child.log_to=stdinlog_fd;
	link_to_child.buffer=buf_to_child;

	link_to_parent.read_from=pipe_to_parent[0];
	close(pipe_to_parent[1]);
	link_to_parent.write_to=1;
	link_to_parent.log_to=stdoutlog_fd;
	link_to_parent.buffer=buf_to_parent;

	assert (pthread_create(&pt_to_child,  NULL, to_child,  &link_to_child) == 0);
	assert (pthread_create(&pt_to_parent, NULL, to_parent, &link_to_parent) == 0);
	switchboard_init(atoi(port),"localhost",1);
	assert (pthread_create(&pt_from_tcp, NULL, from_tcp, &link_to_child) == 0);
	
	assert (pthread_join(pt_to_parent, NULL) == 0);
	assert (pthread_join(pt_to_child, NULL) == 0);
	while ( wait((int*)0) != pid )
		;
	close(parent_log_fd);
	close(child_err_fd);
	close(stdinlog_fd);
	close(stdoutlog_fd);
	close(stderrlog_fd);
	close(pipe_to_child[1]);
	close(pipe_to_parent[0]);
	return 0;
}
