# import the SST module
import sst
import sys
from PandosProgramming import *

# parameterize the components
params = {
    "num_cores" : 1,
    "instructions_per_task" : 20,
    "program_binary_fname": program_fpath(),
    "program_argv" : program_argv(),
    "verbose_level" : 1,
}

pn0 = Node("pn0", params)
