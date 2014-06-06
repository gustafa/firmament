// The Firmament project
// Copyright (c) 2011-2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Platform-independent coordinator class implementation. This is subclassed by
// the platform-specific coordinator classes.

#include "engine/coordinator.h"

#include <set>
#include <string>
#include <utility>

#ifdef __PLATFORM_HAS_BOOST__
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#endif

#include <google/protobuf/descriptor.h>

#include "base/resource_desc.pb.h"
#include "base/resource_topology_node_desc.pb.h"
#include "base/task_final_report.pb.h"
#include "storage/simple_object_store.h"
#include "messages/base_message.pb.h"
#include "misc/pb_utils.h"
#include "misc/protobuf_envelope.h"
#include "misc/map-util.h"
#include "misc/utils.h"
#include "scheduling/scheduling_parameters.pb.h"
#include "scheduling/energy_scheduler.h"
#include "scheduling/simple_scheduler.h"
#include "scheduling/quincy_scheduler.h"
#include "messages/storage_registration_message.pb.h"
#include "messages/storage_message.pb.h"


// It is necessary to declare listen_uri here, since "node.o" comes after
// "coordinator.o" in linking order (I *think*).
DECLARE_string(listen_uri);
DEFINE_string(parent_uri, "", "The URI of the parent coordinator to register "
        "with.");
DEFINE_bool(include_local_resources, true, "Add local machine's resources; "
            "will instantiate a resource-less coordinator if false.");
DEFINE_string(scheduler, "simple", "Scheduler to use: one of 'simple' or "
              "'quincy'.");
#ifdef __HTTP_UI__
DEFINE_bool(http_ui, true, "Enable HTTP interface");
DEFINE_int32(http_ui_port, 8080,
        "The port that the HTTP UI will be served on; -1 to disable.");
#endif
DEFINE_uint64(heartbeat_interval, 1000000,
              "Heartbeat interval in microseconds.");

DEFINE_uint64(sleep_time, 100,
              "Sleep time interval in milliseconds.");

DEFINE_uint64(energy_stats_history, 600,
              "Time history is kept for energy stats");

DEFINE_uint64(energy_stat_interval, 3,
              "Time between energy stats messages");

DEFINE_uint64(reconsider_web_interval, 3000000, "Interval in microseconds for the scheduler "
              "to reconsider webrequests");

DEFINE_bool(master_scheduler_on, true, "Whether to use the master scheduler option");

namespace firmament {


  bool first_stupid = true;

Coordinator::Coordinator(PlatformID platform_id)
  : Node(platform_id, GenerateUUID()),
    hostname_(boost::asio::ip::host_name()),
    associated_resources_(new ResourceMap_t),
    local_resource_topology_(new ResourceTopologyNodeDescriptor),
    job_table_(new JobMap_t),
    task_table_(new TaskMap_t),
    resource_to_host_(new ResourceHostMap_t),
    topology_manager_(new TopologyManager()),
    object_store_(new store::SimpleObjectStore(uuid_)),
    parent_chan_(NULL),
    knowledge_base_(new KnowledgeBase()),
    haproxy_controller_(new HAProxyController("my_servers")) {
  // Start up a coordinator according to the platform parameter
  string desc_name = "Coordinator on " + hostname_;
  resource_desc_.set_uuid(to_string(uuid_));
  resource_desc_.set_friendly_name(desc_name);
  resource_desc_.set_type(ResourceDescriptor::RESOURCE_MACHINE);
  resource_desc_.set_storage_engine(object_store_->get_listening_interface());
  local_resource_topology_->mutable_resource_desc()->CopyFrom(
      resource_desc_);

  // Set up the scheduler
  if (FLAGS_scheduler == "simple") {
    // Simple random first-available scheduler
    LOG(INFO) << "Using simple random scheduler.";
    scheduler_ = new SimpleScheduler(
        job_table_, associated_resources_, *local_resource_topology_,
        object_store_, task_table_, topology_manager_, m_adapter_,
        uuid_, FLAGS_listen_uri);
  } else if (FLAGS_scheduler == "quincy") {
    // Quincy-style flow-based scheduling
    LOG(INFO) << "Using Quincy-style min cost flow-based scheduler.";
    SchedulingParameters params;
    scheduler_ = new QuincyScheduler(
        job_table_, associated_resources_, *local_resource_topology_,
        object_store_, task_table_, topology_manager_, m_adapter_, uuid_,
        FLAGS_listen_uri, params);
  } else if (FLAGS_scheduler == "energy") {
        SchedulingParameters params;
    scheduler_ = new EnergyScheduler(
        job_table_, associated_resources_, *local_resource_topology_,
        object_store_, task_table_, topology_manager_, m_adapter_, uuid_,
        FLAGS_listen_uri, params, knowledge_base_, haproxy_controller_, resource_to_host_);
  } else {
    // Unknown scheduler specified, error.
    LOG(FATAL) << "Unknown or unrecognized scheduler '" << FLAGS_scheduler
               << " specified on coordinator command line!";
  }

  // Log information
  LOG(INFO) << "Coordinator starting on host " << FLAGS_listen_uri
          << ", platform " << platform_id << ", uuid " << uuid_;
  LOG(INFO) << "Storage Engine is listening on interface : "
            << object_store_->get_listening_interface();
  switch (platform_id) {
      case PL_UNIX:
      {
          break;
      }
      default:
          LOG(FATAL) << "Unimplemented!";
  }
  ready_to_rumble_ = FLAGS_include_local_resources;
  am_master_scheduler_ = !FLAGS_include_local_resources;
}

Coordinator::~Coordinator() {
  // TODO(malte): check destruction order in C++; c_http_ui_ may already
  // have been destructed when we get here.
  /*#ifdef __HTTP_UI__
    if (FLAGS_http_ui && c_http_ui_)
      c_http_ui_->Shutdown(false);
  #endif*/
}

bool Coordinator::RegisterWithCoordinator(
    StreamSocketsChannel<BaseMessage>* chan) {
  BaseMessage bm;
  ResourceDescriptor* rd = bm.mutable_registration()->mutable_res_desc();
  rd->CopyFrom(resource_desc_); // copies current local RD!
  ResourceTopologyNodeDescriptor* rtnd =
      bm.mutable_registration()->mutable_rtn_desc();
  rtnd->CopyFrom(*local_resource_topology_);
  SUBMSG_WRITE(bm, registration, uuid, to_string(uuid_));
  SUBMSG_WRITE(bm, registration, location, chan->LocalEndpointString());
  SUBMSG_WRITE(bm, registration, hostname, hostname_);

  // wrap in envelope
  VLOG(2) << "Sending registration message...";
  // send heartbeat message
  return SendMessageToRemote(chan, &bm);
}

void Coordinator::DetectLocalResources() {
  // Inform the user about the number of local PUs.
  uint64_t num_local_pus = topology_manager_->NumProcessingUnits();
  LOG(INFO) << "Found " << num_local_pus << " local PUs.";
  LOG(INFO) << "Resource URI is " << node_uri_;
  // Get local resource topology and save it to the topology protobuf
  // TODO(malte): Figure out how this interacts with dynamically added
  // resources; currently, we only run detection (i.e. the DetectLocalResources
  // method) once at startup.
  ResourceTopologyNodeDescriptor* root_node =
      local_resource_topology_->add_children();
  topology_manager_->AsProtobuf(root_node);
  root_node->set_parent_id(to_string(uuid_));
  root_node->mutable_resource_desc()->set_parent(to_string(uuid_));
  resource_desc_.add_children(root_node->resource_desc().uuid());
  TraverseResourceProtobufTree(
      local_resource_topology_,
      boost::bind(&Coordinator::AddResource, this, _1, node_uri_, hostname_, true));
}

void Coordinator::AddResource(ResourceDescriptor* resource_desc,
                              const string& endpoint_uri,
                              const string& hostname,
                              bool local) {
  CHECK(resource_desc);
  // Compute resource ID
  ResourceID_t res_id = ResourceIDFromString(resource_desc->uuid());
  // Add resource to local resource set
  VLOG(1) << "Adding resource " << res_id << " to resource map; "
          << "endpoint URI is " << endpoint_uri;
  //CHECK(

  // Store a resource->host lookup.
  InsertIfNotPresent(resource_to_host_.get(), res_id, hostname);

  InsertIfNotPresent(associated_resources_.get(), res_id,
          new ResourceStatus(resource_desc, endpoint_uri,
                             GetCurrentTimestamp()));
  // //);
  // // Store the resource to host information in the lookup map.
  // (*resource_to_host_)[res_id] = endpoint_uri;
  // Register with scheduler if this resource is schedulable
  if (resource_desc->type() == ResourceDescriptor::RESOURCE_PU) {
    // TODO(malte): We make the assumption here that any local PU resource is
    // exclusively owned by this coordinator, and set its state to IDLE if it is
    // currently unknown. If coordinators were to ever shared PUs, we'd need
    // something more clever here.
    resource_desc->set_schedulable(true);
    if (resource_desc->state() == ResourceDescriptor::RESOURCE_UNKNOWN)
      resource_desc->set_state(ResourceDescriptor::RESOURCE_IDLE);
    scheduler_->RegisterResource(res_id, local);
    VLOG(1) << "Added " << (local ? "local" : "remote") << " resource "
            << resource_desc->uuid()
            << " [" << resource_desc->friendly_name()
            << "] to scheduler.";
  }
}

void Coordinator::Run() {
  // Test topology detection
  LOG(INFO) << "Detecting resource topology:";
  topology_manager_->DebugPrintRawTopology();
  if (FLAGS_include_local_resources)
    DetectLocalResources();

  // Coordinator starting -- set up and wait for workers to connect.
  m_adapter_->ListenURI(FLAGS_listen_uri);
  m_adapter_->RegisterAsyncMessageReceiptCallback(
          boost::bind(&Coordinator::HandleIncomingMessage, this, _1, _2));
  m_adapter_->RegisterAsyncErrorPathCallback(
          boost::bind(&Coordinator::HandleIncomingReceiveError, this,
          boost::asio::placeholders::error, _2));

#ifdef __HTTP_UI__
  InitHTTPUI();
#endif

  // Do we have a parent? If so, register with it now.
  if (FLAGS_parent_uri != "") {
    parent_chan_ =
        new StreamSocketsChannel<BaseMessage > (
            StreamSocketsChannel<BaseMessage>::SS_TCP);
    CHECK(ConnectToRemote(FLAGS_parent_uri, parent_chan_))
        << "Failed to connect to parent at " << FLAGS_parent_uri << "!";
    VLOG(1) << parent_chan_->LocalEndpointString();
    VLOG(1) << parent_chan_->RemoteEndpointString();
    RegisterWithCoordinator(parent_chan_);
    InformStorageEngineNewResource(&resource_desc_);
  }

  uint64_t cur_time = 0;
  uint64_t last_heartbeat_time = 0;
  uint64_t last_webserver_reconsider_time = 0;
  // Main loop
  while (!exit_) {
    // Wait for events (i.e. messages from workers.
    // TODO(malte): we need to think about any actions that the coordinator
    // itself might need to take, and how they can be triggered
    VLOG(3) << "Hello from main loop!";
    AwaitNextMessage();
    // TODO(malte): wrap this in a timer
    cur_time = GetCurrentTimestamp();
    if (cur_time - last_heartbeat_time > FLAGS_heartbeat_interval) {
      MachinePerfStatisticsSample stats;
      stats.set_timestamp(GetCurrentTimestamp());
      stats.set_resource_id(to_string(uuid_));
      machine_monitor_.CreateStatistics(&stats);
      // Record this sample locally
      knowledge_base_->AddMachineSample(stats);
      if (parent_chan_ != NULL) {
        SendHeartbeatToParent(stats);
      }
      last_heartbeat_time = cur_time;
    }

    if ((cur_time - last_webserver_reconsider_time > FLAGS_reconsider_web_interval) && ready_to_rumble_ && parent_chan_ == NULL) {
      last_webserver_reconsider_time = GetCurrentTimestamp();
      // TODO IssueWebserverJobs();
    }

    boost::this_thread::sleep(boost::posix_time::milliseconds(FLAGS_sleep_time));
  }

  // We have dropped out of the main loop and are exiting
  // TODO(malte): any cleanup we need to do; hand-over to another coordinator if
  // possible?
  Shutdown("dropped out of main loop");
}

const JobDescriptor* Coordinator::DescriptorForJob(const string& job_id) {
  JobID_t job_uuid = JobIDFromString(job_id);
  JobDescriptor *jd = FindOrNull(*job_table_, job_uuid);
  return jd;
}

void Coordinator::HandleIncomingMessage(BaseMessage *bm,
                                        const string& remote_endpoint) {
  uint32_t handled_extensions = 0;
  // Registration message
  if (bm->has_registration()) {
    const RegistrationMessage& msg = bm->registration();
    HandleRegistrationRequest(msg);
    handled_extensions++;
  }
  // Resource Heartbeat message
  if (bm->has_heartbeat()) {
    const HeartbeatMessage& msg = bm->heartbeat();
    HandleHeartbeat(msg);
    handled_extensions++;
  }
  // Task heartbeat message
  if (bm->has_task_heartbeat()) {
    const TaskHeartbeatMessage& msg = bm->task_heartbeat();
    HandleTaskHeartbeat(msg);
    handled_extensions++;
  }
  // Task state change message
  if (bm->has_task_state()) {
    const TaskStateMessage& msg = bm->task_state();
    HandleTaskStateChange(msg);
    handled_extensions++;
  }
  // Task spawn message
  if (bm->has_task_spawn()) {
    const TaskSpawnMessage& msg = bm->task_spawn();
    HandleTaskSpawn(msg);
    handled_extensions++;
  }
  // Task info request message
  if (bm->has_task_info_request()) {
    const TaskInfoRequestMessage& msg = bm->task_info_request();
    HandleTaskInfoRequest(msg, remote_endpoint);
    handled_extensions++;
  }
  // Storage engine registration
  if (bm->has_storage_registration()) {
    const StorageRegistrationMessage& msg = bm->storage_registration();
    HandleStorageRegistrationRequest(msg);
    handled_extensions++;
  }
  // Storage engine discovery
  if (bm->has_storage_discover()) {
    const StorageDiscoverMessage& msg = bm->storage_discover();
    HandleStorageDiscoverRequest(msg);
    handled_extensions++;
  }
  // Task delegation message
  if (bm->has_task_delegation()) {
    const TaskDelegationMessage& msg = bm->task_delegation();
    HandleTaskDelegationRequest(msg, remote_endpoint);
    handled_extensions++;
  }
  // DIOS syscall: create message
  if (bm->has_create_request()) {
    const CreateRequest& msg = bm->create_request();
    HandleCreateRequest(msg, remote_endpoint);
    handled_extensions++;
  }
  // DIOS syscall: lookup message
  if (bm->has_lookup_request()) {
    const LookupRequest& msg = bm->lookup_request();
    HandleLookupRequest(msg, remote_endpoint);
    handled_extensions++;
  }
  // DIOS I/O notification
  if (bm->has_end_write_notification()) {
    HandleIONotification(*bm, remote_endpoint);
    handled_extensions++;
  }

  if (bm->has_energy_stats()) {
    const EnergyStatsMessage &msg = bm->energy_stats();
    HandleEnergyStats(msg);
    handled_extensions++;
  }
  // Check that we have handled at least one sub-message
  if (handled_extensions == 0)
    LOG(ERROR) << "Ignored incoming message, no known extension present, "
               << "so cannot handle it: " << bm->DebugString();
}

void Coordinator::HandleIncomingReceiveError(
    const boost::system::error_code& error,
    const string& remote_endpoint) {
  // Notify of receive error
  if (error.value() == boost::asio::error::eof) {
    // Connection terminated, handle accordingly
    LOG(INFO) << "Connection to " << remote_endpoint << " closed.";
    // XXX(malte): Need to figure out if this relates to a resource, and if so,
    // if we should declare it failed; or whether this is an expected job
    // completion.
  } else {
    LOG(WARNING) << "Failed to complete a message receive cycle from "
                 << remote_endpoint << ". The message was discarded, or the "
                 << "connection failed (error: " << error.message() << ", "
                 << "code " << error.value() << ").";
  }
}



void Coordinator::HandleCreateRequest(const CreateRequest& msg,
                                      const string& remote_endpoint) {
  // Try to insert the reference descriptor conveyed into the object table.
  ReferenceDescriptor* new_rd = new ReferenceDescriptor;
  new_rd->CopyFrom(msg.reference());
  new_rd->set_producing_task(msg.task_id());
  bool succ = !object_store_->AddReference(
      DataObjectIDFromProtobuf(msg.reference().id()), new_rd);
  // Manufacture and send a response
  BaseMessage resp_msg;
  SUBMSG_WRITE(resp_msg, create_response, name, msg.reference().id());
  SUBMSG_WRITE(resp_msg, create_response, success, succ);
  m_adapter_->SendMessageToEndpoint(remote_endpoint, resp_msg);
}

void Coordinator::HandleHeartbeat(const HeartbeatMessage& msg) {
  boost::uuids::string_generator gen;
  boost::uuids::uuid uuid = gen(msg.uuid());
  ResourceStatus** rsp = FindOrNull(*associated_resources_, uuid);
  if (!rsp) {
      LOG(WARNING) << "HEARTBEAT from UNKNOWN resource (uuid: "
              << msg.uuid() << ")!";
  } else {
      LOG(INFO) << "HEARTBEAT from resource " << msg.uuid()
                << " (last seen at " << (*rsp)->last_heartbeat() << ")";
      if (msg.has_load())
        VLOG(3) << "Remote resource stats: " << msg.load().ShortDebugString();
      // Update timestamp
      (*rsp)->set_last_heartbeat(GetCurrentTimestamp());
      // Record resource statistics sample
      knowledge_base_->AddMachineSample(msg.load());
  }
}

void Coordinator::HandleIONotification(const BaseMessage& bm,
                                       const string& remote_uri) {
  if (bm.has_end_write_notification()) {
    const EndWriteNotification msg = bm.end_write_notification();
    DataObjectID_t id = DataObjectIDFromProtobuf(msg.reference().id());
    set<ReferenceInterface*>* refs = object_store_->GetReferences(id);
    vector<ReferenceInterface*> remove;
    vector<ConcreteReference*> add;
    for (set<ReferenceInterface*>::iterator it = refs->begin();
         it != refs->end();
         ++it) {
      if ((*it)->desc().type() == ReferenceDescriptor::FUTURE &&
          (*it)->desc().producing_task() == msg.reference().producing_task()) {
        // Upgrade to a concrete reference
        VLOG(2) << "Found future reference for " << id
                << ", upgrading to concrete!";
        remove.push_back(*it);
        // XXX(malte): skanky, skanky...
        add.push_back(new ConcreteReference(
            *dynamic_cast<FutureReference*>(*it)));  // NOLINT
      }
    }
    VLOG(1) << "Found " << remove.size() << " matching references for "
            << id << ", and converted them into concrete refs.";
    TaskDescriptor** td_ptr = FindOrNull(*task_table_,
                                         msg.reference().producing_task());
    CHECK_NOTNULL(td_ptr);
    for (uint64_t i = 0; i < remove.size(); ++i) {
      refs->erase(remove[i]);
      refs->insert(add[i]);
      scheduler_->HandleReferenceStateChange(*remove[i], *add[i], *td_ptr);
      delete remove[i];
    }
    // Call into scheduler, as this change may have made things runnable
    JobDescriptor* jd = FindOrNull(*job_table_,
                                   JobIDFromString((*td_ptr)->job_id()));
    scheduler_->ScheduleJob(jd);
  }
}

void Coordinator::HandleEnergyStats(const EnergyStatsMessage& msg) {
  //TODO(gustafa): Implement
  VLOG(2) << "Handling EnergyStatsMessage";
  // Add the energy statistics to the vector.

  //new WraparoundVector<double>(
      //FLAGS_energy_stats_history / FLAGS_energy_stat_interval)

  for (auto &em : msg.energy_messages()) {
    string hostname = em.hostname();
    WraparoundVector<double>* stats_vector = FindPtrOrNull(energy_stats_map, hostname);

    if (!stats_vector) {
      // Create enough elements to fit energy_stats_history length in seconds.
      stats_vector =
          new WraparoundVector<double>(FLAGS_energy_stats_history / FLAGS_energy_stat_interval, 0);
      energy_stats_map[hostname] = stats_vector;
    }
    VLOG(3) << hostname;
    CHECK(em.has_totalj());
    stats_vector->PushElement(em.totalj());
  }
}

void Coordinator::HandleLookupRequest(const LookupRequest& msg,
                                      const string& remote_endpoint) {
  // Check if the name requested exists in the object table, and return all
  // reference descriptors for it if so.
  // XXX(malte): This currently returns a single reference; we should return
  // multiple if they exist.
  set<ReferenceInterface*>* refs = object_store_->GetReferences(
      DataObjectIDFromProtobuf(msg.name()));
  // Manufacture and send a response
  BaseMessage resp_msg;
  if (refs && refs->size() > 0) {
    for (set<ReferenceInterface*>::const_iterator ref_iter = refs->begin();
         ref_iter != refs->end();
         ++ref_iter) {
      ReferenceDescriptor* resp_rd =
          resp_msg.mutable_lookup_response()->add_references();
      resp_rd->CopyFrom((*ref_iter)->desc());
    }
  }
  m_adapter_->SendMessageToEndpoint(remote_endpoint, resp_msg);
}

void Coordinator::HandleRegistrationRequest(
    const RegistrationMessage& msg) {
  boost::uuids::string_generator gen;
  boost::uuids::uuid uuid = gen(msg.uuid());
  ResourceStatus** rdp = FindOrNull(*associated_resources_, uuid);
  if (!rdp) {
    LOG(INFO) << "REGISTERING NEW RESOURCE (uuid: " << msg.uuid() << ")";
    // N.B.: below creates a new resource descriptor
    ResourceDescriptor* rd = new ResourceDescriptor(msg.res_desc());
    // Insert the root of the registered topology into the topology tree
    ResourceTopologyNodeDescriptor* rtnd =
        local_resource_topology_->add_children();
    rtnd->CopyFrom(msg.rtn_desc());
    rtnd->set_parent_id(resource_desc_.uuid());
    rtnd->mutable_resource_desc()->set_parent(resource_desc_.uuid());
    resource_desc_.add_children(rtnd->resource_desc().uuid());
    // Recursively add its child resources to resource map and topology tree
    TraverseResourceProtobufTree(
        rtnd, boost::bind(&Coordinator::AddResource, this, _1,
                          msg.location(), msg.hostname(), false));
    InformStorageEngineNewResource(rd);

    // If we were not ready to rumble before, we are now!
    ready_to_rumble_ = true;
  } else {
    LOG(INFO) << "REGISTRATION request from resource " << msg.uuid()
              << " that we already know about. "
              << "Checking if this is a recovery.";
    // TODO(malte): Implement checking logic, deal with recovery case
    // Update timestamp (registration request is an implicit heartbeat)
    (*rdp)->set_last_heartbeat(GetCurrentTimestamp());
  }
}

void Coordinator::HandleTaskHeartbeat(const TaskHeartbeatMessage& msg) {
  TaskID_t task_id = msg.task_id();
  TaskDescriptor** tdp = FindOrNull(*task_table_, task_id);
  if (!tdp) {
    LOG(WARNING) << "HEARTBEAT from UNKNOWN task (ID: "
                 << task_id << ")!";
  } else {
    LOG(INFO) << "HEARTBEAT from task " << task_id;
    // Remember the current location of this task
    (*tdp)->set_last_location(msg.location());
    // Process the profiling information submitted by the task, add it to
    // the knowledge base
    if (parent_chan_ != NULL) {
        BaseMessage bm;
        bm.mutable_task_heartbeat()->CopyFrom(msg);
        SendMessageToRemote(parent_chan_, &bm);
    } else {
      knowledge_base_->AddTaskSample(msg.stats());
    }
  }
}

void Coordinator::HandleTaskDelegationRequest(
    const TaskDelegationMessage& msg,
    const string& remote_endpoint) {
  VLOG(1) << "Handling requested delegation of task "
          << msg.task_descriptor().uid() << " from resource "
          << msg.delegating_resource_id();
  // Check if there is room for this task here
  // (or maybe enqueue it?)
  TaskDescriptor* td = new TaskDescriptor(msg.task_descriptor());
  bool result = scheduler_->PlaceDelegatedTask(
      td, ResourceIDFromString(msg.target_resource_id()));
  // Return ACK/NACK
  BaseMessage response;
  SUBMSG_WRITE(response, task_delegation_response, task_id, td->uid());
  if (result) {
    // Successfully placed
    VLOG(1) << "Succeeded, task placed on resource " << msg.target_resource_id()
            << "!";
    SUBMSG_WRITE(response, task_delegation_response, success, true);
  } else {
    // Failure; delegator needs to try again
    VLOG(1) << "Failed to place!";
    SUBMSG_WRITE(response, task_delegation_response, success, false);
  }
  m_adapter_->SendMessageToEndpoint(remote_endpoint, response);
}

void Coordinator::HandleTaskInfoRequest(const TaskInfoRequestMessage& msg,
                                        const string& remote_endpoint) {
  // Send response: the task descriptor if the task is known to this
  // coordinator
  VLOG(1) << "Resource " << msg.requesting_resource_id()
          << " requests task information for " << msg.task_id();
  TaskDescriptor** task_desc_ptr = FindOrNull(*task_table_, msg.task_id());
  CHECK_NOTNULL(task_desc_ptr);
  // Remember the current location of this task
  (*task_desc_ptr)->set_last_location(remote_endpoint);
  BaseMessage resp;
  // XXX(malte): ugly hack!
  SUBMSG_WRITE(resp, task_info_response, task_id, msg.task_id());
  resp.mutable_task_info_response()->
      mutable_task_desc()->CopyFrom(**task_desc_ptr);
  m_adapter_->SendMessageToEndpoint(remote_endpoint, resp);
}

void Coordinator::HandleTaskSpawn(const TaskSpawnMessage& msg) {
  VLOG(1) << "Handling task spawn for new task "
          << msg.spawned_task_desc().uid() << ", child of "
          << msg.creating_task_id();
  // Get the descriptor for the spawning task
  TaskDescriptor** spawner;
  CHECK(spawner = FindOrNull(*task_table_, msg.creating_task_id()));
  // Extract new task descriptor from the message received
  //TaskDescriptor* spawnee = new TaskDescriptor;
  TaskDescriptor* spawnee = (*spawner)->add_spawned();
  spawnee->CopyFrom(msg.spawned_task_desc());
  InsertIfNotPresent(task_table_.get(), spawnee->uid(), spawnee);
  // Extract job ID (we expect it to be set)
  CHECK(msg.spawned_task_desc().has_job_id());
  JobID_t job_id = JobIDFromString(msg.spawned_task_desc().job_id());
  // Find task graph for job
  TaskGraph** task_graph_pptr = FindOrNull(task_graph_table_, job_id);
  CHECK_NOTNULL(task_graph_pptr);
  TaskGraph* task_graph_ptr = *task_graph_pptr;
  VLOG(1) << "Task graph is at " << task_graph_ptr;
  // Add task to task graph
  //task_graph_ptr->AddChildTask(spawner, spawnee)
  // Update references with producing task, if necessary
  // TODO(malte): implement this properly; below is a hack that delegates
  // outputs by simple modifying their producing task.
  for (RepeatedPtrField<ReferenceDescriptor>::const_iterator o_iter =
       msg.spawned_task_desc().outputs().begin();
       o_iter != msg.spawned_task_desc().outputs().end();
       ++o_iter) {
    set<ReferenceInterface*>* refs =
        object_store_->GetReferences(DataObjectIDFromProtobuf(o_iter->id()));
    for (set<ReferenceInterface*>::iterator r_iter = refs->begin();
         r_iter != refs->end();
         ++r_iter) {
      if ((*r_iter)->desc().producing_task() == (*spawner)->uid())
        (*r_iter)->set_producing_task(spawnee->uid());
    }
  }
  // Run the scheduler for this job
  JobDescriptor* job = FindOrNull(*job_table_, job_id);
  CHECK_NOTNULL(job);
  uint64_t tasks_scheduled = scheduler_->ScheduleJob(job);
  LOG(INFO) << "Scheduled " << tasks_scheduled << " tasks for job "
            << job_id;
}

void Coordinator::HandleTaskStateChange(
    const TaskStateMessage& msg) {
  VLOG(1) << "Task " << msg.id() << " now in state "
          << ENUM_TO_STRING(TaskDescriptor::TaskState, msg.new_state())
          << ".";
  TaskDescriptor** td_ptr = FindOrNull(*task_table_, msg.id());
  CHECK(td_ptr) << "Received task state change message for task "
                << msg.id();
  // First check if this is a delegated task, and forward the message if so
  if ((*td_ptr)->has_delegated_from()) {
    BaseMessage bm;
    bm.mutable_task_state()->CopyFrom(msg);
    m_adapter_->SendMessageToEndpoint((*td_ptr)->delegated_from(), bm);
    return;
  }
  switch (msg.new_state()) {
    case TaskDescriptor::COMPLETED:
    {
      (*td_ptr)->set_state(TaskDescriptor::COMPLETED);
      TaskFinalReport report;
      scheduler_->HandleTaskCompletion(*td_ptr, &report);
      knowledge_base_->ProcessTaskFinalReport(report);
      break;
    }
    case TaskDescriptor::FAILED:
    {
      (*td_ptr)->set_state(TaskDescriptor::FAILED);
      scheduler_->HandleTaskFailure(*td_ptr);
      break;
    }
    default:
      VLOG(1) << "Task " << msg.id() << "'s state changed to "
              << static_cast<uint64_t> (msg.new_state());
      (*td_ptr)->set_state(msg.new_state());
      break;
  }
  // This task state change may have caused the job to have schedulable tasks
  // TODO(malte): decide if we should do invoke the scheduler here, or kick off
  // the scheduling iteration from within the earlier handler call into the
  // scheduler
  JobDescriptor* jd = FindOrNull(*job_table_,
                                 JobIDFromString((*td_ptr)->job_id()));
  scheduler_->ScheduleJob(jd);
  // XXX(malte): tear down the respective connection, cleanup
}

#ifdef __HTTP_UI__
void Coordinator::InitHTTPUI() {
  // Start up HTTP interface
  if (FLAGS_http_ui && FLAGS_http_ui_port > 0) {
    // TODO(malte): This is a hack to avoid failure of shared_from_this()
    // because we do not have a shared_ptr to this object yet. Not sure if this
    // is safe, though.... (I think it is, as long as the Coordinator's main()
    // still holds a shared_ptr to the Coordinator).
    //shared_ptr<Coordinator> dummy(this);
    c_http_ui_.reset(new CoordinatorHTTPUI(shared_from_this()));
    c_http_ui_->Init(FLAGS_http_ui_port);
  }
}
#endif

void Coordinator::KillRunningTask(TaskID_t task_id,
                                  TaskKillMessage::TaskKillReason reason) {
  scheduler_->KillRunningTask(task_id, reason);

}

void Coordinator::AddJobsTasksToTables(TaskDescriptor* td, JobID_t job_id) {
  // Set job ID field on task. We do this here since we've only just generated
  // the job ID in the job submission, which passes it in.
  td->set_job_id(to_string(job_id));
  // Insert task into task table
  VLOG(1) << "Adding task " << td->uid() << " to task table.";
  if (!InsertIfNotPresent(task_table_.get(), td->uid(), td)) {
    VLOG(1) << "Task " << td->uid() << " already exists in "
            << "task table, so not adding it again.";
  }
  // Adds its outputs to the object table and generate future references for
  // them.
  for (RepeatedPtrField<ReferenceDescriptor>::iterator output_iter =
       td->mutable_outputs()->begin();
       output_iter != td->mutable_outputs()->end();
       ++output_iter) {
    // First set the producing task field on the task outputs
    output_iter->set_producing_task(td->uid());
    DataObjectID_t output_id(DataObjectIDFromProtobuf(output_iter->id()));
    VLOG(1) << "Considering task output " << output_id << ", "
            << "adding to local object table";
    if (object_store_->AddReference(output_id, &(*output_iter))) {
      VLOG(1) << "Output " << output_id << " already exists in "
              << "local object table. Not adding again.";
    }
    // Check that the object was actually stored
    set<ReferenceInterface*>* refs = object_store_->GetReferences(output_id);
    if (refs && refs->size() > 0)
      VLOG(3) << "Object is indeed in object store";
    else
      VLOG(3) << "Error: Object is not in object store";
  }
  // Process children recursively
  for (RepeatedPtrField<TaskDescriptor>::iterator task_iter =
       td->mutable_spawned()->begin();
       task_iter != td->mutable_spawned()->end();
       ++task_iter) {
    AddJobsTasksToTables(&(*task_iter), job_id);
  }
}

void Coordinator::SendHeartbeatToParent(
    const MachinePerfStatisticsSample& stats) {
  BaseMessage bm;
  // TODO(malte): we do not always need to send the location string; it
  // sufficies to send it if our location changed (which should be rare).
  SUBMSG_WRITE(bm, heartbeat, uuid, to_string(uuid_));
  SUBMSG_WRITE(bm, heartbeat, location, node_uri_);
  SUBMSG_WRITE(bm, heartbeat, capacity,
               topology_manager_->NumProcessingUnits());
  // Include resource usage stats
  bm.mutable_heartbeat()->mutable_load()->CopyFrom(stats);
  VLOG(1) << "Sending heartbeat to parent coordinator!";
  SendMessageToRemote(parent_chan_, &bm);
}


void Coordinator::IssueWebserverJobs() {
  uint64_t seconds;
  uint64_t num_requests = knowledge_base_->GetAndResetWebreqs(seconds);
  VLOG(2) << "Currently seeing " << num_requests << " webrequests per second.";
  // Provision for a potential 15% increase in jobs.
  uint64_t rps = num_requests / seconds + uint64_t(num_requests * 1.15);

  const uint64_t rps_per_job = 10000;
  uint64_t num_jobs = (rps / rps_per_job) + 1;

  // Get the number of active servers
  uint64_t current_num_webservers = haproxy_controller_->GetNumActiveJobs();

  if (num_jobs == current_num_webservers) {
    // Do Nothing.
  } else if (num_jobs > current_num_webservers) {
    // We need more resources add the difference in numbers
    int64_t num_new_jobs = num_jobs - current_num_webservers;
    vector<JobDescriptor *> webserver_jobs;
    haproxy_controller_->GenerateJobs(webserver_jobs, num_new_jobs);
    VLOG(2) << "Starting up " << num_new_jobs << " additional webservers.";
    for (auto job : webserver_jobs) {
      VLOG(2) <<"Job with root task: " << job->root_task().name();
      SubmitJob(*job);
    }
  }
  // Update the number of active servers.
  haproxy_controller_->SetNumActiveJobs(num_jobs);
}

const string Coordinator::SubmitJob(const JobDescriptor& job_descriptor) {
  // //TODO REMOVE ALL THIS energy_stats_history
  // if (first_stupid) {
  //   first_stupid = false;
  // } else {
  //   // Kill the existing tasks.
  //   for (TaskMap_t::const_iterator t_iter = task_table_->begin();
  //        t_iter != task_table_->end();
  //        ++t_iter) {
  //     VLOG(1) << "MANUALLY PREEMPTING " << t_iter->first;
  //     KillRunningTask(t_iter->first, TaskKillMessage::PREEMPTION);

  //   }
  // }


  // Generate a job ID
  // TODO(malte): This should become deterministic, and based on the
  // inputs/outputs somehow, maybe.
  JobID_t new_job_id = GenerateJobID();
  LOG(INFO) << "NEW JOB: " << new_job_id;
  VLOG(2) << "Details:\n" << job_descriptor.DebugString();
  // Clone the submitted JD and add job to local job table
  JobDescriptor* new_jd = new JobDescriptor;
  new_jd->CopyFrom(job_descriptor);
  CHECK(InsertIfNotPresent(job_table_.get(), new_job_id, job_descriptor));
  // The pointer to the JD has now changed, so reassign it
  new_jd = FindOrNull(*job_table_, new_job_id);
  // Clone the JD and update it with some information
  new_jd->set_uuid(to_string(new_job_id));

  // root task

  TaskDescriptor *root_task = new_jd->mutable_root_task();

  // Set the root task ID (which is 0 or unset on submission)
  root_task->set_uid(GenerateRootTaskID(*new_jd));

  // Compute the absolute deadline for the root task if it has a deadline
  // set.
  if (root_task->has_relative_deadline()) {
    root_task->set_absolute_deadline(GetCurrentTimestamp() + root_task->relative_deadline());
  }

  // Create a dynamic task graph for the job
  TaskGraph* new_dtg = new TaskGraph(root_task);
  // Store the task graph
  InsertIfNotPresent(&task_graph_table_, new_job_id, new_dtg);
  // Add itself and its spawned tasks (if any) to the relevant tables:
  // - tasks to the task_table_
  // - inputs/outputs to the object_table_
  // and set the job_id field on every task.
  AddJobsTasksToTables(root_task, new_job_id);
  // Set up job outputs
  for (RepeatedPtrField<string>::const_iterator output_iter =
       new_jd->output_ids().begin();
       output_iter != new_jd->output_ids().end();
       ++output_iter) {
    // The root task must produce all of the non-existent job outputs, so they
    // should all be in the object table now.
    DataObjectID_t output_id(DataObjectIDFromProtobuf(*output_iter));
    VLOG(1) << "Considering job output " << output_id;
    set<ReferenceInterface*>* refs = object_store_->GetReferences(output_id);
    CHECK(refs && refs->size() > 0)
        << "Could not find reference to data object ID " << output_id
        << ", which we just added!";
  }
#ifdef __SIMULATE_SYNTHETIC_DTG__
  LOG(INFO) << "SIMULATION MODE -- generating synthetic task graph!";
  sim_dtg_generator_.reset(new sim::SimpleDTGGenerator(FindOrNull(*job_table_,
                                                                  new_job_id)));
  boost::thread t(boost::bind(&sim::SimpleDTGGenerator::Run,
                              sim_dtg_generator_));
#endif
  // Kick off the scheduler for this job.
  uint64_t num_scheduled = scheduler_->ScheduleJob(
      FindOrNull(*job_table_, new_job_id));
  LOG(INFO) << "Attempted to schedule job " << new_job_id << ", successfully "
            << "scheduled " << num_scheduled << " tasks.";
  // Finally, return the new job's ID
  return to_string(new_job_id);
}

void Coordinator::Shutdown(const string& reason) {
  LOG(INFO) << "Coordinator shutting down; reason: " << reason;
#ifdef __HTTP_UI__
  if (FLAGS_http_ui && c_http_ui_ && c_http_ui_->active())
      c_http_ui_->Shutdown(false);
#endif
  m_adapter_->StopListen();
  VLOG(1) << "All connections shut down; now exiting...";
  // Toggling the exit flag will make the Coordinator drop out of its main loop.
  exit_ = true;
}

/* Storage Engine Interface - The storage engine is notified of the location
 * of new storage engines in one of three ways:
 * 1) if a new resource is added to the coordinator, then all the local
 * storage engines (which share the same coordinator) should be informed.
 * Automatically add this to the list of peers
 * 2) if receive a StorageRegistrationRequest, this signals the desire of a
 * storage engine to be registered with (either) all of the storage engines
 * managed by this coordinator, or its local engine. StorageRegistrationRequests
 * can also arrive directly to the storage engine. The coordinator supports this
 * because outside resources/coordinators may not know the whole topology within
 * the coordinator. I *suspect* this technique will be faster
 * TODO(tach): improve efficiency */
void Coordinator::InformStorageEngineNewResource(ResourceDescriptor* rd_new) {
  VLOG(2) << "Inform Storage Engine of New Resources ";

  /* Create Storage Registration Message */
  BaseMessage base;
  StorageRegistrationMessage* message = new StorageRegistrationMessage();
  message->set_peer(true);
  CHECK_NE(rd_new->storage_engine(), "")
    << "Storage engine URI missing on resource " << rd_new->uuid();
  message->set_storage_interface(rd_new->storage_engine());
  message->set_uuid(rd_new->uuid());

  StorageRegistrationMessage& message_ref = *message;

  vector<ResourceStatus*> rs_vec = associated_resources();

  /* Handle Local Engine First */

  object_store_->HandleStorageRegistrationRequest(message_ref);

  /* Handle other resources' engines*/
  for (vector<ResourceStatus*>::iterator it = rs_vec.begin();
       it != rs_vec.end();
       ++it) {
    ResourceStatus* rs = *it;
    // Only machine-level resources can have a storage engine
    // TODO(malte): is this a correct assumption? We could have per-core
    // storage engines.
    if (rs->descriptor().type() != ResourceDescriptor::RESOURCE_MACHINE)
      continue;
    if (rs->descriptor().storage_engine() != "")
      VLOG(2) << "Resource " << rs->descriptor().uuid() << " does not have a "
              << "storage engine URI set; skipping notification!";
      continue;
    const string& uri = rs->descriptor().storage_engine();
    CHECK_NE(uri, "") << "Missing storage engine URI on RD for "
                      << rs->descriptor().uuid();
    if (!m_adapter_->GetChannelForEndpoint(uri)) {
      StreamSocketsChannel<BaseMessage>* chan =
          new StreamSocketsChannel<BaseMessage > (
              StreamSocketsChannel<BaseMessage>::SS_TCP);
      Coordinator::ConnectToRemote(uri, chan);
    } else {
      m_adapter_->SendMessageToEndpoint(uri, (BaseMessage&)message_ref);
    }
  }
}

/* Wrapper Method - Forward to Object Store. This method is useful
 * for other storage engines which simply talk to the coordinator,
 * letting it to the broadcast to its resources
 */
void Coordinator::HandleStorageRegistrationRequest(
        const StorageRegistrationMessage& msg) {
  if (!object_store_) { // In theory, each node should have an object
                        // store which has already been instantiated. So
                        // this case should never happen.
      VLOG(1) << "No object store detected for this node. Storage Registration"
              << "Message discarded";
  } else {
      object_store_->HandleStorageRegistrationRequest(msg);
  }
}

/* This is a message sent by TaskLib which seeks to discover
 where the storage engine is - this is only important if the
 storage engine is not guaranteed to be local */
void Coordinator::HandleStorageDiscoverRequest(
    const StorageDiscoverMessage& msg) {
  ResourceID_t uuid = ResourceIDFromString(msg.uuid());
  ResourceStatus** rsp = FindOrNull(*associated_resources_, uuid);
  ResourceDescriptor* rd = (*rsp)->mutable_descriptor();
  const string& uri = rd->storage_engine();
  StorageDiscoverMessage* reply = new StorageDiscoverMessage();
  reply->set_uuid(msg.uuid());
  reply->set_uri(msg.uri());
  reply->set_storage_uri(uri);
  /* Send Message*/
}

} // namespace firmament
