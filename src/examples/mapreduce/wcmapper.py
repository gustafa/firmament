#!/usr/bin/python
import os
import sys

from heartbeat.heartbeater import Heartbeater

input_filename = os.environ['INPUT_FILE']
completion_filename = os.environ['FLAGS_completion_filename']
tuples_filename = os.environ['FLAGS_tuples_filename']


# Mark mapper to do work between 0 and 50%
heart_beater = Heartbeater(completion_filename, 0.02, 0.5)

filesize = os.path.getsize(input_filename)

so_far = 0
num_tuples = 0

for line in sys.stdin:
  so_far += len(line)
  line = line.strip()
  words = line.split()
  for word in words:
    print '%s\t%s' % (word, 1)

  num_tuples += len(words)

  # Update the stats to how much of the file we have read.
  heart_beater.set_completed(so_far / float(filesize))

# Mark us as finished.
heart_beater.mark_done()

# Pass number of tuples for reducer to calculate its output.
tuples_file = open(tuples_filename, 'w')
tuples_file.write(str(num_tuples))
tuples_file.close()