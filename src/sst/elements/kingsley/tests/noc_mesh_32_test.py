# Automatically generated SST Python input
import sst

sst.setProgramOption("timebase", "1ps")
#sst.setProgramOption("stopAtCycle", "1000ns")

x_size = 4
y_size = 4

# put in the routers

links = dict()
def getLink(name1, name2):
    name = "link.%s:%s"%(name1, name2)
    if name not in links:
        links[name] = sst.Link(name)
    return links[name]

num_endpoints = 1

num_peers = (num_endpoints * (x_size * y_size)) + (2*x_size) + (2*y_size)
#num_peers = x_size * y_size
num_messages = 10
msg_size = "64B"
link_bw = "32GB/s"
flit_size = "32B"
input_buf_size = "64B"
#input_buf_size = "256B"

# Setting this to True will cause no-cut links on the north and south
# ports, as well as on all endpoints
add_no_cut = False

for y in xrange(y_size):
    for x in xrange(x_size):
        rtr = sst.Component("rtr.%d.%d"%(x,y), "kingsley.noc_mesh")
        rtr.addParams({
            "local_ports" : "%d"%(num_endpoints),
            "link_bw" : link_bw,
            "input_buf_size" : input_buf_size,
            "flit_size" : flit_size,
            "use_dense_map" : "true"
            #"port_priority_equal" : "true"
        })
        # wire up mesh connections
        if y != y_size - 1:
            rtr.addLink(getLink("rtr.%d.%d"%(x,y), "rtr.%d.%d"%(x,y+1)), "north", "800ps")
            if add_no_cut:
                getLink("rtr.%d.%d"%(x,y), "rtr.%d.%d"%(x,y+1)).setNoCut()
        else:
            rtr.addLink(getLink("rtr.%d.%d"%(x,y), "ep0.%d.%d"%(x,y+1)), "north", "800ps")
            if add_no_cut:
                getLink("rtr.%d.%d"%(x,y), "ep0.%d.%d"%(x,y+1)).setNoCut()
            ep = sst.Component("ep0.%d.%d"%(x,y+1), "merlin.test_nic")
            ep.addParams({
                "num_peers" : "%d"%(num_peers),
                "link_bw" : "1GB/s",
                "linkcontrol_type" : "kingsley.linkcontrol",
                "message_size" : msg_size,
                "num_messages" : "%d"%(num_messages)
            })
            ep.addLink(getLink("rtr.%d.%d"%(x,y), "ep0.%d.%d"%(x,y+1)), "rtr", "800ps")
            
            
        if y != 0:
            rtr.addLink(getLink("rtr.%d.%d"%(x,y-1), "rtr.%d.%d"%(x,y)), "south", "800ps")
        else:
            rtr.addLink(getLink("rtr.%d.%d"%(x,y-1), "ep0.%d.%d"%(x,y)), "south", "800ps")
            if add_no_cut:
                getLink("rtr.%d.%d"%(x,y-1), "ep0.%d.%d"%(x,y)).setNoCut()
            ep = sst.Component("ep0.%d.%d"%(x,y-1), "merlin.test_nic")
            ep.addParams({
                "num_peers" : "%d"%(num_peers),
                "link_bw" : "1GB/s",
                "linkcontrol_type" : "kingsley.linkcontrol",
                "message_size" : msg_size,
                "num_messages" : "%d"%(num_messages)
            })
            ep.addLink(getLink("rtr.%d.%d"%(x,y-1), "ep0.%d.%d"%(x,y)), "rtr", "800ps")

        if x != x_size - 1:
            rtr.addLink(getLink("rtr.%d.%d"%(x,y), "rtr.%d.%d"%(x+1,y)), "east", "800ps")
        else:
            rtr.addLink(getLink("rtr.%d.%d"%(x,y), "ep0.%d.%d"%(x+1,y)), "east", "800ps")
            if add_no_cut:
                getLink("rtr.%d.%d"%(x,y), "ep0.%d.%d"%(x+1,y)).setNoCut()
            ep = sst.Component("ep0.%d.%d"%(x+1,y), "merlin.test_nic")
            ep.addParams({
                "num_peers" : "%d"%(num_peers),
                "link_bw" : "1GB/s",
                "linkcontrol_type" : "kingsley.linkcontrol",
                "message_size" : msg_size,
                "num_messages" : "%d"%(num_messages)
            })
            ep.addLink(getLink("rtr.%d.%d"%(x,y), "ep0.%d.%d"%(x+1,y)), "rtr", "800ps")

        if x != 0:
            rtr.addLink(getLink("rtr.%d.%d"%(x-1,y), "rtr.%d.%d"%(x,y)), "west", "800ps")
        else:
            rtr.addLink(getLink("rtr.%d.%d"%(x-1,y), "ep0.%d.%d"%(x,y)), "west", "800ps")
            if add_no_cut:
                getLink("rtr.%d.%d"%(x-1,y), "ep0.%d.%d"%(x,y)).setNoCut()
            ep = sst.Component("ep0.%d.%d"%(x-1,y), "merlin.test_nic")
            ep.addParams({
                "num_peers" : "%d"%(num_peers),
                "link_bw" : "1GB/s",
                "linkcontrol_type" : "kingsley.linkcontrol",
                "message_size" : msg_size,
                "num_messages" : "%d"%(num_messages)
            })
            ep.addLink(getLink("rtr.%d.%d"%(x-1,y), "ep0.%d.%d"%(x,y)), "rtr", "800ps")


        # Add endpoints
        for z in xrange(num_endpoints):
            rtr.addLink(getLink("rtr.%d.%d"%(x,y), "ep%d.%d.%d"%(z,x,y)), "local%d"%(z), "800ps")
            if add_no_cut:
                getLink("rtr.%d.%d"%(x,y), "ep%d.%d.%d"%(z,x,y)).setNoCut()
            ep = sst.Component("ep%d.%d.%d"%(z,x,y), "merlin.test_nic")
            ep.addParams({
                "num_peers" : num_peers,
                "link_bw" : "1GB/s",
                "linkcontrol_type" : "kingsley.linkcontrol",
                "message_size" : msg_size,
                "num_messages" : "%d"%(num_messages)
                
            })
            ep.addLink(getLink("rtr.%d.%d"%(x,y), "ep%d.%d.%d"%(z,x,y)), "rtr", "800ps")


sst.setStatisticLoadLevel(9)

sst.setStatisticOutput("sst.statOutputCSV");
sst.setStatisticOutputOptions({
    "filepath" : "stats.csv",
    "separator" : ", "
})

sst.enableAllStatisticsForComponentType("kingsley.noc_mesh", {"type":"sst.AccumulatorStatistic","rate":"0ns"})
