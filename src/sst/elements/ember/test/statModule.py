import sst

def init( outputFile ):

	sst.setStatisticLoadLevel(9)

	sst.setStatisticOutput("sst.statOutputCSV");
	sst.setStatisticOutputOptions({
    	"filepath" : outputFile,
    	"separator" : ", "
	})

	sst.enableStatisticForComponentType("firefly.nic",'mem_num_stores',{"type":"sst.AccumulatorStatistic","rate":"0ns"})
	sst.enableStatisticForComponentType("firefly.nic",'mem_num_loads',{"type":"sst.AccumulatorStatistic","rate":"0ns"})

	sst.enableStatisticForComponentType("firefly.nic",'mem_addrs',
	{"type":"sst.HistogramStatistic",
		"rate":"0ns",
		"binwidth":"4096",
		"numbins":"512"}
	)
