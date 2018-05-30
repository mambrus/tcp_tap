LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := debug eng
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_SRC_FILES :=  clientserver.c switchboard.c
LOCAL_MODULE := libatcptap
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := debug eng
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_SRC_FILES :=  clientserver.c switchboard.c
LOCAL_MODULE := libsotcptap
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := tcp-tap
LOCAL_MODULE_TAGS := debug eng
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_SRC_FILES := main.c sig_mngr.c
#LOCAL_SHARED_LIBRARIES :=
LOCAL_STATIC_LIBRARIES := libatcptap
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

#include $(call all-subdir-makefiles)
#$(call import-module,lib_something)

TCPTAP_PATH := $(LOCAL_PATH)
include $(TCPTAP_PATH)/examples/Android.mk
