// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Initialization code for the executor library. This will be linked into the
// task binary, and constitutes the entry point for it. After creating an
// task library instance, setting up watcher threads etc., this will delegate to
// the task's Run() method.

#include "base/common.h"
#include "engine/task_lib.h"
//#include "platforms/common.h"

//#include "platforms/common.pb.h"

DECLARE_string(coordinator_uri);
DECLARE_string(resource_id);
DECLARE_string(cache_name);
extern char **environ;



using namespace firmament;  // NOLINT

void LaunchTasklib()
{
  printf("hello from free function\n");

  //sleep(6);

  
  string sargs = "";
  string progargs = "nginxy";


  //LOG(INFO) << "Executor's Run() method returned; terminating...";

  //BaseMessage bm;


  boost::thread::id main_thread_id = boost::this_thread::get_id();

  char *argv[2];
  argv[0] = const_cast<char*>(progargs.c_str());

  argv[1] = const_cast<char*>(sargs.c_str());

  firmament::common::InitFirmament(2, argv);


  char* uri = getenv("FLAGS_coordinator_uri");

  printf("ENV:%s\n", uri);

  printf("Coordinator URI:%s\n", FLAGS_coordinator_uri.c_str());
  sleep(5);
  TaskLib task_lib;

  task_lib.RunMonitor(main_thread_id);

  //   printf("Creating task_lib\n");


  //TaskLib task;
  while (true) {
    sleep(2);
    printf("Still alive!\n");   
  }

  //sleep(1000000);

  //LOG(INFO) << "Firmament task library starting for resource ";
            // << FLAGS_resource_id;

      //printf("%s", "creating tasklib\n");

}


// The main method: initializes, parses arguments and sets up a worker for
// the platform we're running on.
__attribute__ ((constructor)) static void task_lib_main() {
  // N.B.: We must always call InitFirmament from any main(), since it performs
  // setup work for command line flags, logging, etc.
  //sleep(2);

  sleep(3);


  // Unset LD_PRELOAD to avoid us from starting tasklib in childprocess
  setenv("LD_PRELOAD", "", 1);

  printf("Starting thread\n");
  boost::thread t1(&LaunchTasklib);

}
