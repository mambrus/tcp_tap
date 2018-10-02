#ifndef tcptap_local_h
#define tcptap_local_h

/* Stringify */
#define _XSTR( X ) #X
#define STR( X ) _XSTR( X )

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
