LOCAL_PATH := $(call my-dir)

#
# libiep.so
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	iep_api.cpp

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-Wall \
	-Wextra \
	-DLOG_TAG=\"libiep\"

LOCAL_C_INCLUDES += .
LOCAL_LDFLAGS := \
	-Wl,-z,defs

LOCAL_SHARED_LIBRARIES := \
	libcutils

LOCAL_MODULE := libiep
LOCAL_MODULE_TAGS    := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

#
# iep_async_test
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	iep_async_test.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libiep \
	libvpu

LOCAL_LDLIBS += -lpthread

LOCAL_MODULE := iep_async_test
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)

#
# iep_sync_test
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	iep_sync_test.cpp

#LOCAL_C_INCLUDES += kernel/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libiep \
	libvpu

LOCAL_LDLIBS += -lpthread

LOCAL_MODULE := iep_sync_test
LOCAL_MODULE_TAGS := optional tests
LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)

#
# iep_func_test
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	iep_func_test.cpp

#LOCAL_C_INCLUDES += kernel/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libiep \
	libvpu

LOCAL_LDLIBS += -lpthread

LOCAL_MODULE := iep_func_test
LOCAL_MODULE_TAGS := optional tests
LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)

#
# iep_sync
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	iep_sync.cpp

#LOCAL_C_INCLUDES += kernel/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libvpu

LOCAL_MODULE := iep_sync
LOCAL_MODULE_TAGS := optional tests
LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)

#
# iep_dil
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	iep_dil.cpp

#LOCAL_C_INCLUDES += kernel/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libiep \
	libvpu

LOCAL_MODULE := iep_dil
LOCAL_MODULE_TAGS := optional tests
LOCAL_PRELINK_MODULE := false

include $(BUILD_EXECUTABLE)

