#ifndef FIRMAMENT_MISC_WRAPAROUND_VECTOR_H
#define FIRMAMENT_MISC_WRAPAROUND_VECTOR_H

#include <vector>
#include <boost/thread/mutex.hpp>

#include "base/types.h"

namespace firmament {

template<class T>
class WraparoundVector {
 public:
  WraparoundVector(uint64_t max_size, T item);
  ~WraparoundVector();

  void PushElement(T element);
  T GetNthNewest(uint64_t entry);

  T GetDiff(uint64_t first, uint64_t last);


 private:
  uint64_t max_size;
  shared_ptr<vector<T>> inner_vector_;
  boost::mutex access_mutex_;
  uint64_t insert_point_;
};

} // namespace firmament

#endif
