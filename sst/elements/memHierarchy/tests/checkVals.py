#!/usr/bin/env python

import re
import sys;


reqPattern = re.compile('\A0:Cache::makeCPUResponse\(\):[0-9]+: ([^ ]+) Creating Response to CPU: \([0-9+, [0-9]+\) in Response To (\([0-9]+, [0-9]+\)) \[(WriteResp|ReadResp): (0x[0-9a-f]+)\] \[(0x[0-9a-f]+)\]')


reqs = dict()

loads = 0
matchedLoads = 0
stores = 0

with open(sys.argv[1]) as f:
    for line in f:
        reqMap = reqPattern.match(line)
        if reqMap:
            cpu = reqMap.group(1)
            tag = reqMap.group(2)
            cmd = reqMap.group(3)
            addr = reqMap.group(4)
            val = reqMap.group(5)

            if cmd == "WriteResp":
                reqs[addr] = val
                stores = stores + 1
            else:
                loads = loads + 1
                if addr in reqs:
                    matchedLoads = matchedLoads + 1
                    oldval = reqs[addr]
                    if oldval != val:
                        print "Read %s of address %s has value %s.  Does not match old value of %s." % (tag, addr, val, oldval)


print "Checked %d stores, matched %d loads (%d total)" % (stores, matchedLoads, loads)
