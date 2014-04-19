ROOT_DIR=../..
SUFFIX=power

include $(ROOT_DIR)/include/Makefile.config
include $(ROOT_DIR)/include/Makefile.common

all: victoria

victoria: victoria.cpp moncom.c
	$(CXX) $(CPPFLAGS)  victoria.cpp moncom.o  $(BUILD_DIR)/messages/libfirmament_messages.a   \
	$(BUILD_DIR)/base/libfirmament_base.a -o victoria $(LIBS)

moncom.o: moncom.c moncom.h
	$(CC) -c moncom.c -o moncom.o
