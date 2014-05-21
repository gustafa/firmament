#ifndef FIRMAMENT_MISC_WRAPAROUND_VECTOR_INL_H
#define FIRMAMENT_MISC_WRAPAROUND_VECTOR_INL_H


#include "wraparound_vector.h"

namespace firmament {

template<class T>
WraparoundVector<T>::WraparoundVector(uint64_t max_size) :
  max_size(max_size), inner_vector_(new vector<T>(max_size)) {
}

template<class T>
WraparoundVector<T>::~WraparoundVector() {
  // Stub
}

template<class T>
void WraparoundVector<T>::PushElement(T element) {
  scoped_lock(access_mutex_);
  (*inner_vector_)[insert_point_] = element;
  insert_point_ = (insert_point_ + 1) % max_size;
}

template<class T>
// 0 Give newest
T WraparoundVector<T>::GetNthNewest(uint64_t entry) {
  scoped_lock(access_mutex_);
  return (*inner_vector_)[(insert_point_ - entry -1) % max_size];
}

} // namespace firmament

#endif