import sst
import sst.hg

node0 = sst.Component("Node0", "hg.node")
node1 = sst.Component("Node1", "hg.node")

node0.addParams({ "app1.name" : "testme"})
node1.addParams({ "app1.name" : "testme"})
node0.addParams({ "app1.exe" : "testme"})
node1.addParams({ "app1.exe" : "testme"})
node0.addParams({ "verbose" : "100"})
node1.addParams({ "verbose" : "100"})
node0.addParams({ "OperatingSystem.verbose" : "100"})
node1.addParams({ "OperatingSystem.verbose" : "100"})
