#!/usr/bin/python
import os
import sys

for x in range(2,13):
#os.system("./tests/run.py %d ddr4_verimem_openbank.cfg" % x)
	os.system("./tests/run.py %d ddr4_verimem.cfg" % x)
	os.system("./waterfall.py ddr4_trace > cmdseq_%d_new" % x)


