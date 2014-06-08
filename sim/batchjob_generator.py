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



                #  wc join mv
job_fractions = [0.3, 0.7, 1.0]
job_idx_to_type = {0: 'wc', 1: 'join', 2: 'mv'}

job_type_to_binary = {'wc': 'firmareduce', 'join': 'firmareduce', 'mv': 'filetransfer' }

directory = '/home/gjrh2/firmament/src/examples/'

job_type_to_args = {'wc': ["%s%s%s" % (directory, "mapreduce/", name) for name in ['wcmapper.py', 'wcreducer.py']], \
                    'join': ["%s%s%s" % (directory, "mapreduce/", name) for name in ['joinmapper.py', 'joinreducer.py']], \
                    'mv': ["/home/gjrh2/firmament/build/examples/batchjobs" for i in range(2)] #"%s%s%s" %(directory, " bactjobs/", 'filetransfer') ]
                    }

input_size_seconds = {'wc': [(1, 41.560000), (2, 84.740000), (3, 125.560000), (4, 163.600000), (6, 248.270000), (7, 290.850000), (10, 401.670000), (12, 479.280000)],
                      'join': [(1, 48.940000), (2, 99.850000), (3, 159.640000), (4, 219.070000), (6, 365.990000), (7, 432.600000), (10, 741.300000), (12, 751.060000)],
                      'mv': [(1, 3), (10, 10000)]} # TODO mv needs real values!

shortest_job_s = 60
longest_job_s = 600
average_active_jobs = 10


def get_input_size_runtime(job_type, wanted_rt):
  data = input_size_seconds[job_type][:]

  left_idx = 0
  right_idx = 0
  for i in range(len(data)):
    (size, seconds) = data[i]
    if seconds >= wanted_rt:
      left_idx = i - 1
      right_idx = i

  take_left = abs(data[left_idx][2] - wanted_rt) < abs(data[right_idx][2] - wanted_rt)

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
    runtime = shortest_job_s * random.randint(1, (longest_job_s / shortest_job_s) + 1)
  job_idx = get_index(random.random(), job_fractions)
  job_type = job_idx_to_type[job_idx]
  (units, runtime) = get_input_size_runtime(job_type)

  deadline_idx = get_index(random.random(), deadline_fractions)
  deadline_type = deadline_idx_to_type[deadline_idx]
  deadline = runtime * deadline_multipliers[deadline_type]



  if deadline > max_time:
    if runtime * 2 + current_time < max_time:
      deadline = max_time
    else:
      # We tried, we failed don't add.
      if force_job_s:
        return None
      else:
        return generate_job(current_time, max_time, shortest_job_s)

  # Hard copy this stuff.
  args = job_type_to_args[job_type][:]
  # Add input file
  args.append(directory + job_type + str(runtime)) ###EEH WHAT IS GOING ON HERE?

  job = {'runtime': runtime, 'deadline': deadline, 'job_type': job_type, 'deadline_type': deadline_type, \
         'binary': job_type_to_binary[job_type], 'args': args, 'units': units}
  return job

def main():

  if len(sys.argv) != 2:
    print 'usage batchjob_generator.py <experiment time>'
    sys.exit(1)

  runtime_in_s = int(sys.argv[1])
  average_job_s = (shortest_job_s + longest_job_s) / float(2)
  interval_s = average_job_s / average_active_jobs
  num_jobs = int(runtime_in_s / interval_s)
  current_time = 1
  jobs = []

  for i in range(num_jobs):
    job = generate_job(current_time, runtime_in_s)
    current_time += interval_s
    if job:
      jobs.append(job)

  job_dict = {'runtime': runtime_in_s, 'interval_s': interval_s, 'jobs': jobs}

  with file(out_filename, 'w') as outfile:
    json.dump(job_dict, outfile)
  print 'Generated %d jobs ' % len(jobs)

if __name__ == '__main__':
  main()
