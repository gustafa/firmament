#!/usr/bin/python
import os
import sys

from heartbeat.heartbeater import Heartbeater

completion_filename = os.environ['FLAGS_completion_filename']
tuples_filename = os.environ['FLAGS_tuples_filename']

current_count = 0
current_word = ""

num_tuples = 1


first = True

heart_beater = Heartbeater(completion_filename, 0.5, 1)

so_far = 0

for line in sys.stdin:
  if first:
    tuples_file = open(tuples_filename, 'r')
    num_tuples = float(tuples_file.read())
    tuples_file.close()
    first = False

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

    so_far += 1

  heart_beater.set_completed(so_far / num_tuples)

# Output last word.
if current_word:
  print '%s\t%d' % (current_word, current_count)

heart_beater.mark_done()