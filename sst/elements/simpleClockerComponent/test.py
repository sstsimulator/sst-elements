import sst

def sstcreatemodel():

    sst.creategraph()

    if sst.verbose():
        print "Verbose Model"

    for x in range(0, 8):
      id = sst.createcomponent("a" + str(x), "simpleClockerComponent.simpleClockerComponent")
      sst.addcompparam(id, "clock", "1MHz")
      sst.addcompparam(id, "clockcount", "100")
      sst.setcompweight(id, x * 1.5)

    return 0

