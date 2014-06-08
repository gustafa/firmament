#!/usr/bin/python2
import os
import sys

from datetime import datetime
from math import ceil
from subprocess import call, Popen
from time import sleep

TC = '/sbin/tc'
IF = 'em1'
POWER_SCRIPT = 'power_script'
DRY_RUN = False
ENERGY_PID = None
MEASUREMENT_KIT_IP='10.11.12.50'

def get_num_connections(mbps):
  if mbps < 2:
    return 2
  elif mbps < 8:
    return 10
  elif mbps < 20:
    return 30
  elif mbps < 40:
    return 40
  elif mbps < 60:
    return 50
  elif mbps < 100:
    return 70
  elif mbps < 150:
    return 100
  elif mbps < 200:
    return 150
  elif mbps < 250:
    return 200
  else:
    return 250

def get_num_threads(num_connections):
  if num_connections < 12:
    return ceil(num_connections / float(2))
  elif num_connections < 23:
    return num_connections
  else:
    return 23

def alter_num_connections(num_connections, num_threads):
  # Effectively a ceilingish mod
  if num_connections % num_threads:
    return num_connections + num_threads - (num_connections % num_threads)
  else:
    return num_connections

def get_num_requests(mbps, seconds):
  if mbps > 1000:
    mbps_cons = 1000
  else:
    mbps_cons = mbps
  rate_per_mbps = 500
  requests_per_s = rate_per_mbps * mbps_cons
  return requests_per_s * seconds

def set_mbps(mbps, IF):
  global DRY_RUN
  commands = ['%(tc)s qdisc del dev %(if)s root' % {'tc': TC, 'if': IF}, '%(tc)s qdisc add dev %(if)s root handle 1: htb default 30' % {'tc': TC, 'if': IF}, \
              '%(tc)s class add dev %(if)s parent 1: classid 1:1 htb rate %(mbps)smbit' % {'tc': TC, 'if': IF, 'mbps': str(mbps)},
              '%(tc)s filter add dev %(if)s protocol ip parent 1: prio 1 handle 43 fw flowid 1:1' % {'tc': TC, 'if': IF}]

  for command in commands:
    if DRY_RUN:
      print command
    else:
      call(command, shell=True)
      sleep(0.4)

def start_energy_measurement(hostname, energy_filename):
  global ENERGY_PID
  global MEASUREMENT_KIT_IP
  global POWER_SCRIPT

  # Store all the information in the power script.
  f = open(POWER_SCRIPT, 'w')
  f.write('powercapture ' + hostname + '\n')
  f.close()
  command = 'sudo -u gjrh2 ssh -i /home/gjrh2/.ssh/id_rsa -t -t %(kit_ip)s < power_script > %(filename)s' % \
      {'kit_ip': MEASUREMENT_KIT_IP, 'filename': energy_filename}
  if DRY_RUN:
    print command
  else:
    ENERGY_PID = Popen(command, shell=True).pid



def stop_energy_measurement():
  global ENERGY_PID
  global MEASUREMENT_KIT_IP
  command = 'sudo -u gjrh2 ssh %(kit_ip)s killall powerlogger' % {'kit_ip': MEASUREMENT_KIT_IP}
  if DRY_RUN:
    print command
  else:
    Popen(command, shell=True)

def main():
  global DRY_RUN

  if os.geteuid() != 0 and not DRY_RUN:
    print 'The weight benchmarker must be run as root as it limits bandwidth for system users.'
    sys.exit(1)


  hostnames = ['titanic'] #'pandaboard', 'pandaboard', 'michael', 'uriel']  # 'pandaboard']# ,'titanic']
  mbpss = [0.2, 0.5, 1, 2, 4, 8, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80, 90, 100, 150, 200, 250, 300, 400, 500, 600, 700, 800, 900, 1000, 2000]
  base_test = 'sudo -u flowuser weighttp -k '
  base_filename = 'test_nginxbw_%(date)s_%(hostname)s_%(mbps)smbit'
  num_runs = 1
  seconds = 100
  between_tests = 40
  warm_up_time = 20
  for i in range(num_runs):
    for hostname in hostnames:
      for mbps in mbpss:
        if hostname != 'titanic' or mbps < 300:
          continue
        # Limit the flowuser's bandwidth to the specified mbps.
        set_mbps(mbps, IF)
        num_connections = get_num_connections(mbps)
        num_threads = get_num_threads(num_connections)
        num_connections = alter_num_connections(num_connections,num_threads)
        num_requests = get_num_requests(mbps, seconds)

        if hostname == 'pandaboard' and num_requests > 1000000:
          num_requests = 1000000
        if hostname == 'titanic' and num_requests > 5000000:
          num_requests = 5000000

        test_command = '%(base_command)s -n %(requests)d -c %(connections)d -t %(threads)d %(hostname)s/' % \
               {'base_command': base_test, 'requests': num_requests, 'connections': num_connections,
                'threads': num_threads, 'hostname': hostname}
        date_str = datetime.now().strftime('%Y-%m-%d_%H:%M:%S')
        current_base = base_filename % {'date' : date_str, 'hostname': hostname, 'mbps': str(mbps)}

        stats_filename = current_base + '_stats'
        power_filename = current_base + '_power'
        test_command += ' > ' + stats_filename
        #'test_nginxbw_%(date)s_%(hostname)s_%(mbps)d'
        if not DRY_RUN:
          process = Popen(test_command, shell=True)
          print 'Running in background'
          # Let the program stress the target host.
          sleep(warm_up_time)
        else:
          print 'Command %smbps: %s' % (mbps, test_command)
          print '.........Sleeping warmup time.........'
        # Start energy measurement
        start_energy_measurement(hostname, power_filename)
        if not DRY_RUN:
          # Wait til the program has finished.
          if process.wait():
            print 'QUITTING DID NOT EXIT CLEANLY!'
            stop_energy_measurement()
            sys.exit(1)
        else:
          print '.........Sleeping run time.........'
        stop_energy_measurement()

        if not DRY_RUN:
          # Get the system some rest til the next execution.
          sleep(between_tests)
        else:
          print '.........Sleeping between tests.........'


if __name__ == '__main__':
  main()
