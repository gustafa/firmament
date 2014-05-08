#!/usr/bin/python
import os
import sys

from heartbeat.heartbeater import Heartbeater
completion_filename = os.environ['FLAGS_completion_filename']


current_count = 0
current_word = ""

heart_beater = Heartbeater(completion_filename, 0.5, 1)


for line in sys.stdin:
  word_count = line.split()

  if len(word_count) == 2:
    (word, count) = word_count
    count = int(count)

    if word == current_word:
      current_count += count
    else:
      # Emit output
      print '%s\t%d' % (current_word, current_count)
      current_word = word
      current_count = count

heart_beater.mark_done()