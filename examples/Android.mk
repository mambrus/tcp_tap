LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := tcptap-client
LOCAL_MODULE_TAGS := debug eng
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_SRC_FILES := client.c
LOCAL_STATIC_LIBRARIES := libatcptap
LOCAL_SHARED_LIBRARIES:= liblog
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)

LOCAL_MODULE := tcptap-echo
LOCAL_MODULE_TAGS := debug eng
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_SRC_FILES := echo_multi.c
LOCAL_STATIC_LIBRARIES := libatcptap
LOCAL_SHARED_LIBRARIES:= liblog
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)

LOCAL_MODULE := echo_simple
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_SRC_FILES := echo_simple.c
LOCAL_STATIC_LIBRARIES := libatcptap
LOCAL_SHARED_LIBRARIES:= liblog
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)

LOCAL_MODULE := switchboard_simple
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_SRC_FILES := switchboard_simple.c
LOCAL_STATIC_LIBRARIES := libatcptap
LOCAL_SHARED_LIBRARIES:= liblog
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)

LOCAL_MODULE := tcptap-chat
LOCAL_MODULE_TAGS := debug eng
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_SRC_FILES := switchboard_threaded.c
LOCAL_STATIC_LIBRARIES := libatcptap
LOCAL_SHARED_LIBRARIES:= liblog
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)

LOCAL_MODULE := tcptap-rsh
LOCAL_MODULE_TAGS := debug eng
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_SRC_FILES := myrsh.c
LOCAL_STATIC_LIBRARIES := libatcptap
LOCAL_SHARED_LIBRARIES:= liblog
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)
