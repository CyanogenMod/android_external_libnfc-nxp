LOCAL_PATH:= $(call my-dir)

#
# libnfc_ndef
#

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES += src/phFriNfc_NdefRecord.c

LOCAL_CFLAGS += -I$(LOCAL_PATH)/inc
LOCAL_CFLAGS += -I$(LOCAL_PATH)/src

LOCAL_MODULE:= libnfc_ndef
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_SHARED_LIBRARY)

