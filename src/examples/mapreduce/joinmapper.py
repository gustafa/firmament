#!/usr/bin/python
import os
import sys

from heartbeat.heartbeater import Heartbeater

input_filename = os.environ['INPUT_FILE']
completion_filename = os.environ['FLAGS_completion_filename']
tuples_filename = os.environ['FLAGS_tuples_filename']


# Mark mapper to do work between 0 and 50%
#heart_beater = Heartbeater(completion_filename, 0.02, 0.5)

filesize = os.path.getsize(input_filename)

so_far = 0
num_tuples = 0


# Emulating files by new entries.
current_file = 0
current_file_str = '0'

previous_file = 1
previous_file_str = '1'


for line in sys.stdin:
  so_far += len(line)
  line = line.strip()

  if line == 'new_file':
    # Swap files!
    current_file, previous_file = previous_file, current_file
    current_file_str, previous_file_str = previous_file_str, current_file_str
    current_file_str = str(current_file)
  else:
    # Presumes the first value is a key and the rest are values.
    entries = line.split()
    key = entries[0]
    values = current_file_str + '_' + '_'.join(entries[1:])

    print '%s\t%s' % (key, values)

    num_tuples += 1

  # Update the stats to how much of the file we have read.
#  heart_beater.set_completed(so_far / float(filesize))

# Mark us as finished.
#heart_beater.mark_done()

# Pass number of tuples for reducer to calculate its output.
tuples_file = open(tuples_filename, 'w')
tuples_file.write(str(num_tuples))
tuples_file.close()