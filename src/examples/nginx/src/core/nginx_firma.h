// The Firmament project
// Copyright (c) 2012 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// A "hello world" test job.

#ifndef FIRMAMENT_NGINX
#define FIRMAMENT_NGINX
// TODO(gustafa) eliminate this hack
#define __PLATFORM_HAS_BOOST__ true

#include "/home/gustafa/firmament/src/base/task_interface.h"

namespace firmament {
namespace examples {
namespace nginx {

class HelloWorldTask : public TaskInterface {
 public:
  explicit HelloWorldTask(TaskLib* task_lib, TaskID_t task_id)
    : TaskInterface(task_lib, task_id) {
    VLOG(3) << "Constructing HelloWorldTask";
  }
  void Invoke();
};

}  // namespace nginx
}  // namespace examples
}  // namespace firmament

#endif  // FIRMAMENT_NGINX