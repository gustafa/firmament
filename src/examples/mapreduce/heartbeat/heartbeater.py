

class Heartbeater(object):
  # TODO implement complete heartbeating.
  # For now just updates a file read by the C++ version.
  def __init__(self, completed_filename, min_completed=0.0, max_completed=1.0):
    self.min_completed = min_completed
    self.max_completed = max_completed
    self.delta_completed = max_completed - min_completed

    self.f = open(completed_filename, 'w')


  def set_completed(self, raw_completed):
    completed = raw_completed * (self.max_completed - self.min_completed) + self.min_completed
    self.f.write('%.3f' % completed)

    # Write to file and set initial for yourself
    self.f.flush()
    self.f.seek(0,0)

  def mark_done(self):
    # Write a final got it all and close the file.
    self.set_completed(1.0)
    self.f.close()