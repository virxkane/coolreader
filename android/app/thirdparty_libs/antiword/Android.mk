
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := local_antiword

ANTIWORD_SRC_DIR := ../../../../thirdparty_unman/antiword
ANTIWORD_SRC_DIR_P := $(LOCAL_PATH)/../../../../thirdparty_unman/antiword

LOCAL_C_INCLUDES := $(ANTIWORD_SRC_DIR_P)
LOCAL_CFLAGS += -funwind-tables -Wl,--no-merge-exidx-entries
LOCAL_CFLAGS += -DCR3_ANTIWORD_PATCH=1

LOCAL_SRC_FILES := \
    $(ANTIWORD_SRC_DIR)/asc85enc.c \
    $(ANTIWORD_SRC_DIR)/blocklist.c \
    $(ANTIWORD_SRC_DIR)/chartrans.c \
    $(ANTIWORD_SRC_DIR)/datalist.c \
    $(ANTIWORD_SRC_DIR)/depot.c \
    $(ANTIWORD_SRC_DIR)/doclist.c \
    $(ANTIWORD_SRC_DIR)/fail.c \
    $(ANTIWORD_SRC_DIR)/finddata.c \
    $(ANTIWORD_SRC_DIR)/findtext.c \
    $(ANTIWORD_SRC_DIR)/fontlist.c \
    $(ANTIWORD_SRC_DIR)/fonts.c \
    $(ANTIWORD_SRC_DIR)/fonts_u.c \
    $(ANTIWORD_SRC_DIR)/hdrftrlist.c \
    $(ANTIWORD_SRC_DIR)/imgexam.c \
    $(ANTIWORD_SRC_DIR)/listlist.c \
    $(ANTIWORD_SRC_DIR)/misc.c \
    $(ANTIWORD_SRC_DIR)/notes.c \
    $(ANTIWORD_SRC_DIR)/options.c \
    $(ANTIWORD_SRC_DIR)/out2window.c \
    $(ANTIWORD_SRC_DIR)/pdf.c \
    $(ANTIWORD_SRC_DIR)/pictlist.c \
    $(ANTIWORD_SRC_DIR)/prop0.c \
    $(ANTIWORD_SRC_DIR)/prop2.c \
    $(ANTIWORD_SRC_DIR)/prop6.c \
    $(ANTIWORD_SRC_DIR)/prop8.c \
    $(ANTIWORD_SRC_DIR)/properties.c \
    $(ANTIWORD_SRC_DIR)/propmod.c \
    $(ANTIWORD_SRC_DIR)/rowlist.c \
    $(ANTIWORD_SRC_DIR)/sectlist.c \
    $(ANTIWORD_SRC_DIR)/stylelist.c \
    $(ANTIWORD_SRC_DIR)/stylesheet.c \
    $(ANTIWORD_SRC_DIR)/summary.c \
    $(ANTIWORD_SRC_DIR)/tabstop.c \
    $(ANTIWORD_SRC_DIR)/unix.c \
    $(ANTIWORD_SRC_DIR)/utf8.c \
    $(ANTIWORD_SRC_DIR)/word2text.c \
    $(ANTIWORD_SRC_DIR)/worddos.c \
    $(ANTIWORD_SRC_DIR)/wordlib.c \
    $(ANTIWORD_SRC_DIR)/wordmac.c \
    $(ANTIWORD_SRC_DIR)/wordole.c \
    $(ANTIWORD_SRC_DIR)/wordwin.c \
    $(ANTIWORD_SRC_DIR)/xmalloc.c

include $(BUILD_STATIC_LIBRARY)
