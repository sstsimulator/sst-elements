import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")


loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")
sub0 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompSender")
sub0.addParam("sendCount", 15)
sub0.enableAllStatistics()

loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")
sub1 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompReceiver")
sub1.enableAllStatistics()

link = sst.Link("myLink")
link.connect((sub0, "sendPort", "5ns"), (sub1, "recvPort", "5ns"))

sst.enableAllStatisticsForAllComponents()
sst.setStatisticLoadLevel(1)
