/* TCP-TA§P config file */
#ifndef tcp_tap_config_h
#define tcp_tap_config_h

#define PROJ_NAME "tcp-tap"
#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0
#define VERSION "0.1.0"

/* Defaults to corresponding environment variables */
#define TCP_TAP_EXEC /bin/sh

/* Target system supports libraries */
/* #undef HAVE_LIB_RT */
/* #undef HAVE_LIB_MQUEUE */
/* #undef HAVE_LIB_PTHREAD */

/* Target system supports functions */
/* #undef HAVE_PTHREAD_CANCEL_F */
/* #undef HAVE_PTHREAD_CREATE_F */

/* Target system supports symbols */
/* #undef HAVE_PTHREAD_CANCEL_S */
/* #undef HAVE_PTHREAD_CREATE_S */

#if !defined(HAVE_PTHREAD_CANCEL_S) && !defined(HAVE_PTHREAD_CANCEL_S)
/* Work-around for (especially) Android targets */

static int _simulated_pthread_cancel(pthread_t thread)
{
    return pthread_kill(thread, SIGUSR1);
}

#define pthread_cancel _simulated_pthread_cancel

#endif

#endif                          //tcp_tap_config_h
