from base import job_desc_pb2
from base import task_desc_pb2
from base import reference_desc_pb2
from google.protobuf import text_format
import httplib, urllib, re, sys, random
import binascii


def generateHexString(length):
  return '%030x' % random.randrange(16**length)


if len(sys.argv) < 4:
  print "usage: job_submit.py <coordinator hostname> <web UI port> " \
      "<task name> <task binary>"
  sys.exit(1)

hostname = sys.argv[1]
port = int(sys.argv[2])
task_name = sys.argv[3]
binary = sys.argv[4]

job_desc = job_desc_pb2.JobDescriptor()

job_desc.uuid = "" # UUID will be set automatically on submission
job_desc.name = "testjob"
job_desc.root_task.uid = 0
job_desc.root_task.name = task_name
job_desc.root_task.state = task_desc_pb2.TaskDescriptor.CREATED
job_desc.root_task.binary =binary

#job_desc.root_task.args.append("--v=2")
#job_desc.root_task.args.append("0")
#job_desc.root_task.args.append("100000")
#root_input1 = job_desc.root_task.dependencies.add()
#root_input1.id = 123456789
#root_input1.type = reference_desc_pb2.ReferenceDescriptor.FUTURE

# Use the user supplied random identifier if provided or generate
# a new, random one otherwise.
# if len(sys.argv) == 5:
#   input_id = binascii.unhexlify(sys.argv[4])

input_id = binascii.unhexlify(generateHexString(64))


# Add the remaining as job arguments!
if len(sys.argv) > 5:
  for arg in sys.argv[5:]:
    job_desc.root_task.args.append(arg)

#input_id = binascii.unhexlify('feedcafedeadbeeffeedcafedeadbeeffeedcafedeadbeeffeedcafedeadbeef') #sys.argv[4])
output_id = binascii.unhexlify('db33daba280d8e68eea6e490723b02cedb33daba280d8e68eea6e490723b02ce')
output2_id = binascii.unhexlify('feedcafedeadbeeffeedcafedeadbeeffeedcafedeadbeeffeedcafedeadbeef')
job_desc.output_ids.append(output_id)
job_desc.output_ids.append(output2_id)
#job_desc.root_task.binary = "/bin/echo"
#job_desc.root_task.args.append("Hello World!")

input_desc = job_desc.root_task.dependencies.add()
input_desc.id = input_id
input_desc.scope = reference_desc_pb2.ReferenceDescriptor.PUBLIC
input_desc.type = reference_desc_pb2.ReferenceDescriptor.CONCRETE
input_desc.non_deterministic = False
input_desc.location = "blob:/tmp/fib_in"
final_output_desc = job_desc.root_task.outputs.add()
final_output_desc.id = output_id
final_output_desc.scope = reference_desc_pb2.ReferenceDescriptor.PUBLIC
final_output_desc.type = reference_desc_pb2.ReferenceDescriptor.FUTURE
final_output_desc.non_deterministic = False
final_output_desc.location = "blob:/tmp/out1"
final_output2_desc = job_desc.root_task.outputs.add()
final_output2_desc.id = output2_id
final_output2_desc.scope = reference_desc_pb2.ReferenceDescriptor.PUBLIC
final_output2_desc.type = reference_desc_pb2.ReferenceDescriptor.FUTURE
final_output2_desc.non_deterministic = False
final_output2_desc.location = "blob:/tmp/out2"


#params = urllib.urlencode({'test': text_format.MessageToString(job_desc)})
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
  print "JOB SUBMITTED successfully!\nJOB ID is %s\nStatus page: " \
      "http://%s:%d/job/status/?id=%s" % (job_id, hostname, port, job_id)
else:
  print "ERROR submitting job -- response was: %s (Code %d)" % (response.reason,
                                                                response.status)
print "----------------------------------------------"
conn.close()
