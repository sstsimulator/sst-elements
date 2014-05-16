#! /usr/bin/env python

import sys
from subprocess import call

cmd = "time mpirun -n " + sys.argv[1] + " sst " + sys.argv[2] + " emberLoad.py"

print cmd

call( [cmd], shell=True )
