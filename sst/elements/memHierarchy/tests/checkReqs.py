#!/usr/bin/env python

import re
import sys;


reqPattern = re.compile('\A0:Cache::handleCPURequest\(\):[0-9]+: ([^ ]+) (\([0-9]+, [0-9]+\))')
respPattern = re.compile('\A0:Cache::makeCPUResponse\(\):[0-9]+: ([^ ]+) Creating Response to CPU: \([^)]+\) in Response To (\([0-9+, [0-9]+\))')


reqs = set()

with open(sys.argv[1]) as f:
    for line in f:
        reqMap = reqPattern.match(line)
        if reqMap:
            #print "req Matched ", line
            key = reqMap.group(1, 2)
            reqs.add(key)
        else:
            respMap = respPattern.match(line)
            if respMap:
                #print "resp Matched", line
                key = respMap.group(1, 2)
                if key not in reqs:
                    print "Found response for missing request:", key
                else:
                    reqs.remove(key)

for key in reqs:
    print "Incomplete request:  ", key

