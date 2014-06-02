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
  string dir;
  if (argc < 3) {
    std::cerr << "Usage: filetransfer <dir> <num_files>" << std::endl;
    exit(1);
  }
  dir = argv[1];
  int num_files = atoi(argv[2]);
  std::cout << dir;

  firmament::common::InitFirmament(argc, argv);
  task_lib.reset(new firmament::TaskLib());
  task_lib->SetCompleted(0);

  boost::thread t1(&LaunchTasklib);

  int i = 0;
  for (;i < num_files; ++i) {
    string current_file = dir + "/input" + std::to_string(i);
    ifstream source(current_file, ios::binary);
    ofstream dest(dir + "/output" + std::to_string(i), ios::binary);
    dest << source.rdbuf();
    source.close();
    dest.close();
    task_lib->SetCompleted((i+1) / double(num_files));
  }

  if (i == 0) {
    std::cerr << "NO FILES COPIED, could not find any in: " << dir << std::endl;
    exit(2);
  } else {
    exit(0);
  }
}


