'''
Converts workload traces from the standard workload format
to SST scheduler format
usage: python convertTrace.py <input file name>
'''

import sys
import numpy as np

if __name__ == '__main__':
  #check input
  if len(sys.argv) != 2:
    print "usage: python convertTrace.py <input file name>"
    sys.exit()
  #load data
  print "Converting..."
  data = np.loadtxt(sys.argv[1], comments = ';')
  data = np.array([data[:,1], data[:,3], data[:,4], data[:,8]])
  data = np.int_(data.T)
  #write data
  np.savetxt(sys.argv[1] + ".sim", data, delimiter=' ', fmt='%.0f')
  print "Completed!"
