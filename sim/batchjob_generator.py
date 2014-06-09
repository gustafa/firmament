#!/usr/bin/python2
import json
import numpy as np
import random
import sys

#deadline_fractions['short': 0.1, 'medium': 0.2, 'long': 0.5, 'anytime': 0.2]
                  # shrm med, lng, any
deadline_fractions = [0.1, 0.3, 0.8, 1.0]
deadline_idx_to_type = {0: 'short', 1: 'medium', 2: 'long', 3: 'anytime'}
deadline_multipliers = {'short': 1.2, 'medium': 3, 'long': 15, 'anytime': 10000}

job_type_to_inputrange = {'wc': (1,12), 'mv': (20, 250), 'join': (1,10)}

                #  wc join mv
job_fractions = (0.45, 0.90, 1.0)
job_idx_to_type = {0: 'wc', 1: 'join', 2: 'mv'}

job_type_to_binary = {'wc': 'firmareduce', 'join': 'firmareduce', 'mv': 'filetransfer' }

directory = '/home/gjrh2/firmament/src/examples/'
mv_dir = '/home/gjrh2/firmament/build/examples/batchjobs'
mapreduce_dir = directory + "mapreduce/"
job_type_to_args = {'wc': (mapreduce_dir +  'wcmapper.py', mapreduce_dir + 'wcreducer.py', '/home/gjrh2/firmament/sim/wcfiles'), \
                    'join': (mapreduce_dir + "joinmapper.py", mapreduce_dir + 'joinreducer.py', '/home/gjrh2/firmament/sim/joinfiles'), \
                    'mv': (mv_dir, mv_dir) #"%s%s%s" %(directory, " bactjobs/", 'filetransfer') ]
                    }

input_size_seconds = {'wc': ((1, 41.560000), (2, 84.740000), (3, 125.560000), (4, 163.600000), (6, 248.270000), (7, 290.850000), (10, 401.670000), (12, 479.280000)),
                      'join': ((1, 48.940000), (2, 99.850000), (3, 159.640000), (4, 219.070000), (6, 365.990000), (7, 432.600000), (10, 741.300000), (12, 751.060000)),
                      'mv': ((10, 37.940000), (20, 73.080000), (50, 123.740000), (100, 266.990000), (150, 452.980000), (250, 696.060000), (400, 1160.330000), (600, 1720.590000))} # TODO mv needs real values!

job_type_to_input_dir = {
  'wc':
  'mv'
}

shortest_job_s = 30
longest_job_s = 180
average_active_jobs = 10


def get_input_size_runtime(job_type, wanted_rt):
  print 'Wanted runtime ' + str(wanted_rt)
  data = input_size_seconds[job_type][:]

  left_idx = 0
  right_idx = 0
  for i in range(len(data)):
    (size, seconds) = data[i]
    if seconds < wanted_rt:
      left_idx = i - 1
      right_idx = i
  if left_idx < 0:
    left_idx = 0
  print 'left idx %d' % left_idx

  take_left = abs(data[left_idx][1] - wanted_rt) < abs(data[right_idx][1] - wanted_rt)

  if take_left:
    return data[left_idx]
  else:
    return data[right_idx]

out_filename = 'sim/batchload'

def get_index(ltval, lst):
  assert(len(lst) > 0)
  for i in range(len(lst)):
    if ltval <= lst[i]:
      return i
  return len(lst) - 1

def generate_job(current_time, max_time, force_job_s=False):
  if force_job_s:
    runtime = force_job_s
  else:
    runtime = random.randint(shortest_job_s, longest_job_s)
  job_idx = get_index(random.random(), job_fractions)
  job_type = job_idx_to_type[job_idx]
  (units, runtime) = get_input_size_runtime(job_type, runtime)
  print (units, runtime)

  deadline_idx = get_index(random.random(), deadline_fractions)
  deadline_type = deadline_idx_to_type[deadline_idx]
  deadline = runtime * deadline_multipliers[deadline_type]



  # Hard copy this stuff.
  args = job_type_to_args[job_type][:] + (units,)
  # Add input file


  job = {'runtime': runtime, 'deadline': int(deadline), 'job_type': job_type, 'deadline_type': deadline_type, \
         'binary': job_type_to_binary[job_type], 'args': args, 'units': units}
  return job

def main():

  if len(sys.argv) != 2:
    print 'usage batchjob_generator.py <experiment time>'
    sys.exit(1)
  gamma = 1.8 # multiplier of real expected runtime.
  runtime_in_s = int(sys.argv[1])
  max_runtime = int(runtime_in_s * 1.3)
  average_job_s = ((shortest_job_s + longest_job_s) / float(2)) * gamma
  interval_s = average_job_s / average_active_jobs
  num_jobs = int(runtime_in_s / interval_s)
  current_time = 1
  jobs = []

  print num_jobs
  for i in range(num_jobs):


    job = generate_job(current_time, max_runtime)
    current_time += interval_s
    if job:
      jobs.append(job)

  job_dict = {'runtime': runtime_in_s, 'interval_s': interval_s, 'jobs': jobs}

  with file(out_filename, 'w') as outfile:
    json.dump(job_dict, outfile)
  print 'Generated %d jobs ' % len(jobs)

if __name__ == '__main__':
  main()
