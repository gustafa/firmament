
#include "haproxy_controller.h"

#include "base/common.h"
#include "base/task_desc.pb.h"
#include "misc/map-util.h"

#include <boost/lexical_cast.hpp>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>



DEFINE_string(haproxy_conf_file, "/home/gjrh2/haproxy.conf",
              "The path of the nginx configuration file");

DEFINE_string(haproxy_socket_file, "/home/gjrh2/haproxy.socket",
              "The path of the HA proxy socket file used for "
              "stats extraction.");

namespace firmament {


HAProxyController::HAProxyController(string server_backend) :
  server_backend_(server_backend),
  stats_headers(new vector<string>()),
  current_web_job_(0),
  num_active_jobs_(0),
  start_port_(16000)
 {
  //stats_headers.reset(new vector<string>());
  //stats_map.reset(new unordered_map<string, unordered_map<string, string>>);
}


bool HAProxyController::DisableServer(string hostname, uint64_t port) {
  string webserver_name = hostname + boost::lexical_cast<std::string>(port);
  VLOG(2) << "HAPROXY Disabling server: " << webserver_name;
  string command = "disable server my_servers/" + webserver_name;
  running_servers_.erase(webserver_name);
  HAProxyCommand(command);
  return true;
}

bool HAProxyController::EnableServer(string hostname, uint64_t port) {
  string webserver_name = hostname + boost::lexical_cast<std::string>(port);
  VLOG(2) << "HAPROXY Enabling server: " << webserver_name;
  string command = "enable server my_servers/" + webserver_name;
  HAProxyCommand(command);
  running_servers_.insert(webserver_name);
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

  int64_t i = 0;
  size_t inner_pos;

  while ((pos = stats_string.find(outer_delimiter)) != string::npos) {
    string stat_row = stats_string.substr(0, pos);
    stats_string.erase(0, pos + 1);

    // my_servers,hostname
    // Skip the first bit
    // The two first columns are the inner key
    string inner_key = "";

    // Deleting the first bit, how the rest should be the server
    inner_pos = stat_row.find(inner_delimiter);
    stat_row.erase(0, inner_pos + 1);

    inner_pos = stat_row.find(inner_delimiter);
    string server = stat_row.substr(0, inner_pos);
    stat_row.erase(0, inner_pos + 1);


    // Todo verify this!
    if (server.empty() || running_servers_.find(server) != running_servers_.end()) {
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
  VLOG(3) << "Executing HAProxy command: " << command;


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



void HAProxyController::GenerateJobs(vector<JobDescriptor*> &jobs, uint64_t number_of_jobs) {
  const int64_t name_size = 32;
  char buffer[name_size];
  char input_buffer[name_size];
  int fd = open("/dev/urandom", O_RDONLY);
  read(fd, buffer, name_size);
  uint64_t current_start_port = start_port_ + num_active_jobs_;
  for (uint64_t i = 0; i < number_of_jobs; ++i) {
    JobDescriptor *job_desc = new JobDescriptor();
    TaskDescriptor *root_task = job_desc->mutable_root_task();
    job_desc->set_uuid("");
    job_desc->set_name("webserver_job" + boost::lexical_cast<std::string>(current_web_job_));
    root_task->set_name("nginx" + boost::lexical_cast<std::string>(current_web_job_));
    root_task->set_task_type(TaskDescriptor::NGINX);
    root_task->set_state(TaskDescriptor_TaskState_CREATED);
    root_task->set_binary("nginx_firmament");
    uint64_t port = current_start_port + i;
    root_task->set_port(port);
    string config_file = "/home/gjrh2/firmament/configs/nginx/nginx" +
        boost::lexical_cast<std::string>(port) + ".conf";
    root_task->add_args(boost::lexical_cast<std::string>(port));
    root_task->add_args("-c " + config_file);


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
    current_web_job_++;
    jobs.push_back(job_desc);
  }
}


} // namespace firmament