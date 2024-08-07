Implementing a new topology requires two parts:
* A C++ SubComponent derived from the Topology class defined in `sst/lelements/merlin/router.h`. The purpose of the C++ class is the provide all the functionality to assign routes to packets in the router.
* A python class derived from the Topology class in `sst/elements/merlin/pymerlin-base.py`. The python class is responsible for creating the connection graph for the simulation during simulation creation.

The C++ topology class is called only by `hr_router` to help it know how to move packets through the network. This is a  feature common to all SST SubComponents -- they are only called by the object that loads them.

The hyperx topology class in `sst/elements/merlin/topology` provides an example of what each of the functions does.

The merlin python module based off of classes in `pymerlin-base.py` is relatively new (as of January 2023) and has been in flux.

The hyperx topology in `sst/element/merlin/topology/pymerlin-topo-hyperx.py` will give you a good example of 
how the class works. It plugs into a broader set of classes, but only interacts with them through 
the APIs defined in the python Topology class.

The Python Topology class inherits from a base class that overloads many of the low level 
Python functions (`getattr` and `setattr`, for example). As such, you have to declare all variables 
that will be used in the class. If you try to use an undeclared variable, you will get an error. 
There are two types of these declarations: `Variables` and `Params`. The major difference is 
that `Params` are intended to be passes as parameters to underlying SST elements during build. 
There is a set of functions that allows you to group the parameters into sets that can be passed 
to the correct object. In the hyperx class, there are two calls to these functions. 
The call to `_declareClassVariables()` creates variable that are used by the class, but 
not passed to any elements during build(). The call to `_declareParams("main", [...])` declares a set of 
variables that will end up in a dictionary called "main" that can later be fetched and passed a 
call to `addParams()` for an element instance in the `build()` function. All of the variables 
declared with either of these functions can be accessed like a normal class data member 
(e.g., `comp.link_latency`, `comp.algorithm`, etc.).


The virtual network is a common abstraction used in networks. They represent a set of independent resources through the network made up of one or more virtual channels (sometimes called virtual lanes). A given packet will stay in the virtual network on which it was injected into the router, but can change virtual channels based on the requirements of the routing algorithm to avoid deadlock in the network. When 
the Topology object is created in `hr_router`, it will be given the number of virtual networks required.
