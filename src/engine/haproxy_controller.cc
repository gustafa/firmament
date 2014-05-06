
#include "haproxy_controller.h"

#include "base/common.h"
#include "base/task_desc.pb.h"
#include "misc/map-util.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>



DEFINE_string(haproxy_conf_file, "/home/gjrh2/haproxy.conf",
              "The path of the nginx configuration file");

DEFINE_string(haproxy_socket_file, "/home/gjrh2/haproxy.socket",
              "The path of the HA proxy socket file used for "
              "stats extraction.");

namespace firmament {


HAProxyController::HAProxyController() {
  stats_headers.reset(new vector<string>());
  //stats_map.reset(new unordered_map<string, unordered_map<string, string>>);
}


bool HAProxyController::DisableServer(string hostname) {
  string command = "disable server my_servers/" + hostname;
  HAProxyCommand(command);
  return true;
}

bool HAProxyController::EnableServer(string hostname) {

  string command = "enable server my_servers/" + hostname;
  HAProxyCommand(command);
  return true;
}


void HAProxyController::GetStatistics() {
  // TODO(gustafa):
  string stats_string = HAProxyCommand("show stat");
  const string outer_delimiter = "\n";
  const string inner_delimiter = ",";

  size_t pos = stats_string.find(outer_delimiter);

  string headers = stats_string.substr(0, pos);

  stats_string.erase(0, pos + 1);


  if (!(stats_headers->size())) {
    while ((pos = headers.find(inner_delimiter)) != string::npos) {
      stats_headers->push_back(headers.substr(0, pos));
      //cout << "HEAD: "  << headers.substr(0, pos) << "\n";
      headers.erase(0, pos + 1);
    }
  }

  int64_t i;
  size_t inner_pos;

  while ((pos = stats_string.find(outer_delimiter)) != string::npos) {
    string stat_row = stats_string.substr(0, pos);
    stats_string.erase(0, pos + 1);



    // The two first columns are the inner key
    string inner_key;

    for (i = 0; i < 2; ++i) {
      inner_pos = stat_row.find(inner_delimiter);
      inner_key += stat_row.substr(0, inner_pos);
      stat_row.erase(0, inner_pos + 1);
    }

    if (inner_key.empty()) {
      continue;
    }

    // cout << "INNER_KEY" << inner_key << "\n";

    while ((inner_pos = stat_row.find(inner_delimiter)) != string::npos) {

      // Iterate over each stat, look up its header and store it in the map.
      string temp =  stat_row.substr(0, inner_pos);

      string key = (*stats_headers)[i] + "_" + inner_key;

      stats_map[key] = temp;
      stat_row.erase(0, inner_pos + 1);
      ++i;
    }

    // for (auto keyval : stats_map) {
    //   cout << "STATS " << keyval.first << ": " << keyval.second << "\n";
    // }
  }

}

string HAProxyController::HAProxyCommand(string args) {
  std::stringstream ss;

  string command = string("haproxyctl ") + args;

  FILE *in;
  char buff[512];

  if(!(in = popen(command.c_str(), "r"))){
    // ERROR
  }

  while (fgets(buff, sizeof(buff), in)!=NULL) {
    ss << buff;
  }

  pclose(in);
  return ss.str();
}



void HAProxyController::GetJobs(vector<JobDescriptor*> &jobs, uint64_t next_seconds) {
  // Get statistics and approximate how many webserver jobs we need to create.
  GetStatistics();

  // TODO(gustafa): Compute how many we should actually have!
  int64_t num_of_tasks = 3;
  const int64_t name_size = 32;

  char buffer[name_size];
  char input_buffer[name_size];

  int fd = open("/dev/urandom", O_RDONLY);

  read(fd, buffer, name_size);


  for (int64_t i = 0; i < num_of_tasks; ++i) {
    JobDescriptor *job_desc = new JobDescriptor();
    TaskDescriptor *root_task = job_desc->mutable_root_task();
    job_desc->set_uuid("");
    job_desc->set_name("webserver_job");
    root_task->set_name("webserver_task");
    root_task->set_state(TaskDescriptor_TaskState_CREATED);
    root_task->set_binary("nginx_firmament");

    ReferenceDescriptor *input_desc = root_task->add_dependencies();
    read(fd, input_buffer, name_size);
    input_desc->set_id(input_buffer, name_size);
    input_desc->set_scope(ReferenceDescriptor_ReferenceScope_PUBLIC);
    input_desc->set_type(ReferenceDescriptor_ReferenceType_CONCRETE);
    input_desc->set_location("blob:/tmp/fib_in");
    input_desc->set_non_deterministic(false);


    for (int64_t j = 0; j < 2; ++j) {
      read(fd, buffer, name_size);
      ReferenceDescriptor *output_desc = root_task->add_outputs();
      output_desc->set_id(buffer, name_size);
      output_desc->set_scope(ReferenceDescriptor_ReferenceScope_PUBLIC);
      output_desc->set_type(ReferenceDescriptor_ReferenceType_FUTURE);
      output_desc->set_location("blob:/tmp/out" + std::to_string(j));
      output_desc->set_non_deterministic(false);
    }

    jobs.push_back(job_desc);


//    read(fd, input_buffer, 256);

  }
}


} // namespace firmament