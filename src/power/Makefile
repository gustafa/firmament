ROOT_DIR=../..
SUFFIX=power

include $(ROOT_DIR)/include/Makefile.config
include $(ROOT_DIR)/include/Makefile.common

VIC_LIBS = $(LIBS) -lboost_system -lboost_thread -lboost_regex -lasan

all: .setup  $(BUILD_DIR)/power/victoria

$(BUILD_DIR)/power/victoria: victoria.cpp moncom.o
	$(CXX) $(CPPFLAGS) -D__FIRMAMENT__ -D__PLATFORM_HAS_BOOST__ -D_GNU_SOURCE -std=c++11 victoria.cpp moncom.o  $(BUILD_DIR)/messages/libfirmament_messages.a   \
	$(BUILD_DIR)/base/libfirmament_base.a  $(BUILD_DIR)/platforms/unix/libfirmament_unix.a -o $(BUILD_DIR)/power/victoria $(VIC_LIBS)


moncom.o: moncom.c moncom.h
	$(CC) -c moncom.c -o moncom.o
