LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := tcp_tap
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS += -fPIC
LOCAL_CFLAGS += -DNDEBUG
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_SRC_FILES := main.c  server.c  sig_mngr.c  switchboard.c

LOCAL_SHARED_LIBRARIES := 


include $(LOCAL_PATH)/common.mk
include $(BUILD_EXECUTABLE)
#$(call import-module,lib_something)
