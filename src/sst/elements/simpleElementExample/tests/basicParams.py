# Import the SST module
import sst

# The basicParams.py script demonstrates passing parameters to a component
# The parameters aren't used for anything, but the constructor will print
# the values of the parameters it received and check that the required parameter
# was provided (fatal error if not). The component always runs for 20 us.
#
# There are multiple ways to pass parameters to a component in an input file, as shown below
#
#   basicParams takes the following parameters:
#       int_param       : Parameter which will be read as an int, this is a required parameter
#       bool_param      : Parameter which will be read as a bool, default is false
#       string_param    : Parameter which will be left as a string, default is "hello"
#       uint32_param    : Parameter which will be read as a uint32_t, default is 400
#       double_param    : Parameter which will be read as a doube, default is 12.5
#       ua_param        : Parameter which will be read as a UnitAlgebra, default is 2TB/s
#       example_param   : Parameter which will be read as an ExampleType (defined in basicParams.h as a string key and integer value separated by ':'), default is "key:5"
#       array_param     : Parameter that takes an array of values. Arrays must be enclosed in brackets and string elements do not have to be enclosed in quotes. Python lists are supported.
#
# This component has no links and SST will produce a warning because that is an unusual configuration
# that often points to a mis-configuration. For this simulation, the warning can be ignored.
#
# Relevant code:
#   simpleElementExample/basicParams.h
#   simpleElementExample/basicParams.cc
# Output:
#   simpleElementExample/tests/refFiles/basicParams.out
#

### Create the component
component = sst.Component("ParamComponent", "simpleElementExample.basicParams")

### Parameterize the component.
# Run 'sst-info simpleElementExample.basicParams' at the command line 
# to see parameter documentation


# Example 1: Pass parameters using a dictionary

requiredParam = 20

# Array parameters can be directly passed from python lists 
arrayP = [12, 50, -8, 0, 0, 48939]

params = {
    "int_param" : requiredParam,    # This is a required parameter for the basicParams component.
    "bool_param" : "false",         # Bool types can use 0/1, "T/F" or "true/false" (case insensitive)
    "uint32_param" : "678",         # SST interprets all parameters as strings and the component initiates a conversion to a specific type. 
                                    # So the user can pass a string or anything that can be trivially converted to a string.
    "array_param"  : arrayP,        # Parameters can take arrays too. 
#   "array_param"  : "[0, 1, 5, 20, -1 ]", # Strings formatted like an array are fine too
    "example_param" : "a:92",
#   "double_param"  : 22.9,          # If we don't provide this, the parameter will take its default value
}
component.addParams(params)


# Example 2: Add a parameter directly
component.addParam("string_param", "i am string")
component.addParam("ua_param", "10MB/s")    # A UnitAlgebra type has units associated and SI prefixes are recognized (e.g., GHz, KB, MiB, bps, B/s, us, etc.)
component.addParam("bool_param", "T")      # Overwritten params take on the last added value (bool_param is also set above in the 'params' dictionary)

