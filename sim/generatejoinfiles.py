#!/usr/bin/python2
import random

min_int = 0
max_int = 100000


def generate_join_ints(ints):
  for i in range(ints):
    print '%d %d' % (random.randint(min_int, max_int), random.randint(min_int, max_int))

ints_per_file = 1000000

generate_join_ints(ints_per_file)
print 'new_file'
generate_join_ints(ints_per_file)
