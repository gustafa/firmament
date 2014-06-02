#!/usr/bin/python2
import sys
import json

def generate_rps(mean, fraction):
  max_web_rps = 235000
  return max_web_rps * mean * fraction

web_means = [0.40, 0.45, 0.52 ,0.58, 0.65, 0.71, 0.78, 0.80, 0.86, 0.9 , 0.91 , 0.92, 0.84 , 0.9 , 0.85, 0.85, 0.82, 0.79, 0.75, 0.75, 0.74, 0.76, 0.77, 0.78 , 0.77 , 0.78 , 0.75, 0.70, 0.68, 0.62, 0.60, 0.52, 0.42, 0.38, 0.32, 0.23, 0.20, 0.15, 0.12, 0.10 , 0.10, 0.11, 0.12, 0.15, 0.20, 0.28, 0.36, 0.42]
rps_per_mbps = 500
num_entries = 48

def generate_rps_list(fraction):
  return [generate_rps(mean, fraction) for mean in web_means]


def rps_to_mbps(rps):
  return rps / rps_per_mbps

def main():

  if len(sys.argv) != 3:
    print 'Usage: ./webrequest_generator.py <num loadbalancers> <runtime in s>'
    sys.exit(1)

  num_load_balancers = int(sys.argv[1])

  runtime = int(sys.argv[2])
  ms_runtime = runtime * 1000

  # 10 second wait time
  wait_time = 1000 * 10
  runtime_per_sample = ms_runtime / num_entries

  runtimes = [runtime_per_sample * i for i in range(num_entries)]
  rpss =  generate_rps_list(0.9)
  mbpss = [rps_to_mbps(rps) for rps in rpss]



  output_dict = {}
  output_dict['runtime_per_sample'] = runtime_per_sample
  output_dict['num_load_balancers'] = num_load_balancers
  output_dict['mbps_per_sample'] = [mbps / num_load_balancers for mbps in mbpss]
  output_dict['wait_time'] = wait_time

  with open('sim/webload', 'w') as outfile:
    json.dump(output_dict, outfile)

if __name__ == '__main__':
  main()