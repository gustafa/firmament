// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Initialization code for the executor library. This will be linked into the
// task binary, and constitutes the entry point for it. After creating an
// task library instance, setting up watcher threads etc., this will delegate to
// the task's Run() method.

#include "base/common.h"
#include "engine/task_lib.h"
#include <curl/curl.h>

DECLARE_string(coordinator_uri);
DECLARE_string(resource_id);
DECLARE_string(cache_name);
extern char **environ;

using namespace firmament;  // NOLINT

void LaunchTasklib() {
  /* Sets up and runs a TaskLib monitor in the current thread. */
  VLOG(3) << "Tasklib thread launched";
  firmament::common::InitFirmament(2, argv);


  string sargs = "";
  string progargs = "nginxy";
  boost::thread::id main_thread_id = boost::this_thread::get_id();

  char *argv[2];
  argv[0] = const_cast<char*>(progargs.c_str());

  argv[1] = const_cast<char*>(sargs.c_str());


  TaskLib task_lib;
  task_lib.RunMonitor(main_thread_id);
}


_attribute__ ((constructor)) static void task_lib_main() {
  /*
  Launched through LD_PRELOAD. Starts a new thread to run the TaskLib
  monitoring and lets the main program continue execution in the current thread.
  */

  // Unset LD_PRELOAD to avoid us from starting launching monitors in childprocesses.
  setenv("LD_PRELOAD", "", 1);

  VLOG(2) << "Starting tasklib monitor thread\n");
  boost::thread t1(&LaunchTasklib);
}
