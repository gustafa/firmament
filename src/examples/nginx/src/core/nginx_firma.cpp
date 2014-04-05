extern "C" {
  #include "nginx.h"
}

#include "nginx_firma.h"

// int main(int argc, char *const *argv)
// {
//   //google::ParseCommandLineFlags(&argc, &argv, true);

//   return mainy(argc, argv);
// }

namespace firmament {

void task_main(TaskLib* task_lib, TaskID_t task_id,
               vector<char*>*) {
  LOG(INFO) << "Called task_main, starting ";
  VLOG(3) << "Called task main";
  examples::nginx::HelloWorldTask t(task_lib, task_id);
  t.Invoke();
}

namespace examples {
namespace nginx {

void HelloWorldTask::Invoke() {
  LOG(INFO) << "Hello world (log)!";
  std::cout << "Hello world (stdout)!\n";
  std::cerr << "Hello world (stderr)!\n";
}

}  // namespace nginx
}  // namespace examples
}  // namespace firmament
