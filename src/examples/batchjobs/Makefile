ROOT_DIR=../../..
SUFFIX=examples/batchjobs

include $(ROOT_DIR)/include/Makefile.config
include $(ROOT_DIR)/include/Makefile.common

FT_NUM_INPUT_FILES=2
FT_SIZE_IN2=8
FT_OUTPUT_DIR=$(BUILD_DIR)/$(SUFFIX)

LIBS+=  -lm -ljansson -lcurl -lboost_thread  -lboost_filesystem -lboost_system -lboost_regex -lboost_date_time -lssl -lcrypto -lstdc++

FIRMAMENT_LIBS =	$(BUILD_DIR)/engine/libfirmament_task_lib.a \
		$(BUILD_DIR)/messages/libfirmament_messages.a \
		$(BUILD_DIR)/misc/libfirmament_misc.a \
		$(BUILD_DIR)/base/libfirmament_base.a \
		$(BUILD_DIR)/platforms/unix/libfirmament_unix.a

all: .setup $(FT_OUTPUT_DIR)/filetransfer $(FIRMAMENT_LIBS)

$(FT_OUTPUT_DIR)/filetransfer:	filetransfer.cc filegenerator
	$(CC) $(OPTFLAGS) $(CPPFLAGS) $(DEFINES) $(INCLUDES) filetransfer.cc  $(FIRMAMENT_LIBS) -o $(FT_OUTPUT_DIR)/filetransfer  $(LIBS)

filegenerator: $(FT_OUTPUT_DIR)/input0

$(FT_OUTPUT_DIR)/input0:
	@echo "Generating input files for the filetransfer batch job"
	@./filegenerator 5 5 $(FT_OUTPUT_DIR)
