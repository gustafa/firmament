// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Miscellaneous utility functions. Descriptions with their declarations.

#include <boost/functional/hash.hpp>

// N.B.: C header for gettimeofday()
extern "C" {
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <openssl/sha.h>

}
#include <set>
#include <string>
#include <unordered_map>
#include <vector>


#include "misc/utils.h"

namespace firmament {

boost::mt19937 resource_id_rg_;
boost::mt19937 job_id_rg_;

unordered_map<string, ResourceID_t> hostname_to_uuid_;


bool resource_id_rg_init_ = false;
bool job_id_rg_init_ = false;

uint64_t GetCurrentTimestamp() {
  struct timeval ts;
  gettimeofday(&ts, NULL);
  return ts.tv_sec * 1000000 + ts.tv_usec;
}

/*uint64_t MakeJobUID(Job *job) {
  CHECK_NOTNULL(job);
  boost::hash<string> hasher;
  return hasher(job->name());
}

uint64_t MakeEnsembleUID(Ensemble *ens) {
  CHECK_NOTNULL(ens);
  VLOG(1) << ens->name();
  boost::hash<string> hasher;
  return hasher(ens->name());
}*/

// DEPRECATED wrapper for backwards compatibility
ResourceID_t GenerateUUID() {
  return GenerateResourceID();
}

ResourceID_t GenerateResourceID() {
  if (!resource_id_rg_init_) {
    // TODO(malte): This crude method captures the first 100 chars of the
    // hostname (not the FQDN). It remains to be seen if it is sufficient.
    SetupResourceID(&resource_id_rg_, NULL);
    resource_id_rg_init_ = true;
  }
  boost::uuids::basic_random_generator<boost::mt19937> gen(&resource_id_rg_);
  return gen();
}

void SetupResourceID(boost::mt19937 *resource_id, const char *hostname) {
  size_t hash = 0;
  char hn[100];
  bzero(&hn, 100);
  if (hostname == NULL) {
    gethostname(hn, 100);
  } else {
    strcpy(hn, hostname);
  }
  // Hash the hostname (truncated to 100 characters)
  boost::hash_combine(hash, hn);
  VLOG(2) << "Seeing resource ID RNG with " << hash << " from hostname "
          << hn;
  resource_id->seed(hash);
}



// TODO the following two functions should be renamed, they are only used to get some reliable names
// Find a resource UUID f
ResourceID_t FindResourceID(string hostname) {
  unordered_map<string, ResourceID_t>::const_iterator got =
      hostname_to_uuid_.find(hostname);

  // If the UUID is already cached return it.
  if (got != hostname_to_uuid_.end()) {
    return got->second;
  } else {
  // Otherwise compute it and insert it into the cache.
    boost::mt19937 resource_id_rg;
    SetupResourceID(&resource_id_rg, hostname.c_str());
    boost::uuids::basic_random_generator<boost::mt19937> gen(&resource_id_rg);
    ResourceID_t uuid = gen();
    hostname_to_uuid_[hostname] = uuid;
    return uuid;
  }
}

void InsertResourceID(string hostname, string uuid_string) {
  hostname_to_uuid_[hostname] = ResourceIDFromString(uuid_string);
}



JobID_t GenerateJobID() {
  if (!job_id_rg_init_) {
    job_id_rg_.seed(time(NULL));
    job_id_rg_init_ = true;
  }
  boost::uuids::basic_random_generator<boost::mt19937> gen(&job_id_rg_);
  return gen();
}

TaskID_t GenerateRootTaskID(const JobDescriptor& job_desc) {
  size_t hash = 0;
  //boost::hash_combine(hash, job_desc.uuid());
  boost::hash_combine(hash, job_desc.root_task().binary());
  boost::hash_combine(hash, job_desc.root_task().name());

  return static_cast<TaskID_t>(hash);
}

TaskID_t GenerateTaskID(const TaskDescriptor& parent_task) {
  // A new task's ID is a hash of the parent (spawning) task's ID and its
  // current spawn counter value, which is implicitly stored in the TD by means
  // of the length of its set of spawned tasks.
  size_t hash = 0;
  boost::hash_combine(hash, parent_task.uid());
  boost::hash_combine(hash, parent_task.spawned_size());
  return static_cast<TaskID_t>(hash);
}

DataObjectID_t GenerateDataObjectID(const TaskDescriptor& producing_task) {
  // A thin shim that converts to the signature of GenerateDataObjectID.
  return GenerateDataObjectID(producing_task.uid(),
                              producing_task.outputs_size());
}

DataObjectID_t GenerateDataObjectID(
    TaskID_t producing_task, TaskOutputID_t output_id) {
  // A new data object ID is allocated by hashing the ID of the producing task
  // and the ID of the output (which may be greater than the number of declared
  // output IDs, since tasks can produce extra outputs).
  // TODO(malte): This is not *quite* the same as CIEL's naming scheme (which is
  // a little cleverer and uses the task argument structure here), but works for
  // now. Revisit later.
  uint8_t hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, &producing_task, sizeof(TaskID_t));
  SHA256_Update(&ctx, &output_id, sizeof(TaskOutputID_t));
  SHA256_Final(hash, &ctx);
  DataObjectID_t doid(hash);
  return doid;
}

DataObjectID_t DataObjectIDFromString(const string& str) {
  // N.B.: This assumes that the string is a readable, hexadecimal
  // representation of the ID.
  DataObjectID_t object_id(str, true);
  return object_id;
}

DataObjectID_t DataObjectIDFromProtobuf(const string& str) {
  // N.B.: This assumes that the string is a binary representation of the ID.
  DataObjectID_t object_id(str, false);
  return object_id;
}

DataObjectID_t DataObjectIDFromProtobuf(const ReferenceDescriptor& rd) {
  // N.B.: This assumes that the string is a binary representation of the ID.
  DataObjectID_t object_id(rd.id(), false);
  return object_id;
}


JobID_t JobIDFromString(const string& str) {
  // XXX(malte): This makes assumptions about JobID_t being a Boost UUID. We
  // should have a generic "JobID_t-from-string" helper instead.
#ifdef __PLATFORM_HAS_BOOST__
  boost::uuids::string_generator gen;
  boost::uuids::uuid job_uuid = gen(str);
#else
  string job_uuid = str;
#endif
  return job_uuid;
}

ResourceID_t ResourceIDFromString(const string& str) {
  // XXX(malte): This makes assumptions about ResourceID_t being a Boost UUID.
  // We should have a generic "JobID_t-from-string" helper instead.
#ifdef __PLATFORM_HAS_BOOST__
  boost::uuids::string_generator gen;
  boost::uuids::uuid res_uuid = gen(str);
#else
  string res_uuid = str;
#endif
  return res_uuid;
}

TaskID_t TaskIDFromString(const string& str) {
  stringstream strm(str);
  TaskID_t task_uuid;
  strm >> task_uuid;
  return task_uuid;
}

// Pipe setup
// outfd[0] == PARENT_READ
// outfd[1] == CHILD_WRITE
// infd[0] == CHILD_READ
// infd[1] == PARENT_WRITE
int32_t ExecCommandSync(const string& cmdline, vector<string> args,
                        int infd[2], int outfd[2]) {
  VLOG(2) << "Executing externally: " << cmdline;
  pid_t pid;
  if (pipe(infd) != 0) {
    LOG(ERROR) << "Failed to create pipe to task.";
  }
  if (pipe(outfd) != 0) {
    LOG(ERROR) << "Failed to create pipe from task.";
  }
  // Convert args from string to char*
  vector<char*> argv;
  // no performance monitoring, so we only need to reserve space for the
  // default and NULL args
  argv.reserve(args.size() + 1);
  argv.push_back((char*)(cmdline.c_str()));  // NOLINT
  for (uint32_t i = 0; i < args.size(); ++i) {
    // N.B.: This casts away the const qualifier on the c_str() result.
    // This is joyfully unsafe, of course.
    argv.push_back((char*)(args[i].c_str()));  // NOLINT
  }
  // The last argument to execvp is always NULL.
  argv.push_back(NULL);
  // Print the whole command line
  string full_cmd_line;
  for (vector<char*>::const_iterator arg_iter = argv.begin();
       arg_iter != argv.end();
       ++arg_iter) {
    if (*arg_iter != NULL) {
      full_cmd_line += *arg_iter;
      full_cmd_line += " ";
    }
  }
  LOG(INFO) << "External execution of command: " << full_cmd_line;
  pid = fork();
  switch (pid) {
    case -1:
      // Error
      PLOG(ERROR) << "Failed to fork child process.";
      break;
    case 0: {
      // Child
      // Close parent pipe descriptors
      close(infd[1]);
      close(outfd[0]);
      // set up pipes
      CHECK(dup2(infd[0], STDIN_FILENO) == STDIN_FILENO);
      CHECK(dup2(outfd[1], STDOUT_FILENO) == STDOUT_FILENO);
      // close unnecessary pipe descriptors
      close(infd[0]);
      close(outfd[1]);
      // Run the task binary
      execvp(argv[0], &argv[0]);
      // execl only returns if there was an error
      PLOG(ERROR) << "execvp failed for task command '" << full_cmd_line << "'";
      //ReportTaskExecutionFailure();
      _exit(1);
      break;
    }
    default:
      // Parent
      VLOG(1) << "Subprocess with PID " << pid << " created.";
      // close unused pipe ends
      close(infd[0]);
      close(outfd[1]);
      // TODO(malte): ReadFromPipe is a synchronous call that will only return
      // once the pipe has been closed! Check if this is actually the semantic
      // we want.
      // The fact that we cannot concurrently read from the STDOUT and the
      // STDERR pipe this way suggest the answer is that it is not...
      //ReadFromPipe(pipe_from[0]);
      //ReadFromPipe(pipe_from[1]);
      return pid;
  }
  return -1;
}

int32_t WaitForFinish(pid_t pid) {
  // Wait for task to terminate
  int status;
  while (waitpid(pid, &status, 0) != pid) {
    VLOG(2) << "Waiting for child process " << pid << " to exit...";
  };
  if (WIFEXITED(status)) {
    VLOG(1) << "Subprocess with PID " << pid << " exited with status "
            << WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    VLOG(1) << "Subprocess with PID " << pid << " exited due to uncaught "
            << "signal " << WTERMSIG(status);
  } else if (WIFSTOPPED(status)) {
    VLOG(1) << "Subprocess with PID " << pid << " is stopped due to "
            << "signal " << WSTOPSIG(status);
  } else {
    LOG(ERROR) << "Unexpected exit status: " << hex << status;
  }
  return status;
}

uint8_t* SHA256Hash(uint8_t* bytes, uint64_t len) {
  uint8_t* hash = new uint8_t[SHA256_DIGEST_LENGTH];
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, bytes, len);
  SHA256_Final(hash, &ctx);
  return hash;
}

// Helper function to convert a repeated bytes field to a set of
// DataObjectID_t.
// This method copies the input collection, so it is O(N) in time and space.
set<DataObjectID_t*> DataObjectIDsFromProtobuf(
    const RepeatedPtrField<string>& pb_field) {
  set<DataObjectID_t*> return_set;
  // N.B.: using GNU-style RTTI (typeof)
  for (__typeof__(pb_field.begin()) iter = pb_field.begin();
       iter != pb_field.end();
       ++iter)
    return_set.insert(new DataObjectID_t(*iter, false));
  return return_set;
}

// Helper function to convert a repeated bytes field to a set of
// DataObjectID_t.
// This method copies the input collection, so it is O(N) in time and space.
set<DataObjectID_t*> DataObjectIDsFromProtobuf(
    const RepeatedPtrField<ReferenceDescriptor>& pb_field) {
  set<DataObjectID_t*> return_set;
  // N.B.: using GNU-style RTTI (typeof)
  for (__typeof__(pb_field.begin()) iter = pb_field.begin();
       iter != pb_field.end();
       ++iter)
    return_set.insert(new DataObjectID_t(iter->id(), false));
  return return_set;
}


}  // namespace firmament
