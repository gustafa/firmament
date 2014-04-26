// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Main task library class.
// TODO(malte): This should really be made platform-independent, so that we can
// have platform-specific libraries.

#include "engine/task_lib.h"

#include <vector>

#include "base/common.h"
#include "base/data_object.h"
#include "messages/registration_message.pb.h"
#include "messages/task_heartbeat_message.pb.h"
#include "messages/task_info_message.pb.h"
#include "messages/task_spawn_message.pb.h"
#include "messages/task_state_message.pb.h"
#include "misc/utils.h"
#include "platforms/common.h"
#include "platforms/common.pb.h"

#include "examples/nginx/src/event/ngx_event.h"


// TODO(gustafa): FIX later!
DEFINE_string(coordinator_uri, "tcp:localhost:8088", "The URI to contact the coordinator at.");
DEFINE_string(resource_id, "feedcafedeadbeeffeedcafedeadbeeffeedcafedeadbeeffeedcafedeadbeef",
        "The resource ID that is running this task.");
DEFINE_string(task_id, "feedcafedeadbeeffeedcafedeadbeeffeedcafedeadbeeffeedcafedeadbeef", "The ID of this task.");
DEFINE_int32(heartbeat_interval, 1,
        "The interval, in seconds, between heartbeats sent to the"
        "coordinator.");

DEFINE_string(tasklib_application, "", "The application running alongside tasklib");



namespace firmament {
string TaskLib::web_contents = "";


// TODO(gustafa) fix these envs.
  TaskLib::TaskLib()
  : m_adapter_(new StreamSocketsAdapter<BaseMessage>()),
    chan_(new StreamSocketsChannel<BaseMessage>(
        StreamSocketsChannel<BaseMessage>::SS_TCP)),
    coordinator_uri_(getenv("FLAGS_coordinator_uri")),
    resource_id_(ResourceIDFromString(getenv("FLAGS_resource_id"))),
    pid_(getpid()),
    task_error_(false),
    task_running_(false),
    heartbeat_seq_number_(0),
    task_perf_monitor_(1000000) {
  const char* task_id_env;
  if (FLAGS_task_id.empty())
    task_id_env = getenv("FLAGS_task_id");
  else
    task_id_env = FLAGS_task_id.c_str();

  if (FLAGS_tasklib_application.empty())
    FLAGS_tasklib_application = getenv("FLAGS_tasklib_application");

  VLOG(1) << "Task ID is " << task_id_env;
  CHECK_NOTNULL(task_id_env);
  task_id_ = TaskIDFromString(task_id_env);
  setUpStorageEngine();
}

void TaskLib::AddTaskStatisticsToHeartbeat(
    const ProcFSMonitor::ProcessStatistics_t& proc_stats,
    TaskPerfStatisticsSample* stats) {
  // Task ID and timestamp
  stats->set_task_id(task_id_);
  stats->set_timestamp(GetCurrentTimestamp());
  // Memory allocated and used
  stats->set_vsize(proc_stats.vsize);
  stats->set_rsize(proc_stats.rss * getpagesize());
  // Scheduler statistics
  stats->set_sched_run(proc_stats.sched_run_ticks);
  stats->set_sched_wait(proc_stats.sched_wait_runnable_ticks);



  if (FLAGS_tasklib_application == "nginx") {
    AddNginxStatistics(stats->mutable_nginx_stats());
  }
}

void TaskLib::AwaitNextMessage() {
  // Finally, call back into ourselves.
  //AwaitNextMessage();
}

bool TaskLib::ConnectToCoordinator(const string& coordinator_uri) {
  return m_adapter_->EstablishChannel(coordinator_uri, chan_);
}

void TaskLib::Spawn(const ReferenceInterface& code,
                    vector<ReferenceInterface>* /*dependencies*/,
                    vector<FutureReference>* outputs) {
  VLOG(1) << "Spawning a new task; code reference is " << code.desc().id();
  // Craft a task spawn message for the new task, using a newly created task
  // descriptor.
  BaseMessage msg;
  SUBMSG_WRITE(msg, task_spawn, creating_task_id, task_id_);
  //TaskDescriptor* new_task =
  //    msg.mutable_task_spawn()->mutable_spawned_task_desc();
  TaskDescriptor* new_task = task_descriptor_.add_spawned();
  new_task->set_uid(GenerateTaskID(task_descriptor_));
  VLOG(1) << "New task's ID is " << new_task->uid();
  new_task->set_name("");
  new_task->set_state(TaskDescriptor::CREATED);
  // XXX(malte): set the code ref of the new task
  //new_task->set_binary(code);
  // Create the outputs of the new task
  uint64_t i = 0;
  for (vector<FutureReference>::iterator out_iter = outputs->begin();
       out_iter != outputs->end();
       ++out_iter) {
    DataObjectID_t new_output_id = GenerateDataObjectID(*new_task);
    VLOG(1) << "Output " << i << "'s ID: " << new_output_id;
    ReferenceDescriptor* out_rd = new_task->add_outputs();
    out_rd->CopyFrom(out_iter->desc());
    out_rd->set_id(new_output_id.name_bytes(), DIOS_NAME_BYTES);
    out_rd->set_producing_task(new_task->uid());
  }
  // Job ID field must be set on task spawn
  CHECK(task_descriptor_.has_job_id());
  new_task->set_job_id(task_descriptor_.job_id());
  // Copy the new task descriptor into the message
  msg.mutable_task_spawn()->mutable_spawned_task_desc()->CopyFrom(*new_task);
  // Off we go!
  SendMessageToCoordinator(&msg);
}

void TaskLib::Publish(const vector<ConcreteReference>& /*references*/) {
  LOG(ERROR) << "Output publication currently unimplemented!";
}

void TaskLib::ConvertTaskArgs(int argc, char *argv[], vector<char*>* arg_vec) {
  VLOG(1) << "Stripping Firmament default arguments...";
  for (int64_t i = 0; i < argc; ++i) {
    if (strstr(argv[i], "--tryfromenv") || strstr(argv[i], "--v")) {
      // Ignore standard arguments that refer to Firmament's task_lib, rather
      // than being passed through to the user code.
      continue;
    } else {
      arg_vec->push_back(argv[i]);
    }
    VLOG(1) << arg_vec->size() << " out of " << argc << " original arguments "
            << "remain.";
  }
}

void TaskLib::HandleWrite(const boost::system::error_code& error,
        size_t bytes_transferred) {
  VLOG(1) << "In HandleWrite, thread is " << boost::this_thread::get_id();
  if (error)
    LOG(ERROR) << "Error returned from async write: " << error.message();
  else
    VLOG(1) << "bytes_transferred: " << bytes_transferred;
}

bool TaskLib::PullTaskInformationFromCoordinator(TaskID_t task_id,
                                                 TaskDescriptor* desc) {
  // Send request for task information to coordinator
  BaseMessage msg;
  SUBMSG_WRITE(msg, task_info_request, task_id, task_id);
  SUBMSG_WRITE(msg, task_info_request, requesting_resource_id,
               to_string(resource_id_));
  SUBMSG_WRITE(msg, task_info_request, requesting_endpoint,
               chan_->LocalEndpointString());
  Envelope<BaseMessage> envelope(&msg);
  CHECK(chan_->SendS(envelope));
  // Wait for a response
  BaseMessage response;
  Envelope<BaseMessage> recv_envelope(&response);
  chan_->RecvS(&recv_envelope);
  CHECK(response.has_task_info_response());
  desc->CopyFrom(SUBMSG_READ(response, task_info_response, task_desc));
  VLOG(1) << "Received TD: " << desc->DebugString();
  return true;
}


void TaskLib::RunMonitor(boost::thread::id main_thread_id) {
  ConnectToCoordinator(coordinator_uri_);
  VLOG(3) << "Setting up storage engine";
  setUpStorageEngine();
  VLOG(2) << "Finished setting up storage engine";


  task_running_ = true;
  printf("Setting up stats\n");

  ProcFSMonitor::ProcessStatistics_t current_stats;
  printf("Finished setting up stats\n");


  int FLAGS_heartbeat_interval = 1;

  // // This will check if the task thread has joined once every heartbeat
  // // interval, and go back to sleep if it has not.
  // // TODO(malte): think about whether we'd like some kind of explicit
  // // notification scheme in case the heartbeat interval is large.
  // while (!main_thread.timed_join(
  //         boost::posix_time::seconds(FLAGS_heartbeat_interval))) {
  while (true) {
      sleep(FLAGS_heartbeat_interval);
          // TODO(malte): Check if we've exited with an error
    // if(error)
    //   task_error_ = true;
    // Notify the coordinator that we're still running happily
      //printf("Stat handled: %lu\n", *ngx_connection_counter);
      //printf("my_shared from tasklib: %d\n",my_shared);
      // TODO(gustafa): Move to a different function, do not assume nginx!


      VLOG(1) << "Task thread has not yet joined, sending heartbeat...";
      printf("HEARTHEATIN\n");
      task_perf_monitor_.ProcessInformation(pid_, &current_stats);
      SendHeartbeat(current_stats);
    // TODO(malte): We'll need to receive any potential messages from the
    // coordinator here, too. This is probably best done by a simple RecvA on
    // the channel.
    }

  task_running_ = false;

}

void TaskLib::AddNginxStatistics(TaskPerfStatisticsSample::NginxStatistics *ns) { 
    CURLcode curl_code = GetWebpageContents("localhost/nginx_status");
    if (curl_code == CURLE_OK) {
      printf("%s\n", web_contents.c_str());
      ns->set_status(TaskPerfStatisticsSample_NginxStatistics_Status_OK);

      const string delimiter = ",";
      
      string token;
      int pos = 0;
      int i = 0;

      const int num_stats = 4;
      unsigned long values[num_stats];




      while ((pos = web_contents.find(delimiter)) != std::string::npos) {
        token = web_contents.substr(0, pos);
        std::cout << token << std::endl;
        web_contents.erase(0, pos + delimiter.length());

        values[i] = strtoul(token.c_str(), NULL, 0);
        ++i;
      }

      if (i != num_stats) {
        LOG(ERROR) << "Found an unexpected number of Nginx statistics!";
      }

      // TODO(gustafa): Send diffs rather than totals.
      ns->set_active_connections(values[0]);
      ns->set_reading(values[1]);
      ns->set_writing(values[2]);
      ns->set_waiting(values[3]);

      for (int i = 0; i < num_stats; ++i) {
        printf("%lu\n", values[i]);
      }
   } else {
      ns->set_status(TaskPerfStatisticsSample_NginxStatistics_Status_DOWN);
   }
}


void TaskLib::SendFinalizeMessage(bool success) {
  BaseMessage bm;
  SUBMSG_WRITE(bm, task_state, id, task_id_);
  if (success)
    SUBMSG_WRITE(bm, task_state, new_state, TaskDescriptor::COMPLETED);
  else
    LOG(FATAL) << "Unimplemented error path!";
  VLOG(1) << "Sending finalize message (task state change to "
          << (success ? "COMPLETED" : "FAILED") << ")!";
  //SendMessageToCoordinator(&bm);
  Envelope<BaseMessage> envelope(&bm);
  CHECK(chan_->SendS(envelope));
  VLOG(1) << "Done sending message, sleeping before quitting";
  boost::this_thread::sleep(boost::posix_time::seconds(1));
}


void TaskLib::SendHeartbeat(const ProcFSMonitor::ProcessStatistics_t& proc_stats) {
  BaseMessage bm;
  printf("Writing bytes\n");
  SUBMSG_WRITE(bm, task_heartbeat, task_id, task_id_);
  // Add current set of procfs statistics

  printf("Creating stats\n");
  TaskPerfStatisticsSample* taskperf_stats =
      bm.mutable_task_heartbeat()->mutable_stats();
      printf("Adding sats to heartbeat\n");

  AddTaskStatisticsToHeartbeat(proc_stats, taskperf_stats);

  // TODO(malte): we do not always need to send the location string; it
  // sufficies to send it if our location changed (which should be rare).
  printf("Local endpoint\n");
  SUBMSG_WRITE(bm, task_heartbeat, location, chan_->LocalEndpointString());
  printf("Something else\n");
  SUBMSG_WRITE(bm, task_heartbeat, sequence_number, heartbeat_seq_number_++);

  printf("SENDING HEARTBEAT\n");
  VLOG(1) << "Sending heartbeat message!";
  SendMessageToCoordinator(&bm);
}


size_t TaskLib::StoreWebsite(void *ptr, size_t size, size_t nmemb, void *stream)  { 
    int numbytes = size*nmemb; 
    // The data is not null-terminated, so get the last character, and replace 
    // it with '\0'. 
    char lastchar = *((char *) ptr + numbytes - 1); 
    *((char *) ptr + numbytes - 1) = '\0'; 
    web_contents.append((char *)ptr); 
    web_contents.append(1,lastchar); 
    *((char *) ptr + numbytes - 1) = lastchar;  // Might not be necessary. 
    return size*nmemb; 
} 


CURLcode TaskLib::GetWebpageContents(const char *uri) {
  // Clear any old contents.
  web_contents = "";
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StoreWebsite);
    CURLcode res = curl_easy_perform(curl); 

    curl_easy_cleanup(curl);
    return res;
  } else {
    LOG(ERROR) << "Failed to instantiate curl!";
    exit(1);
  }
}

bool TaskLib::SendMessageToCoordinator(BaseMessage* msg) {
  Envelope<BaseMessage> envelope(msg);
  return chan_->SendA(
          envelope, boost::bind(&TaskLib::HandleWrite,
          this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
}

void TaskLib::setUpStorageEngine() {

    VLOG(1) << "Setting Up Storage Engine (TaskLib)" << endl;
  /* Contact coordinator to ask where storage engine is for this resource
   As currently, only assume that is local, don't currently need it
   Also need */
  try {
    const char* name = "Cache";

    // StorageDiscoverMessage* msg = new StorageDiscoverMessage();
    //
    // msg->set_uuid(resource_id_);
    // msg->uri("");
    //
    // SendMessageToCoordinator(msg);
    // Expect reply with URI

    // If local

    managed_shared_memory* segment = new managed_shared_memory(open_only, name);

    cache = new Cache_t(0);

    cache->object_list = (segment->find<SharedVector_t > ("objects")).first;

    cache->capacity = *((segment->find<size_t > ("capacity")).first);
    cache->size = *((segment->find<size_t > ("size")).first);

    mutex = new named_mutex(open_only, name);
    cache_lock = new scoped_lock<named_mutex > (*mutex, defer_lock);

    reference_not_t =
            (segment->find<ReferenceNotification_t>("refnot")).first;

    VLOG(1) << "Cache created with capacity: " << cache->capacity << " size "
            << cache->size << endl;
  } catch (const interprocess_exception& e) {  // NOLINT
    LOG(ERROR) << e.what();
    LOG(ERROR) << "Error: storage engine coudn't be initialised (TaskLib) ";
  }
  // TODO(tach): Else if not local - not implemented
}

// Finds data from cache and returns pointer. Acquires shared
// read lock. Contacts storage engine if data is not in cache
void* TaskLib::GetObjectStart(const DataObjectID_t& id) {
  cout << "GetObjectStart " << id << endl;
  try {
    cout << "Cache Capacity " << cache->capacity
         << " Cache size " << cache->size << endl;
    string nm = boost::lexical_cast<string > (id);
    if (cache->capacity != cache->size) {
      //SharedVector_t::iterator result =
      //        find(cache->object_list->begin(),
      //        cache->object_list->end(), id);
      // Should only be one result in theory
      cout << "Object " << id << " was  found in cache." << endl;

      cout << "Obtaining mutex for " << nm << endl;
      string mutex_name = nm + "mut";
      named_upgradable_mutex mut(open_only, mutex_name.c_str());
      ReadLock_t rlock(mut);
      cout << "Obtaining lock on mutex at " << rlock.mutex() << endl;
      //rlock.lock();
      cout << "Lock obtained" <<  endl;

      cout << "Obtaining file mapping for " << nm << endl;
      file_mapping m_file(nm.c_str(), read_only);
      cout << "Obtaining mapped region" << endl;
      mapped_region region(m_file, read_only);
      void* object = region.get_address();
      cout << "Mapped object address is " << object << endl;
      return object;
    } else {
      cout << "Object " << id << " was not found in cache. Contacting "
              "Object Store " << endl;

      // Data is not in cache - Let object store do the work
      // Really ought to use memory channels

      // Not fully implemented. Refactor so that
      // the whole thing is asynchronous?

      scoped_lock<interprocess_mutex> lock_ref(reference_not_t->mutex);
      cout << "Acquired reference mutex " << endl;
      while (!reference_not_t->writable) {
        cout << "Waiting for reference to become writable " << endl;
        reference_not_t->cond_read.wait(lock_ref);
      }
      cout << "Writing Message " << endl;
      reference_not_t->rd.set_id(*id.name_str());
      reference_not_t->writable = false;
      reference_not_t->request_type = GET_OBJECT;
      lock_ref.unlock(); /* Not sure if this is necessary */
      cout << "Notify storage engine of new message " << endl;
      reference_not_t->cond_added.notify_one(); // Storage engine will be
                                                // only one waiting on this

      while (!reference_not_t->writable ||
             !(reference_not_t->request_type == GET_OBJECT )) {
        cout << "Wait for reply " << endl;
        reference_not_t->cond_read.wait(lock_ref);
      }

      if (!reference_not_t->success) {
        cout << "Object was never found " << endl;
        return NULL;
      } else {
        //SharedVector_t::iterator result =
        //        find(cache->object_list->begin(),
        //             cache->object_list->end(), id);
        // Should only be one result in theory
        cout << "Object " << id << " was now found in cache." << endl;

        string mutex_name = nm + "mut";
        named_upgradable_mutex mut(open_only, mutex_name.c_str());
        ReadLock_t rlock(mut);
        rlock.lock();

        file_mapping m_file(nm.c_str(), read_only);
        mapped_region region(m_file, read_only);
        void* object = region.get_address();
        return object;
      }

      reference_not_t->writable = true;
      reference_not_t->request_type = FREE;
//      lock_ref.unlock(); /* Not sure if this is necessary */
      reference_not_t->cond_added.notify_all(); // Storage engine will be
                                                // only one waiting on this
    }
  } catch (const interprocess_exception& e) {  // NOLINT
    cout << "Error: GetObjectStart " << endl;
    cout << "Error: " << e.what() << endl;
  }
  return NULL;
}

/* Releases shared read lock */
void TaskLib::GetObjectEnd(const DataObjectID_t& id) {
  cout << "GetObjectEnd " << endl;
  try {
    // TODO(tach): very inefficient
    string mut_name_s = boost::lexical_cast<string > (id) + "mut";
    named_upgradable_mutex mut(open_only, mut_name_s.c_str());
    ReadLock_t lock(mut, accept_ownership);
    //      string nm = boost::lexical_cast<string>(id);
    //      file_mapping::remove(nm.c_str());
    lock.unlock();
  } catch (const interprocess_exception& e) {  // NOLINT
    cout << "Error: GetObjectEnd " << endl;
    cout << "Error: " << e.what();
  }
}

// Return null if cache is full
void* TaskLib::PutObjectStart(const DataObjectID_t& id, size_t size) {
  try {
    cout << "PutObjectStart " << endl;

    cache_lock->lock();
    if (cache->capacity > size) {
      cache->capacity -= size;
    } else {
      cout << "Fail to create object" << id << ". Cache full ";
      cache_lock->unlock();
      return NULL;
    }
    cache_lock->unlock();


    string file_name = to_string(id);
    cout << "file_name is: " << file_name << endl;

    // Create file
    cout << "About to create file" << endl;
    std::ofstream ofs(file_name.c_str(), std::ios::binary | std::ios::out);
    cout << "File created: " << file_name << endl;
    cout << "Create done, failbit: " << ofs.fail() << "!" << endl;
    ofs.seekp(size - 1);
    cout << "Seek done, failbit: " << ofs.fail() << "!" << endl;
    ofs.write("", 1);
    cout << "Write done, failbit: " << ofs.fail() << "!" << endl;
    ofs.close();

    // Map it
    permissions permission;
    permission.set_unrestricted();

    // Should delete in PutObjectEnd.
    // TODO(tach): Modify to have coherent picture
    // of stack vs memory allocated
    cout << "About to map file" << endl;
    file_mapping* m_file = new file_mapping(file_name.c_str(), read_write);
    cout << "file_mapping created: " << m_file << endl;
    mapped_region*  region = new mapped_region(*m_file, read_write, 0, size);

    cout << "File Mapped  of size " << region->get_size() << endl;
    void* address = region->get_address();

    string mut_name_s = to_string(id) + "mut";
    named_upgradable_mutex mut(open_or_create, mut_name_s.c_str(), permission);

    cout << "Mutex created " << endl;

    /* Don't think this is correct. Find source of deadlock
     * Should be able to simply do WriteLock_t lock(mut) */
    WriteLock_t lock(mut, accept_ownership);

    cout << "Lock created " << endl;
    cout << " Write lock acquired on file " << endl;
    /* Add it to cache */
    cache->object_list->push_back(id);
    cout << "Added list to cache" << endl;
    cout << " New capacity " << cache->capacity << endl;
    return address;
  } catch (const interprocess_exception& e) {  // NOLINT
    cout << "Error: PutObjectStart " << endl;
    cout << "Error: " << e.what() << endl;
  }
  return NULL;
}

// Tries to extend file by "extend_by" bytes. Returns void if failed
// ex: cache full. Find a way to do this without unmapping/remapping the
// file
void* TaskLib::Extend(const DataObjectID_t& id, size_t old_size,
                      size_t new_size) {
  void* address = NULL;
  //    named_mutex mutex_(open_only, "Cache");
  //    scoped_lock<named_mutex> cache_locks(mutex_);
  cache_lock->lock();
  size_t extend_by = new_size - old_size;
  if (cache->capacity > extend_by) {
    cache->capacity -= extend_by;
  } else {
    VLOG(3) << "Fail to extend. Cache full ";
    cache_lock->unlock();
    return address;
  }
  cache_lock->unlock();
  string file_name = boost::lexical_cast<string > (id);
  file_mapping* m_file = new file_mapping(file_name.c_str(), read_write);
  mapped_region* region = new mapped_region(*m_file, read_write);
  region->flush();
//   file_mapping::remove(file_name.c_str());
  std::ofstream ofs(file_name.c_str(), std::ios::binary);
  ofs.seekp(new_size - 1);
  ofs.write("", 1);
  m_file = new file_mapping(file_name.c_str(), read_write);
  region = new mapped_region(*m_file, read_write);
  address = region->get_address();

  return address;
}

/* Release exclusive lock  - value of size actually written */
void TaskLib::PutObjectEnd(const DataObjectID_t& id, size_t size) {
  cout << "PutObjectEnd " << endl;
  try {
    string file_name = boost::lexical_cast<string > (id);
    file_mapping*  m_file = new file_mapping(file_name.c_str(), read_write);
    mapped_region* region  = new mapped_region(*m_file, read_write);
    region->flush();
    file_name += "mut";
    named_upgradable_mutex mut(open_only, file_name.c_str());
    WriteLock_t lock(mut, accept_ownership);
    lock.unlock();

    /* Notify Engine that reference is now concrete */
    scoped_lock<interprocess_mutex> lock_ref(reference_not_t->mutex);
    while (!reference_not_t->writable) {
      reference_not_t->cond_read.wait(lock_ref);
    }

    reference_not_t->rd.set_id(*id.name_str());
    reference_not_t->writable = false;
    reference_not_t->size = size;
    reference_not_t->request_type = PUT_OBJECT;
    lock_ref.unlock(); /* Not sure if this is necessary */
    reference_not_t->cond_added.notify_all();
  } catch (const interprocess_exception& e) {  // NOLINT
    VLOG(1) << "Error: PutObjectEnd " << endl;
  }
}


} // namespace firmament
