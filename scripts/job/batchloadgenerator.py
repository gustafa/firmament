from base import job_desc_pb2
from base import task_desc_pb2
from base import reference_desc_pb2
from google.protobuf import text_format

import httplib
import json
import random
import re
import sys
import urllib

from time import sleep

DRY_RUN = False

job_types = ['wc', 'join', 'mv']
type_to_current_num = {}
for job_type in job_types:
  type_to_current_num[job_type] = 0

type_to_td_type = {'wc': task_desc_pb2.TaskDescriptor.MAPREDUCE_WC, \
                   'join': task_desc_pb2.TaskDescriptor.MAPREDUCE_JOIN, \
                   'mv': task_desc_pb2.TaskDescriptor.FILETRANSFER }


def get_and_increment_jobnum(job_type):
  curr = type_to_current_num[job_type]
  type_to_current_num[job_type] = curr + 1
  return curr


def generate_job_desc(job_dict):
  job_type = str(job_dict['job_type'])
  # Which number of this job is this?
  job_num = get_and_increment_jobnum(job_type)
  job_desc = job_desc_pb2.JobDescriptor()


  job_desc.uuid = "" # UUID will be set automatically on submission
  job_desc.name = job_type + str(job_num)
  job_desc.root_task.uid = random.randint(0, 60000);
  job_desc.root_task.name = job_type + str(job_num)
  job_desc.root_task.state = task_desc_pb2.TaskDescriptor.CREATED
  job_desc.root_task.binary = str(job_dict['binary'])
  job_desc.root_task.task_type = type_to_td_type[job_type]

  for arg in job_dict['args']:
    job_desc.root_task.args.append(str(arg))

  input_desc = job_desc.root_task.dependencies.add()
  input_desc.scope = reference_desc_pb2.ReferenceDescriptor.PUBLIC
  input_desc.type = reference_desc_pb2.ReferenceDescriptor.CONCRETE
  input_desc.non_deterministic = False
  input_desc.location = "blob:/tmp/fib_in"
  final_output_desc = job_desc.root_task.outputs.add()
  final_output_desc.scope = reference_desc_pb2.ReferenceDescriptor.PUBLIC
  final_output_desc.type = reference_desc_pb2.ReferenceDescriptor.FUTURE
  final_output_desc.non_deterministic = False
  final_output_desc.location = "blob:/tmp/out1"
  final_output2_desc = job_desc.root_task.outputs.add()
  final_output2_desc.scope = reference_desc_pb2.ReferenceDescriptor.PUBLIC
  final_output2_desc.type = reference_desc_pb2.ReferenceDescriptor.FUTURE
  final_output2_desc.non_deterministic = False
  final_output2_desc.location = "blob:/tmp/out2"

  return job_desc

def submit_proto(hostname, port, job_desc):
  params = urllib.urlencode({'test': text_format.MessageToString(job_desc)})
  params = 'test=%s' % text_format.MessageToString(job_desc)
  print "SUBMITTING job with parameters:"
  print params
  print ""

  try:
    headers = {"Content-type": "application/x-www-form-urlencoded"}
    conn = httplib.HTTPConnection("%s:%s" % (hostname, port))
    conn.request("POST", "/job/submit/", params, headers)
    response = conn.getresponse()
  except Exception as e:
    print "ERROR connecting to coordinator: %s" % (e)
    sys.exit(1)

  data = response.read()
  match = re.search(r"([0-9a-f\-]+)", data, re.MULTILINE | re.S | re.I | re.U)
  print "----------------------------------------------"
  if match and response.status == 200:
    job_id = match.group(1)
    print "JOB SUBMITTED successfully!\nJOB ID is %s\nStatus page: " % job_id
  else:
    print "ERROR submitting job -- response was: %s (Code %d)" % (response.reason,
                                                                  response.status)
  print "----------------------------------------------"
  conn.close()


if len(sys.argv) != 4:
  print "usage: batchloadgenerator <coordinator hostname> <web UI port> " \
      "<batchjobfile>"
  sys.exit(1)

hostname = sys.argv[1]
port = sys.argv[2]
job_filename = sys.argv[3]


with file(job_filename, 'r') as input_file:
  data = json.load(input_file)

jobs = data['jobs']
interval_s = data['interval_s']
runtime = data['runtime']

# Generate protobufs in advance
job_descs = [generate_job_desc(job) for job in jobs]

for job_desc in job_descs:
  if DRY_RUN:
    print 'desc: %s' % text_format.MessageToString(job_desc)
  else:
    submit_proto(hostname, port, job_desc)
    sleep(interval_s)
