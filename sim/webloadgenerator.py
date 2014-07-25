#!/usr/bin/python2
import json
from time import sleep

from subprocess import call, Popen

import weightbenchmarker
import os
import signal
import sys


def sleep_for(ms):
  sleep(ms / 1000)



def main():
  interface = 'p4p1.2'
  with open('sim/webload') as inputdata:
    data = json.load(inputdata)

  attack_host = '10.10.0.10'

  wait_time = data['wait_time']
  runtime_per_sample = data['runtime_per_sample']
  mbps_per_sample = data['mbps_per_sample']

  num_samples = len(mbps_per_sample)
  sleep_for(wait_time)

  base_test = 'sudo -u flowuser weighttp -k '
  connections = weightbenchmarker.get_num_connections(400)
  num_threads = weightbenchmarker.get_num_threads(connections)
  connections = weightbenchmarker.alter_num_connections(connections, num_threads)
  test_command = '%(base_command)s -n 10000000000 -c %(connections)d -t %(threads)d %(hostname)s/' % \
       {'base_command': base_test, 'connections': connections, \
                'threads': num_threads, 'hostname': attack_host}
  p = Popen(test_command, shell=True, preexec_fn=os.setsid)
  for i in range(num_samples):
    mbps = mbps_per_sample[i]
    print "running with " + str(mbps)
    weightbenchmarker.set_mbps(mbps, interface)

    # Launch web requests and sleep for the intended duration
    sleep_for(runtime_per_sample)
  os.killpg(p.pid, signal.SIGTERM)
  call("killall weighttp", shell=True)


if __name__ == '__main__':
  main()



