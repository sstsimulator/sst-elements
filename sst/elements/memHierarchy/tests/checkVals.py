#!/usr/bin/env python

import re
import sys;


reqPattern = re.compile('\A[0-9]+:Cache::makeCPUResponse\(\):[0-9]+ ([^ ]+): Creating ([0-9]+) byte Response to CPU: \([0-9+, [0-9]+\) in Response To (\([0-9]+, [0-9]+\)) \[(WriteResp|ReadResp): (0x[0-9a-f]+)\] \[0x([0-9a-f]+)\]')


reqs = dict()

loads = 0
matchedLoads = 0
stores = 0

with open(sys.argv[1]) as f:
    for line in f:
        reqMap = reqPattern.match(line)
        if reqMap:
            cpu = reqMap.group(1)
            size = int(reqMap.group(2))
            tag = reqMap.group(3)
            cmd = reqMap.group(4)
            addr = int(reqMap.group(5), 0)
            val = reqMap.group(6)

            if cmd == "WriteResp":
                for byte in range(size):
                    reqs[addr + byte] = val[byte*2:(byte+1)*2]
                    stores = stores + 1
            else:
                loads = loads + 1
                for byte in range(size):
                    addr2 = addr + byte
                    if addr2 in reqs:
                        matchedLoads = matchedLoads + 1
                        oldval = reqs[addr2]
                        if oldval != val[byte*2:(byte+1)*2]:
                            sys.stderr.write( "Read %s of address %s has value %s.  Does not match old value of %s.\n\n" % (tag, addr, val, oldval))
                            sys.exit(0)


sys.stderr.write( "Checked %d stores, matched %d loads (%d total)\n" % (stores, matchedLoads, loads))
