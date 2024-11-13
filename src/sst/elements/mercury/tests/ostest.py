import sst
import sst.hg

node0 = sst.Component("Node0", "hg.Node")
node1 = sst.Component("Node1", "hg.Node")
os0 = node0.setSubComponent("os_slot", "hg.OperatingSystem")
os1 = node1.setSubComponent("os_slot", "hg.OperatingSystem")

link0 = sst.Link("link0")
link0.connect( (node0,"network","1ns"), (node1,"network","1ns") )

os0.addParams({ "app1.name" : "ostest"})
os1.addParams({ "app1.name" : "ostest"})
os0.addParams({ "app1.exe" : "ostest.so"})
os1.addParams({ "app1.exe" : "ostest.so"})

#node0.addParams({ "verbose" : "100"})
#node1.addParams({ "verbose" : "100"})
#os0.addParams({ "verbose" : "100"})
#os1.addParams({ "verbose" : "100"})
