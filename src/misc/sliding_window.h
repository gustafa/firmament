// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Implementation of the coordinator knowledge base.
// Author: Gustaf Helgesson <gustafa987@gmail.com>
#include "base/types.h"

namespace firmament {
class SlidingWindow {

 public:
  SlidingWindow(uint64_t size);
  void AddEntry(double val);
  inline double GetAverage() {return average_; }
  ~SlidingWindow();


 private:
  uint64_t current_pos_;
  uint64_t size_;
  double average_;
  double *entries_;
};

} // namespace firmament