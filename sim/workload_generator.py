#!/usr/bin/python2




class MapReduceApplication(object):

  def __init__(self, directory, mapper, reducer):
    self.mapper = mapper
    self.reducer = reducer
    self.command = 'firmareduce %(dir)s/%(mapper)s %(dir)s%(reducer)s ' % \
                   {'dir': directory, 'mapper':mapper, 'reducer': reducer}

  def output_line(self):
    pass


max_load = 0.9
        # CREATED FROM GOOGLE GRAPH AT 30 MINUTE INTERVALS, DO NOT TOUCH DIRECTLY!
assert(len(web_means) == 48)


# Fraction of power dedicated to webrequests.
web_req_frac = 0.80
batch_frac = 1 - web_req_frac

DIR = '/home/gjrh2/firmament/src/examples'
MAP_REDUCE_APPS = [('wcmapper.py', 'wcreducer.py'), ('joinmapper.py', 'joinreducer.py')]
MISC_APPS = ['filetransfer']

def main():
  pass


if __name__ == '__main__':
  main()