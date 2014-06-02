// The Firmament project
// Copyright (c) 2014 Malte Schwarzkopf <malte.schwarzkopf@cl.cam.ac.uk>
//
// Implementation of the coordinator knowledge base.
// Author: Gustaf Helgesson <gustafa987@gmail.com>

#include "misc/sliding_window.h"

namespace firmament {

SlidingWindow::SlidingWindow(uint64_t size) :
  current_pos_(0),
  size_(size) {
    CHECK(size > 0);
    entries_ = new double[size];
    for (uint64_t i = 0; i < size; ++i) {
      entries_[i] = 0;
    }
}

void SlidingWindow::AddEntry(double val) {
  uint64_t previous_pos = (current_pos_ - 1) % size_;
  entries_[current_pos_] = val;
  average_ += val - entries_[previous_pos];
  current_pos_ = (current_pos_ + 1) % size_;
}

SlidingWindow::~SlidingWindow() {
  delete [] entries_;
}

} // namespace firmament