#ifndef FIRMAMENT_MISC_WRAPAROUND_VECTOR_INL_H
#define FIRMAMENT_MISC_WRAPAROUND_VECTOR_INL_H


#include "wraparound_vector.h"
//#include boost/interprocess/sync/scoped_lock.hpp


namespace firmament {

template<class T>
WraparoundVector<T>::WraparoundVector(uint64_t max_size) :
  max_size(max_size), inner_vector_(new vector<T>(max_size)) {
}

template<class T>
WraparoundVector<T>::~WraparoundVector() {
}

template<class T>
void WraparoundVector<T>::PushElement(T element) {
  boost::mutex::scoped_lock lock(access_mutex_);
  (*inner_vector_)[insert_point_] = element;
  insert_point_ = (insert_point_ + 1) % max_size;
}

template<class T>
// 0 Gives newest
T WraparoundVector<T>::GetNthNewest(uint64_t entry) {
  boost::mutex::scoped_lock lock(access_mutex_);
  return (*inner_vector_)[(insert_point_ - entry -1) % max_size];
}

template<class T>
// 0 Give newest
T WraparoundVector<T>::GetDiff(uint64_t first, uint64_t last) {
  boost::mutex::scoped_lock lock(access_mutex_);
  uint64_t first_idx = (insert_point_ - first - 1) % max_size;
  uint64_t last_idx = (insert_point_ - last - 1) % max_size;
  return (*inner_vector_)[last_idx] - (*inner_vector_)[first_idx];
}


} // namespace firmament

#endif