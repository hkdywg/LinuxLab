SDK_PATH = /home/yinwg/ywg_workspace/PHUD/rk3576_linux6.1_release
SYSROOT += $(SDK_PATH)/buildroot/output/rockchip_rk3576/host/aarch64-buildroot-linux-gnu/sysroot
INCLUDES += -I./inc -I$(SYSROOT)/usr/include/opencv4 \
			-I$(SYSROOT)/usr/include/gstreamer-1.0 \
			-I$(SYSROOT)/usr/include/glib-2.0 \
			-I$(SYSROOT)/usr/lib/glib-2.0/include

TOOLCHAIN := $(SDK_PATH)/buildroot/output/rockchip_rk3576/host/bin/aarch64-linux-
CXX 	:= $(TOOLCHAIN)gcc

CFLAGS  += -g -Wall -O3 -Wl,--sysroot=$(SYSROOT) $(INCLUDES)
LIBS    += -lpthread  
LDFLAGS := `pkg-config gstreamer-1.0 --libs` -L$(SYSROOT)/usr/lib
CXXFLAGS:= $(CFLAGS)
SOURCE  := $(wildcard ./src/*.c)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
TARGET  := gst_dec_display
BUILD_PATH := ./build

.PHONY : clean distclean rebuild

all : $(TARGET)

$(BUILD_PATH):
	@mkdir -p $(BUILD_PATH)

$(TARGET) : $(SOURCE) $(BUILD_PATH)
	$(CXX) $(CXXFLAGS) -o ./build/$@ $(SOURCE) $(LDFLAGS) $(LIBS)

rebuild: distclean all

clean :
	rm -rf *~
	rm -rf *.o

distclean : clean
	rm -rf $(TARGET)



