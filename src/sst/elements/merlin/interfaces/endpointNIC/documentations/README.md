# Merlin endpointNIC Models

This directory contains a NIC model that allows plug-in functionalities. 

The goal is to support a wider varity of NIC/smartNIC functionality for HPC networks in SST Merlin. 

Some possible applications:
- enable source routing (e.g., MPLS, SRv6, G-SRv6) [implemented in this PR]
- ECMP entropy/flow-hash manipulation (e.g., Ultra-Ethernet)
- end-to-end flow control (e.g., slingshot)
- RDMA direct memory operations
- ...

This builds an initial framework for plug-in NIC model, possible to support a variety of NIC functionalities. In its current status, source routing is implemented. 

## Python code architecture

In python, `endpointNIC` inherits from the `NetworkInterface` class, and it is implemented in the `pymerlin-interface` module

In the base class `job`, a new method `setEndpointNIC` and a new variable `networkIFparams` are defined to build the endpointNIC. In all (merlin, ember, mercury)

```python
    def setEndpointNIC(self, endpointNIC, params):
        """
        Set the NetworkInterface class and parameters to be used.\n
        The params argument is a dictionary of parameters that will be added to the endpointNIC
        """
        self.network_interface = endpointNIC
        self.networkIFparams = params # this will be later passed to the build() method of `endpointNIC`

```

this `self.networkIFparams` variable will be passed to the `build()` and `_instanceNetworkInterfaceBackCompat()` method when building the network_interface.

As a consequence:
- if linkcontrol is used, `endponitNIC.network_interface` will be an instance of `linkcontrol`.
- if reorderlinkcontrol is used, `endponitNIC.network_interface` will be an instance of `reorderlinkcontrol`, and the `endponitNIC.network_interface.network_interface` will be an instance of `linkcontrol`.

## C++ code architecture
In the C++ implementation, `endpointNIC` is essentially a siblings of `LinkControl` and `reorderlinkcontrol`.

There are three main components:

### 1. NICPlugin Interface (`NICPlugin.h`)
An abstract base class that defines the plugin interface. Each plugin must implement:
- `processOutgoing()`: Process packets from endpoint to network
- `processIncoming()`: Process packets from network to endpoint
- `getPluginName()`: Return plugin identifier

### 2. endpointNIC Class (`endpointNIC.h/cc`)
The main NIC component that:
- Manages a pipeline of NIC plugins loaded via SubComponent slots
- Forwards packets through the plugin pipeline bidirectionally
- Delegates actual network communication to LinkControl
- Processes outgoing packets through the pipeline in forward order
- Processes incoming packets through the pipeline in reverse order

### 3. PlugIn Implementations (e.g., `SourceRouting.h/cc`)
Individual plugin modules that implement specific NIC functionality.
  - **SourceRoutingPlugin** (implemented) : Adds source routing headers to packets
  - ECMP entropy manipulation
  - RDMA: send RDMA writes directly to spcific addresses of the memory
  - Endpoint-to-endpoint flow control: need to monitor flow bandwidths and send control messages.
  - ...

## Packet Flow

**Outgoing (endpoint → network):**
```
Endpoint → endpointNIC → Plugin1 → Plugin2 → ... → LinkControl → Network
```

**Incoming (network → endpoint):**
```
Network → LinkControl → ... → Plugin2 → Plugin1 → endpointNIC → Endpoint
```

## Usage Example (Python)

```python
job = Job(job_id, size)

# Add source routing plugin
sr_params = {
    "routing_string": "0:1.0,0,1,2|1:1.0,0,3,1",
    "path_selection_algorithm": "random_weighted"
}
job.addEndpointNICPlugin("merlin.sourceRoutingPlugin", sr_params)

# Add more plugins as needed
# job.addEndpointNICPlugin("merlin.ecmpPlugin", ecmp_params)

# When you call job.build(), it will automatically use endpointNIC
# if plugins were added, otherwise it uses the standard network interface
```

The `addEndpointNICPlugin()` method automatically sets `use_endpointNIC = True`. When `build()` is called, it will:
- Use endpointNIC with the plugin pipeline if `use_endpointNIC` is True and plugins are configured
- Use the standard network interface otherwise

## Adding New Plugins

To create a new plugin:

1. Inherit from `NICPlugin`
2. Implement `processOutgoing()` and `processIncoming()`, if necessary
3. Register with SST ELI macros
4. Add to build system

Example skeleton:

```cpp
class MyPlugin : public NICPlugin {
    SST_ELI_REGISTER_SUBCOMPONENT(MyPlugin, "merlin", "myPlugin", ...)
    
    Request* processOutgoing(Request* req, int vn) override {
        // Modify outgoing packet
        return req;
    }
    
    Request* processIncoming(Request* req, int vn) override {
        // Modify incoming packet
        return req;
    }
    
    std::string getPluginName() const override { return "MyPlugin"; }
};
```
