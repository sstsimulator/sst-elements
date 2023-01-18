# import the SST module
import sst

# create a single pandos node
pn0 = sst.Component("pn0", "PandosProgramming.PandosNodeT")

# parameterize the components
params = {
    
}

pn0.addParams(params)
