ROOT_DIR=../..
SUFFIX=engine

include $(ROOT_DIR)/include/Makefile.config

TASK_LIB_LIBS := $(LIBS) -lboost_thread -lboost_system -lboost_regex -lboost_date_time -lhwloc -lssl -lcrypto
LIBS += -lboost_thread -lboost_system -lboost_regex -lboost_date_time -lhwloc -lssl -lcrypto
HTTP_UI_LIBS += $(PION_LIB) $(PB2JSON_LIB) -llog4cpp -ljansson -lctemplate

LIB = libfirmament_engine.a
TASK_LIB = libfirmament_task_lib.a
BINS = coordinator worker coordinator_sim
OBJS = coordinator_http_ui.o coordinator.o knowledge_base.o local_executor.o node.o remote_executor.o worker.o topology_manager.o haproxy_controller.o
PBS =
TESTS = coordinator_test local_executor_test simple_scheduler_test topology_manager_test worker_test
TESTS_DEPS = $(BUILD_DIR)/misc/libfirmament_misc.a \
	     $(BUILD_DIR)/base/libfirmament_base.a \
	     $(BUILD_DIR)/messages/libfirmament_messages.a \
	     $(BUILD_DIR)/platforms/unix/libfirmament_unix.a \
	     $(BUILD_DIR)/platforms/common.pb.o \
	     $(BUILD_DIR)/storage/libfirmament_storage.a
# Test-specific dependencies; we keep these separate from the others in order
# to avoid cyclic dependencies that will break test compilation.
# E.g. worker_test would depend on coordinator.o if it had coordinator_http_ui.o
# as a dependency.
TESTS_coordinator_DEPS = $(BUILD_DIR)/engine/coordinator_http_ui.o \
	                 $(BUILD_DIR)/engine/node.o \
	                 $(BUILD_DIR)/engine/knowledge_base.o \
	                 $(BUILD_DIR)/platforms/unix/libfirmament_unix.a \
	                 $(BUILD_DIR)/engine/haproxy_controller.o \
	                 $(BUILD_DIR)/engine/local_executor.o \
	                 $(BUILD_DIR)/engine/remote_executor.o \
	                 $(BUILD_DIR)/engine/topology_manager.o \
	                 $(BUILD_DIR)/scheduling/libfirmament_scheduling.a \
	                 $(BUILD_DIR)/misc/libfirmament_misc.a \
	                 $(BUILD_DIR)/base/libfirmament_base.a \
	                 $(BUILD_DIR)/messages/libfirmament_messages.a \
	                 $(BUILD_DIR)/storage/libfirmament_storage.a \
			 $(HTTP_UI_LIBS)
# N.B.: Note that simple_scheduler_test lives in the engine makefile as it
# depends on engine components that are not yet built when the schedulers (which
# the coordinator depends on) are built.
TESTS_simple_scheduler_DEPS = $(BUILD_DIR)/engine/node.o \
			      $(BUILD_DIR)/engine/local_executor.o \
			      $(BUILD_DIR)/engine/remote_executor.o \
			      $(BUILD_DIR)/engine/topology_manager.o \
			      $(BUILD_DIR)/messages/libfirmament_messages.a \
			      $(BUILD_DIR)/scheduling/libfirmament_scheduling.a \
			      $(BUILD_DIR)/misc/libfirmament_misc.a \
			      $(BUILD_DIR)/base/libfirmament_base.a \
			      $(BUILD_DIR)/platforms/unix/libfirmament_unix.a \
			      $(BUILD_DIR)/storage/libfirmament_storage.a
TESTS_local_executor_DEPS = $(BUILD_DIR)/engine/topology_manager.o \
			    $(BUILD_DIR)/misc/libfirmament_misc.a \
			    $(BUILD_DIR)/base/libfirmament_base.a
TESTS_worker_DEPS = $(BUILD_DIR)/engine/node.o \
		    $(BUILD_DIR)/engine/topology_manager.o \
		    $(BUILD_DIR)/platforms/unix/libfirmament_unix.a \
		    $(BUILD_DIR)/messages/libfirmament_messages.a \
		    $(BUILD_DIR)/misc/libfirmament_misc.a \
		    $(BUILD_DIR)/storage/libfirmament_storage.a \
		    $(BUILD_DIR)/base/libfirmament_base.a

OBJ_BIN = $(addprefix $(OBJ_DIR)/, $(BINS))
OBJ_LIB = $(addprefix $(OBJ_DIR)/, $(LIB))
TASK_OBJ_LIB = $(addprefix $(OBJ_DIR)/, $(TASK_LIB))
#TESTS_OBJS = $(addprefix $(TEST_OBJ_DIR)/, $(TESTS))
TESTS_BIN = $(addprefix $(TEST_OBJ_DIR)/, $(TESTS))

all: protobufs $(OBJ_BIN) $(TESTS_BIN) $(OBJ_LIB)
	#$(TASK_OBJ_LIB) $(BUILD_DIR)/engine/task_lib_main.so .setup
	#

include $(ROOT_DIR)/include/Makefile.common

protobufs: $(addprefix $(OBJ_DIR)/, $(PBS))

# TODO: check if any of the below targets can make use of a generic
# build rule from Makefile.common, or can be converted to do so.

$(OBJ_DIR)/coordinator: $(addprefix $(OBJ_DIR)/, $(OBJS)) \
	$(BUILD_DIR)/base/libfirmament_base.a \
	$(BUILD_DIR)/misc/libfirmament_misc.a \
	$(BUILD_DIR)/messages/libfirmament_messages.a \
	$(BUILD_DIR)/scheduling/libfirmament_scheduling.a \
	$(BUILD_DIR)/storage/libfirmament_storage.a \
	$(BUILD_DIR)/platforms/unix/libfirmament_unix.a
	$(call quiet-command, \
		$(CXX) $(CPPFLAGS) \
		$(SRC_ROOT_DIR)/engine/coordinator_main.cc \
		$(OPTFLAGS) $(DEFINES) \
		$(BUILD_DIR)/engine/coordinator.o \
		$(BUILD_DIR)/engine/haproxy_controller.o \
		$(BUILD_DIR)/engine/knowledge_base.o \
		$(BUILD_DIR)/engine/node.o \
		$(BUILD_DIR)/engine/coordinator_http_ui.o \
		$(BUILD_DIR)/engine/local_executor.o \
		$(BUILD_DIR)/engine/remote_executor.o \
		$(BUILD_DIR)/engine/topology_manager.o \
		$(BUILD_DIR)/platforms/common.pb.o \
		$(BUILD_DIR)/messages/libfirmament_messages.a \
		$(BUILD_DIR)/scheduling/libfirmament_scheduling.a \
		$(BUILD_DIR)/base/libfirmament_base.a \
		$(BUILD_DIR)/misc/libfirmament_misc.a \
		$(BUILD_DIR)/platforms/unix/libfirmament_unix.a \
		$(BUILD_DIR)/storage/libfirmament_storage.a \
		$(LIBS) $(HTTP_UI_LIBS) \
		-o $(BUILD_DIR)/engine/coordinator, \
		"  DYNLNK  $(BUILD_DIR)/engine/coordinator")

# XXX(malte): temporary hack
$(OBJ_DIR)/coordinator_sim: $(addprefix $(OBJ_DIR)/, $(OBJS)) \
	$(BUILD_DIR)/sim/libfirmament_sim.a \
	$(BUILD_DIR)/base/libfirmament_base.a \
	$(BUILD_DIR)/misc/libfirmament_misc.a \
	$(BUILD_DIR)/messages/libfirmament_messages.a \
	$(BUILD_DIR)/scheduling/libfirmament_scheduling.a \
	$(BUILD_DIR)/storage/libfirmament_storage.a \
	$(BUILD_DIR)/engine/haproxy_controller.o \
	$(BUILD_DIR)/platforms/unix/libfirmament_unix.a
	$(call quiet-command, \
		$(CXX) $(CPPFLAGS) \
		$(SRC_ROOT_DIR)/engine/coordinator_main.cc \
		$(OPTFLAGS) $(DEFINES) \
		$(SRC_ROOT_DIR)/engine/coordinator.cc \
		$(BUILD_DIR)/engine/haproxy_controller.o \
		$(BUILD_DIR)/engine/node.o \
		$(BUILD_DIR)/engine/knowledge_base.o \
		$(BUILD_DIR)/engine/coordinator_http_ui.o \
		$(BUILD_DIR)/engine/local_executor.o \
		$(BUILD_DIR)/engine/remote_executor.o \
		$(BUILD_DIR)/engine/topology_manager.o \
		$(BUILD_DIR)/platforms/common.pb.o \
		$(BUILD_DIR)/messages/libfirmament_messages.a \
		$(BUILD_DIR)/scheduling/libfirmament_scheduling.a \
		$(BUILD_DIR)/base/libfirmament_base.a \
		$(BUILD_DIR)/misc/libfirmament_misc.a \
		$(BUILD_DIR)/platforms/unix/libfirmament_unix.a \
		$(BUILD_DIR)/storage/libfirmament_storage.a \
		$(BUILD_DIR)/sim/libfirmament_sim.a \
		$(LIBS) $(HTTP_UI_LIBS) \
		-o $(BUILD_DIR)/engine/coordinator_sim \
		-D__SIMULATE_SYNTHETIC_DTG__, \
		"  DYNLNK  $(BUILD_DIR)/engine/coordinator_sim")

$(OBJ_DIR)/worker: $(addprefix $(OBJ_DIR)/, $(OBJS)) \
	$(BUILD_DIR)/platforms/unix/libfirmament_unix.a \
	$(BUILD_DIR)/messages/libfirmament_messages.a \
	$(BUILD_DIR)/storage/libfirmament_storage.a \
	$(BUILD_DIR)/base/libfirmament_base.a
	$(call quiet-command, \
		$(CXX) $(CPPFLAGS) \
		$(SRC_ROOT_DIR)/engine/worker_main.cc \
		$(OPTFLAGS) $(DEFINES) \
		$(BUILD_DIR)/engine/node.o \
		$(BUILD_DIR)/engine/worker.o \
		$(BUILD_DIR)/messages/libfirmament_messages.a \
		$(BUILD_DIR)/base/libfirmament_base.a \
		$(BUILD_DIR)/platforms/common.pb.o \
		$(BUILD_DIR)/platforms/unix/libfirmament_unix.a \
		$(BUILD_DIR)/storage/libfirmament_storage.a \
		$(LIBS) -o $(BUILD_DIR)/engine/worker, \
		"  DYNLNK  $(BUILD_DIR)/engine/worker")
#		$(BUILD_DIR)/engine/local_executor.o \


#echo $(INCLUDES)
# Task interface library
# ---------------------------
# This needs to be linked into task binaries.
$(OBJ_LIB): $(addprefix $(OBJ_DIR)/, $(OBJS))

#TODO VERIFY IF THIS CAN BE REMOVED$(OBJ_LIB): $(addprefix $(OBJ_DIR)/, task_lib.o)
NGINX_INCLUDES := -I $(SRC_ROOT_DIR)/examples/nginx/src/core  -I $(SRC_ROOT_DIR)/examples/nginx/objs -I $(SRC_ROOT_DIR)/examples/nginx/src/os/unix -I $(SRC_ROOT_DIR)/examples/nginx/src/event #$(addprefix("-I ", core))
NGINX_DEF := -D__FIRMAMENT__-DNGX_STAT_STUB=1

ALL_O=$(addprefix $(BUILD_DIR)/, scheduling/trivial_cost_model.o scheduling/scheduling_delta.pb.o scheduling/flow_node_type.pb.o scheduling/quincy_scheduler.o scheduling/flow_graph.o scheduling/quincy_cost_model.o scheduling/event_driven_scheduler.o scheduling/scheduling_parameters.pb.o scheduling/dimacs_exporter.o scheduling/simple_scheduler.o sim/simulation_event.pb.o    storage/simple_object_store.o storage/Cache.o storage/StorageInfo.o misc/string_utils.o misc/pb_utils.o misc/event_logging.o misc/equivclasses.o misc/utils.o platforms/common.pb.o platforms/unix/stream_sockets_adapter.o platforms/unix/common.o platforms/unix/async_tcp_server.o platforms/unix/procfs_monitor.o platforms/unix/tcp_connection.o platforms/unix/signal_handler.o platforms/unix/procfs_machine.o engine/local_executor.o engine/knowledge_base.o engine/node.o engine/remote_executor.o engine/topology_manager.o messages/task_info_message.pb.o messages/registration_message.pb.o messages/storage_registration_message.pb.o messages/copy_message.pb.o messages/task_state_message.pb.o messages/storage_message.pb.o messages/task_delegation_message.pb.o messages/heartbeat_message.pb.o messages/base_message.pb.o messages/create_message.pb.o messages/lookup_message.pb.o messages/test_message.pb.o messages/task_heartbeat_message.pb.o messages/task_kill_message.pb.o messages/task_spawn_message.pb.o messages/io_notification_message.pb.o messages/delete_message.pb.o messages/energy_stats_message.pb.o base/ensemble_desc.pb.o base/task_desc.pb.o base/resource_topology_node_desc.pb.o base/reference_desc.pb.o base/resource_desc.pb.o base/data_object.o base/job_desc.pb.o base/task_final_report.pb.o base/task_perf_statistics_sample.pb.o base/task_graph.o base/resource_status.o base/data_object_name.pb.o base/job.o base/machine_perf_statistics_sample.pb.o base/task.o)

TASK_LIB_LIBS := $(LIBS) -lcurl -ljansson

$(BUILD_DIR)/engine/task_lib_main.so:	task_lib_main.cc task_lib.cc
	# echo $(NGINX_INCLUDES) #echo $(addprefix(-I $(SRC_ROOT_DIR)/examples/nginx/src, core))
	$(CC) -fPIC  -shared -std=c++11 -fvisibility=default $(OPTFLAGS) $(TASK_LIB_LIBS) $(INCLUDES)  $(NGINX_INCLUDES) $(DEFINES)  -o $(BUILD_DIR)/engine/task_lib_main.so    task_lib_main.cc task_lib.cc $(ALL_O) -ldl
