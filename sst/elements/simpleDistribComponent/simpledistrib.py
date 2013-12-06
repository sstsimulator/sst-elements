import sst

d0 = sst.Component("d0", "simpleDistribComponent.SimpleDistribComponent")
d0.addParams({
		"distrib" : "gaussian",
#		"distrib" : "exponential",
		"mean" : "32",
		"stddev" : "4",
#		"lambda" : "0.5",
		"count" : "100000000",
		"binresults" : "1"
        })
