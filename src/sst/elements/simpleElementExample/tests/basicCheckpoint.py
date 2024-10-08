import sst

# Test a basic param
basicParam = sst.Component("basicParam", "simpleElementExample.basicParams")
params = {
    "int_param" : 20,
    "bool_param" : "false",
    "uint32_param" : "678",
    "array_param"  : "[0, 1, 5, 20, -1 ]",
    "example_param" : "a:92",
}
basicParam.addParams(params)

component0 = sst.Component("c0", "simpleElementExample.example0")
component1 = sst.Component("c1", "simpleElementExample.example1")

params = {
        "eventsToSend" : 50,    # Required parameter, error if not provided
        "eventSize" : 32        # Optional parameter, defaults to 16 if not provided
}
component0.addParams(params)
component1.addParams(params)

link = sst.Link("component_link0")
link.connect( (component0, "port", "1ns"), (component1, "port", "1ns") )


sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole") 
sst.enableAllStatisticsForComponentType("simpleElementExample.example0")
sst.enableAllStatisticsForComponentType("simpleElementExample.example1")