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

#undef  NDEBUG
#include <assert.h>

/* The maximum numbers of arguments in chaild we handle*/
#define MAX_ARGS 50

/* The size of each buffer used for tranfer in either direction */
#define BUFF_SZ 0x400

/* flags for all the logs*/
#define LFLAGS ( O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_SYNC )

/* modes for all the logs */
#define LMODES 0777


/* Reason for the below names to be arrays and not defines is that they are to be
 * read from environment variables in future*/

/* Where to put intermediate files and pipes */
char log_path[PATH_MAX]="/tmp/tcp_tap";

/* stdin,stdout,stderr for the native process */
char stdin_name[PATH_MAX]="stdin";
char stdout_name[PATH_MAX]="stdout";
char stderr_name[PATH_MAX]="stderr";

/* Name of the main process to run */
char execute_bin[PATH_MAX]="/skiff/bin/arm-hixs-elf-gdb";

/* Some log-files */
char parent_log_name[PATH_MAX]="parent.log";
char child_log_name[PATH_MAX]="child.log";	//stderr for the child piped here

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
	int i,j;
	struct data_link *lp = (struct data_link *)arg;

	while (1){
		i=read(lp->read_from, lp->buffer, BUFF_SZ);
		if (i<0) {
			perror("Failed reading from link");
			exit(-1);
		}
		j=write(lp->write_to, lp->buffer, i);
		assert(i==j);
		j=write(lp->log_to, lp->buffer, i);
		assert(i==j);
	}
	return NULL;
}

/* Transfers output from child and sends to stdout(and TCP socket, if any) */
void *to_parent(void *arg){
	int i,j;
	struct data_link *lp = (struct data_link *)arg;

	while (1){
		i=read(lp->read_from, lp->buffer, BUFF_SZ);
		if (i<0) {
			perror("Failed reading from link");
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
	struct data_link link_to_child;
	struct data_link link_to_parent;

	assert(argc<MAX_ARGS);

	/* Ignore any error when creating tempdir*/
	mkdir(log_path, 0777);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,stdin_name);
	assert((stdinlog_fd=open(tname, LFLAGS, LMODES)) > 0);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,stdout_name);
	assert((stdoutlog_fd=open(tname, LFLAGS, LMODES)) > 0);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,stderr_name);
	assert((stderrlog_fd=open(tname, LFLAGS, LMODES)) > 0);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,child_log_name);
	assert((child_err_fd=open(tname, LFLAGS, LMODES)) > 0);

	snprintf(tname,PATH_MAX,"%s/%s",log_path,parent_log_name);
	assert((parent_log_fd=open(tname, LFLAGS, LMODES)) > 0);

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
