import sst

# Define SST core options
sst.setProgramOption("stopAtCycle", "10us")

loader0 = sst.Component("Loader0", "simpleElementExample.SubComponentLoader")
loader0.addParam("clock", "1.5GHz")
sub0 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompSender",0)
sub0.addParam("sendCount", 15)
sub0.enableAllStatistics()
#sub2 = loader0.setSubComponent("mySubComp", "simpleElementExample.SubCompReceiver",1)
#sub2.enableAllStatistics()

loader1 = sst.Component("Loader1", "simpleElementExample.SubComponentLoader")
loader1.addParam("clock", "1.0GHz")
sub1 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompReceiver",0)
sub1.enableAllStatistics()
#sub3 = loader1.setSubComponent("mySubComp", "simpleElementExample.SubCompSender",1)
#sub3.addParam("sendCount", 15)
#sub3.enableAllStatistics()

link = sst.Link("myLink1")
link.connect((sub0, "sendPort", "5ns"), (sub1, "recvPort", "5ns"))

#link = sst.Link("myLink2")
#link.connect((sub3, "sendPort", "5ns"), (sub2, "recvPort", "5ns"))

sst.enableAllStatisticsForAllComponents()
sst.setStatisticLoadLevel(1)
