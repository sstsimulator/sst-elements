import sst
import sst.merlin

import random

class app_state:
    def __init__(self):
        self.traffic_class = -1
        self.nids = None

_nids = None
_apps = []
        
def _random( args ):
    # args: size [seed]
    global _nids
    size = int(args[0])

    if len(args) is 2 :
        random.seed(int(args[1]))
    
    random.shuffle(_nids)

    nid_list = _nids[0:size]
    _nids = _nids[size:]
    return nid_list

def _random_linear( args ):
    # args: size [seed]
    global _nids
    size = int(args[0])

    if len(args) is 2 :
        random.seed(int(args[1]))
    
    random.shuffle(_nids)

    nid_list = _nids[0:size]
    _nids = _nids[size:]
    nid_list.sort()
    return nid_list


def _linear( args ):
    # args: size
    global _nids
    size = int(args[0])
    
    _nids.sort()

    nid_list = _nids[0:size]
    _nids = _nids[size:]
    return nid_list
    #return ','.join(str(num) for num in nid_list)


def _interval( args ):
    # args: start_index interval [count]
    
    # if count is not specified, it will go through the end.  If count
    # takes you beyond the end, it will stop at the end of the nid
    # list
    global _nids
    start_index = int(args[0])
    interval = int(args[1])
    if len(args) >= 3:
        count = int(args[2])
    else:
        count = sys.maxsize
        
    # Start with a sorted nid space
    _nids.sort()

    nid_list = []
    index = start_index
    for i in range(0,count):
        if ( index < len(_nids) ):
            nid_list.append( _nids.pop(index) )
            index = index + interval - 1 # -1 because we popped a value
        else:
            break
            
    return nid_list



_functions = { "random" : _random,
               "random-linear" : _random_linear,
               "linear" : _linear,
               "interval" : _interval }

def _finalize_qos_config(total_nodes) :
    # Using the _apps array, create the qos merlin variables and put
    # them in the sst.merlin._params array

    # get the highest traffic class
    max = 0
    for app in _apps:
        if app.traffic_class > max:
            max = app.traffic_class

    num_vns = max + 1

    # set up number of VNs.  Ember currently uses two VNs per class,
    # so we need to multiply by 2
    sst.merlin._params["num_vns"] = num_vns

    # set up the vn_remap array
    mylist = [-1] * ( num_vns * total_nodes )

    # walk through apps array and set up the VN map
    for app in _apps:
        for nid in app.nids:
            index = (nid * num_vns) + app.traffic_class
            mylist[index] = 0

    sst.merlin._params["vn_remap_shm"] = "ember_vn_remap"
    sst.merlin._params["vn_remap"] = mylist

    #sst.merlin._params["portcontrol:output_arb"] = "merlin.arb.output.qos.multi"
    #sst.merlin._params["portcontrol:arbitration:qos_settings"] = qos_settings
    

def generate( args ):

    # arguments are: total_nodes, traffic class, allocation type, allocation args...
    args = args.split(',')

    total_nodes = int(args[0])
    traffic_class = int(args[1])
    allocation = args[2]

    global _nids
    if _nids is None:
        _nids = list(range(total_nodes))

    # allocation types and their args:
    # linear size
    # interval start length interval
    # random size
    
    new_args = args[3:]

    # call function
    nid_list = _functions[allocation](new_args)
    app = app_state()
    app.traffic_class = traffic_class
    app.nids = nid_list

    global _apps
    _apps.append(app)

    if len(_nids) is 0 :
        _finalize_qos_config(total_nodes)
    
    return ','.join(str(num) for num in nid_list)
        
    #if int(args) is 1:
    #    return '0-15'
    #if int(args) is 2:
    #    return '16-31'
