#ifndef tcptap_local_h
#define tcptap_local_h
#include <limits.h>

/* Stringify */
#define _XSTR( X ) #X
#define STR( X ) _XSTR( X )

/* Environment overloadable variables. Note: NEVER change these in code
 * unless bug is found as they are considered trusted and safe, and as they
 * will allow tcp_tap to start without any wrapping scripts (important when
 * debugging and testing). Always use the corresponding environment variable
 * */
struct env {
/* Name of the main process to run */
    char execute_bin[NAME_MAX];

/* Port number */
    char port[NAME_MAX];

/* FIFO(s) pre-name */
    char fifo_prename[NAME_MAX];

/* listen at NIC bound to this name (human readable name or
 * IP-address). Aditionaly two special names:
 * @HOSTNAME@: Look up the primary interface bound to this name
 * @ANY@: Allow connection to any of the servers IF
 * */
    char nic_name[NAME_MAX];
};

/* Transfer between two fd end-poins */
struct data_link {
    int read_from;
    int write_to;
    int enable_log;
    const char *worker_name;
    char *buffer;
};

typedef enum {
    to_child = 0,
    to_parent,
    from_tcps
} links_t;

struct link {
    pthread_t thid;
    void *(*worker) (void *);
    struct data_link *data_link;
};

void env_int(void);
struct env *env_get();
void *thread_to_child(void *arg);
void *thread_to_parent(void *arg);
void *thread_from_tcps(void *arg);
struct link *link_create(void *(*worker) (void *), int fd_from, int fd_to);
void link_kill(struct link *);

#ifdef __ANDROID__

/*  <android/log.h> is part of NDK <cutils/log.h> is not. <android/log.h>
    however lacks the LOGX macros */

#include <android/log.h>

#ifndef  LOG_TAG
#define  LOG_TAG "tcp-tap"
#endif

#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#else

#include <liblog/log.h>

#endif                          // __ANDROID__

#endif                          //tcptap_local_h
