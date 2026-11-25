
import os
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")

serr_comp = sst.Component("serrano", "serrano.Serrano")
serr_comp.addParams({
	"verbose" : 26,
	"kernel0" : "tests/graphs/sum.graph"
	})
