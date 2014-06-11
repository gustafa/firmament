#include "base/common.h"
#include "engine/task_lib.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <stdlib.h>
#include <string>


using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::shared_ptr;


shared_ptr<firmament::TaskLib> task_lib;

bool fileExists(const std::string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}

void LaunchTasklib() {
  boost::thread::id task_thread_id = boost::this_thread::get_id();
  // TODO(gustafa): Send the main thread id and join neatly in the
  // tasklib monitor.
  task_lib->RunMonitor(task_thread_id);
}

int main(int argc, char **argv) {
  string input_dir;
  string output_dir;
  if (argc < 4) {
    std::cerr << "Usage: filetransfer <input_dir> <output_dir> <num_files>" << std::endl;
    exit(1);
  }
  input_dir = argv[1];
  output_dir = argv[2];
  int num_files = atoi(argv[3]);
  std::cout << input_dir;
  bool inside_firmament = getenv("FLAGS_coordinator_uri") != NULL;


  // Setup firmament. This check is to allow for benchmarking without a running instance of firmament.
  // (inside_firmament) {
    string sargs = "--tryfromenv=coordinator_uri,resource_id,task_id,heartbeat_interval,tasklib_application";
    string progargs = "nginxy";
    boost::thread::id task_thread_id = boost::this_thread::get_id();

    // char *argv2[1];
    // argv2[0] = argv[0];
    // //argv2[1] = const_cast<char*>(sargs.c_str());
    // firmament::common::InitFirmament(1, argv2);

    //firmament::common::InitFirmament(argc, argv);
    task_lib.reset(new firmament::TaskLib());
    task_lib->SetCompleted(0);
    boost::thread t1(&LaunchTasklib);
  //}



  int i = 0;
  int max_files = 20;
  for (;i < num_files; ++i) {
    string current_file = input_dir + "/input" + std::to_string(i % max_files);
    ifstream source(current_file, ios::binary);
    ofstream dest(output_dir + "/output" + std::to_string(i % max_files), ios::binary);
    dest << source.rdbuf();
    source.close();
    dest.close();
    if (inside_firmament) {
      task_lib->SetCompleted((i+1) / double(num_files));
    }
  }

  if (i == 0) {
    std::cerr << "NO FILES COPIED, could not find any in: " << input_dir << std::endl;
    exit(2);
  } else {
    exit(0);
  }
}


