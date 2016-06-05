
import sst
from sst.merlin import *

def setParams( networkParams, rtrArb, topParams ):

	sst.merlin._params["link_lat"] = networkParams['link_lat']
	sst.merlin._params["link_bw"] = networkParams['link_bw']
	sst.merlin._params["xbar_bw"] = networkParams['link_bw']
	sst.merlin._params["flit_size"] = networkParams['flitSize']
	sst.merlin._params["input_latency"] = networkParams['input_latency']
	sst.merlin._params["output_latency"] = networkParams['output_latency']
	sst.merlin._params["input_buf_size"] = networkParams['buffer_size']
	sst.merlin._params["output_buf_size"] = networkParams['buffer_size']

	if "network_inspectors" in networkParams.keys():
		sst.merlin._params["network_inspectors"] = networkParams['network_inspectors']

	if rtrArb:
		sst.merlin._params["xbar_arb"] = "merlin." + rtrArb

	sst.merlin._params.update( topParams )
