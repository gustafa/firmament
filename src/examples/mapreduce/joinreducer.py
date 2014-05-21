#!/usr/bin/python
import os
import sys

from heartbeat.heartbeater import Heartbeater


def print_map(output_map):
  # Time to compute the join if still existing!
  num_files = len(output_map)

  join_entries = []
  for i in range(num_files):
    join_entries.append(output_map[i])
    #print entries

  if num_files == 1:
    for elem in join_entries[0]:
       print elem
  else:
    for i in range(len(join_entries[0])):
      first_val = current_key + '\t' + join_entries[0][i] + ' '
      for j in range(len(join_entries[1])):
        print first_val + join_entries[1][j]


completion_filename = os.environ['FLAGS_completion_filename']
tuples_filename = os.environ['FLAGS_tuples_filename']

num_tuples = 1

first = True

#heart_beater = Heartbeater(completion_filename, 0.5, 1)

so_far = 0

current_key = ''
current_map = {}

# Assumes we want to join entries from each file in order

for line in sys.stdin:
  if first:
    tuples_file = open(tuples_filename, 'r')
    num_tuples = float(tuples_file.read())
    tuples_file.close()
    first = False

  entries = line.split()

  # key \t file_val1_val2_val3
  key = entries[0]
  if key != current_key:
    # Time to compute the join if still existing!
    if current_map:
      print_map(current_map)

    current_key = key
    current_map = {}

  values = entries[1].split('_')
  file_num = int(values[0])
  values = ' '.join(values[1:])

  if file_num in current_map:
    current_map[file_num].append(values)
  else:
    current_map[file_num] = [values]

  so_far += 1

if current_map:
  print_map(current_map)

  #heart_beater.set_completed(so_far / num_tuples)

#heart_beater.mark_done()
