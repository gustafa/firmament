// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Initialization code for the executor library. This will be linked into the
// task binary, and constitutes the entry point for it. After creating an
// task library instance, setting up watcher threads etc., this will delegate to
// the task's Run() method.

#include "base/common.h"
#include "engine/task_lib.h"
//#include <cstdlib>
#include <curl/curl.h>

DECLARE_string(coordinator_uri);
DECLARE_string(resource_id);
DECLARE_string(cache_name);
extern char **environ;

using namespace firmament;  // NOLINT

TaskLib *task_lib;

void TerminationCleanup() {
  if (task_lib) {
    task_lib->Stop();
  }
}

void LaunchTasklib() {
  /* Sets up and runs a TaskLib monitor in the current thread. */
  // Read these important variables from the environment.
  sleep(1);
  string sargs = "--tryfromenv=coordinator_uri,resource_id,task_id,heartbeat_interval,tasklib_application,completion_filename";
  string progargs = "nginxy";
  boost::thread::id task_thread_id = boost::this_thread::get_id();

  char *argv[2];
  argv[0] = const_cast<char*>(progargs.c_str());

  argv[1] = const_cast<char*>(sargs.c_str());
    firmament::common::InitFirmament(2, argv);

  task_lib = new TaskLib();
  task_lib->RunMonitor(task_thread_id);

}

__attribute__((constructor)) static void task_lib_main() {
  /*
  Launched through the LD_PRELOAD environment variable.
  Starts a new thread to run the TaskLib monitoring and lets
  the main program continue execution in the current thread.
  */

  // Unset LD_PRELOAD to avoid us from starting launching monitors in
  // childprocesses.
  setenv("LD_PRELOAD", "", 1);

  // Cleanup task lib before terminating the process.
  atexit(TerminationCleanup);

  VLOG(2) << "Starting tasklib monitor thread\n";
  boost::thread t1(&LaunchTasklib);
}
