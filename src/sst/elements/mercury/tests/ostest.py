import sst
import sst.hg

node0 = sst.Component("Node0", "hg.Node")
os0 = node0.setSubComponent("os_slot", "hg.OperatingSystem")

link0 = sst.Link("link0")
link0.connect( (node0,"network","1ns"), (node0,"network","1ns") )

os0.addParams({ "app1.name" : "ostest"})
os0.addParams({ "app1.exe_library_name" : "ostest"})
os0.addParams({ "app1.argv" : "arg1 arg2"})

#node0.addParams({ "verbose" : "100"})
#os0.addParams({ "verbose" : "100"})
