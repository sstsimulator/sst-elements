# scheduler simulation input file
import sst

# Define SST core options
sst.setProgramOption("run-mode", "both")

# Define the simulation components
scheduler = sst.Component("myScheduler",             "scheduler.schedComponent")
scheduler.addParams({
      "traceName" : "test_MappingImpact_alltoall.sim",
      "machine" : "torus[2,2,2]",
      "coresPerNode" : "1",
      "scheduler" : "easy",
      "allocator" : "simple",
      "taskMapper" : "simple",
      "timeperdistance" : ".001865[.1569,0.0129]",
      "dMatrixFile" : "none",
      "detailedNetworkSim" : "ON",
      "completedJobsTrace" : "emberCompleted.txt",
      "runningJobsTrace" : "emberRunning.txt"
})

# nodes
n0 = sst.Component("n0", "scheduler.nodeComponent")
n0.addParams({
      "nodeNum" : "0",
})
n1 = sst.Component("n1", "scheduler.nodeComponent")
n1.addParams({
      "nodeNum" : "1",
})
n2 = sst.Component("n2", "scheduler.nodeComponent")
n2.addParams({
      "nodeNum" : "2",
})
n3 = sst.Component("n3", "scheduler.nodeComponent")
n3.addParams({
      "nodeNum" : "3",
})
n4 = sst.Component("n4", "scheduler.nodeComponent")
n4.addParams({
      "nodeNum" : "4",
})
n5 = sst.Component("n5", "scheduler.nodeComponent")
n5.addParams({
      "nodeNum" : "5",
})
n6 = sst.Component("n6", "scheduler.nodeComponent")
n6.addParams({
      "nodeNum" : "6",
})
n7 = sst.Component("n7", "scheduler.nodeComponent")
n7.addParams({
      "nodeNum" : "7",
})

# define links
l0 = sst.Link("l0")
l0.connect( (scheduler, "nodeLink0", "0 ns"), (n0, "Scheduler", "0 ns") )
l1 = sst.Link("l1")
l1.connect( (scheduler, "nodeLink1", "0 ns"), (n1, "Scheduler", "0 ns") )
l2 = sst.Link("l2")
l2.connect( (scheduler, "nodeLink2", "0 ns"), (n2, "Scheduler", "0 ns") )
l3 = sst.Link("l3")
l3.connect( (scheduler, "nodeLink3", "0 ns"), (n3, "Scheduler", "0 ns") )
l4 = sst.Link("l4")
l4.connect( (scheduler, "nodeLink4", "0 ns"), (n4, "Scheduler", "0 ns") )
l5 = sst.Link("l5")
l5.connect( (scheduler, "nodeLink5", "0 ns"), (n5, "Scheduler", "0 ns") )
l6 = sst.Link("l6")
l6.connect( (scheduler, "nodeLink6", "0 ns"), (n6, "Scheduler", "0 ns") )
l7 = sst.Link("l7")
l7.connect( (scheduler, "nodeLink7", "0 ns"), (n7, "Scheduler", "0 ns") )
