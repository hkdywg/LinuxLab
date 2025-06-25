#
# Copyright(c) 2011-2014 Renesas Electronics Corporation
# RENESAS ELECTRONICS CONFIDENTIAL AND PROPRIETARY.
# This program must be used solely for the purpose for which
# it was furnished by Renesas Electronics Corporation. No part of this
# program may be reproduced or disclosed to others, in any
# form, without the prior written permission of Renesas Electronics
# Corporation.
#

PROG       = egl_gst_video_player

# CC         = $(CROSS_COMPILE)gcc
# CXX        = $(CROSS_COMPILE)g++

TARGET     = $(PROG)
OBJS       = $(PROG).o gst_discoverer.o

PKGROOT ?= 

LIBS       = -lEGL -lGLESv2 -lpthread

# for libdrm
DRMPKG	= libdrm libkms
CFLAGS	+= $(shell pkg-config --cflags-only-I $(DRMPKG))
LDFLAGS	+= $(shell pkg-config --libs-only-L $(DRMPKG))
LIBS	+= $(shell pkg-config --libs-only-l $(DRMPKG))

CFLAGS	+= $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gstreamer-pbutils-1.0)
LDFLAGS	+= $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gstreamer-pbutils-1.0)
LIBS	+= $(shell pkg-config --libs --cflags gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gstreamer-pbutils-1.0)

.c.o:
	$(CC) -c $(INCLUDE) $(CFLAGS) $*.c

.cpp.o:
	$(CXX) -c $(INCLUDE) $(CFLAGS) $*.cpp

all:    $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

$(OBJS) : $(HEADS)

clean :
	-$(RM) $(OBJS)
	-$(RM) $(TARGET)

