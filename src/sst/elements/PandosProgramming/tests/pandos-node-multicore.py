# import the SST module
import sst
import sys
from PandosProgramming import *

# parameterize the components
params = {
    "num_cores" : 2,
    "instructions_per_task" : 20,
    "program_binary_fname": program_fpath(),
    "program_argv" : program_argv(),
    "verbose_level" : 4,
    "debug_scheduler" : False,
    "debug_memory_requests" : False,
    "debug_initialization" : False,
}


# create a single pandos node
nf = NodeFactory(params)
n0 = nf.createNode('n0')
nf.done()
