#!/usr/bin/env python

# Copyright 2009-2022 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2022, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# of the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: MIT License

# Authors: Kartik Lakhotia 

import sst
from sst.merlin.base import *
from sst.merlin.topology import topoPolarFly

from os import path, makedirs
import numpy as np

import os
import sys

try:
    import networkx as nx
except: 
    pass
    #print('--> MODULE NOT FOUND: networkx')

supp_path   = os.environ["SST_ELEMENTS_ROOT"]
sys.path.append(supp_path+"/src/sst/elements/merlin/topology/")

from GF import *

#Paley Supernode
class Paley():
    #q -> order (no. of vertices), degree = (q-1)/2
    def __init__(self, q):
        assert(q%4 == 1)
        self.q          = q
        self.gf         = GF(q)
        self.pe         = self.gf.getPrimitiveElem()
        self.vertices   = self.gf.getElems()
        assert(len(self.vertices)==q)

        self.pe_powers  = None
        #Generator set
        self.X          = None

        #Phi mapping
        self.phi        = None

        #topology
        self.topo       = None


    def make(self):
        print("----> Generating Paley Graph")
        self.makePhi()
        self.makeTopo()
        if 'networkx' in sys.modules:
            if not self.validate():
                raise Exception("Validation not passed!")
        return self.phi, self.topo


    def validate(self):
        print("--> Validating Paley(%d):" %self.q)
        # check sizes
        if (len(self.topo) != self.q):
            print("     --> construction error: incorrect number of nodes")
            return 0

        # check symmetry
        for i in range(len(self.topo)):
            for j in self.topo[i]:
                if (i not in self.topo[j]):
                    print("    --> construction not undirected, adjacency matrix asymmetric")
                    return 0

        # construct a nx graph (undirected, no multi-edges) for further validation
        nx_graph = nx.Graph()
        for i in range(len(self.topo)):
            nx_graph.add_node(i)
            for j in self.topo[i]:
                nx_graph.add_edge(i, j)

        # check max degree
        max_degree = max(dict(nx_graph.degree).values())
        if max_degree != (self.q-1)//2:
             print("     --> construction error: incorrect degrees")
             return 0

        # check if graph is connected
        if not nx.is_connected(nx_graph):
            print("     --> construction error: not conncected")
            return 0

        #check diameter
        diameter = nx.diameter(nx_graph)
        if diameter != 2:
            print("     --> construction error: incorrect diameter")
            return 0

        #check property R1/P1 - adding f(E(G)) edges should give complete graph
        for u,v in nx_graph.edges():
            nx_graph.add_edge(self.phi[u], self.phi[v])
        diameter = nx.diameter(nx_graph)
        if diameter != 1:
            print("     --> construction error: property R1 not satisfied")
            return 0 

        return 1


    def makePhi(self):
        if self.phi is None:
            self.phi    = {}
            q           = self.q
            for i in range(q):
                u       = self.vertices[i]
                for j in range(q):
                    v   = self.vertices[j]
                    if (self.gf.mul(u, self.pe) == v):
                        assert(i not in self.phi)
                        self.phi[i]  = j

    def makeTopo(self):
        if self.topo is None:
            q               = self.q
            self.pe_powers  = [1]
            for i in range(1,q):
                self.pe_powers.append(self.gf.mul(self.pe_powers[i-1], self.pe))
            #Generator set - squares in GF(q)
            self.X          = [self.pe_powers[i] for i in range(0, q-2, 2)]

            graph           = [[] for _ in range(q)]
            for i in range(q):
                for j in range(q):
                    u       = self.vertices[i]
                    v       = self.vertices[j]
                    #if difference is a square in GF(q)
                    if (self.gf.sub(v,u) in self.X):
                        graph[i].append(j)
                graph[i].sort()

            self.topo       = graph



#Inductive-quad Supernode
class IQ():
    #q -> degree, order (no. of vertice) = 2q+2
    def __init__(self, q):
        assert((q%4==0) or (q%4==3)) 
        self.q      = q
        self.V      = 2*q + 2
        self.A      = []
        self.fA     = []
        self.topo   = None
        self.phi    = None
        self.incr   = 4


    def make(self):
        print("----> Generating Inductive-quad Graph")
        if self.topo is None:
            self.makeTopo()
        if 'networkx' in sys.modules:
            if not self.validate():
                raise Exception("Validation not passed!")
        return self.phi, self.topo


    def validate(self):
        print("--> Validating Inducitve-Quad(%d):" %self.q)
        #check sizes
        if (len(self.topo) != 2*self.q + 2):
            print("     --> construction error: incorrect number of nodes")
            return 0

        # check symmetry
        for i in range(len(self.topo)):
            for j in self.topo[i]:
                if (i not in self.topo[j]):
                    print("    --> construction not undirected, adjacency matrix asymmetric")
                    return 0
            f2I = self.phi[self.phi[i]]
            if (f2I != i):
                print("    --> function 'f' not symmetric")
                return 0

        # construct a nx graph (undirected, no multi-edges) for further validation
        nx_graph = nx.Graph()
        for i in range(len(self.topo)):
            nx_graph.add_node(i)
            for j in self.topo[i]:
                nx_graph.add_edge(i, j)

        # check max degree
        max_degree = max(dict(nx_graph.degree).values())
        if max_degree != self.q:
             print("     --> construction error: incorrect degrees")
             return 0

        #check property R* - adding f(E(G)) and (x,f(x)) edges should give complete graph
        for u in nx_graph.nodes():
            nx_graph.add_edge(u, self.phi[u]) 
        for u,v in nx_graph.edges():
            nx_graph.add_edge(self.phi[u], self.phi[v])
        diameter = nx.diameter(nx_graph)
        if diameter != 1:
            print("     --> construction error: property R1 not satisfied")
            return 0 

        return 1

    def makeBase(self):
        q           = self.q
        V           = self.V 
        self.topo   = [[] for _ in range(V)]
        self.phi    = {}
        #degree 0 base
        if (q%self.incr == 0):
            self.A   = [0]
            self.fA  = [1]
            self.phi = {0:1, 1:0}     
        #degree 3 base
        else:
            incrG, self.A, self.fA, self.phi    = self.makeIncr()
            incrV                               = len(incrG)
            incrD                               = (incrV-2)//2
            assert(incrV == 2*self.incr)
            for i in range(incrV):
                neighs      = len(incrG[i])
                assert(neighs==incrD)
                for j in incrG[i]:
                    self.topo[i].append(j)

    def makeIncr(self):
        A       = [i for i in range(self.incr)]
        fA      = [i + self.incr for i in range(self.incr)]
        phi     = {}
        incrV   = 2*self.incr
        incrG   = [[] for _ in range(incrV)]
        x       = 0
        fx      = x + self.incr
        #add edges incident with x and f(x)
        for i in A:
            if (i != x):
                incrG[x].append(i)
                incrG[i].append(x)
                incrG[fx].append(i)
                incrG[i].append(fx)
        #add rest of the edges
        for i in A:
            if (i != x):
                y   = i 
                fy  = self.incr + y
                w   = (y%(self.incr-1)) + 1
                fw  = self.incr + w
                incrG[y].append(fw)
                incrG[fw].append(y)
                incrG[fy].append(fw)
                incrG[fw].append(fy)   
        #make phi
        for i in range(self.incr):
            phi[i]              = i + self.incr
            phi[i+self.incr]    = i

        return incrG, A, fA, phi 
           
             
    def makeTopo(self):
        q       = self.q
        V       = self.V
        if self.topo is None:
            #make degree 0 or degree 3 base
            self.makeBase()     
            currDeg = q%self.incr
            currV   = 2*currDeg + 2
            #while construction does not complete
            #incrementally add the IQ quadrilateral
            while(currDeg < q):
                incrG, incrA, incrfA, incrPhi   = self.makeIncr()
                incrV                           = len(incrG)
                #add edges internal to new quadrilateral
                for i in range(incrV):
                    u   = i + currV
                    assert(u < V)
                    for j in incrG[i]:
                        v   = j + currV
                        assert(v < V)
                        self.topo[u].append(v)
                #connect existing topology and new quadrilateral
                connA   = self.incr//2
                for i in range(self.incr):
                    u           = incrA[i] + currV
                    fu          = incrPhi[incrA[i]] + currV
                    assert((u < V) and (fu < V))
                    self.phi[u] = fu
                    self.phi[fu]= u
                    for j in range(len(self.A)):
                        #connect half the pairs to A and half the pairs to fA
                        v   = self.A[j] if (i < connA) else self.fA[j]
                        self.topo[u].append(v)
                        self.topo[v].append(u)
                        self.topo[fu].append(v)
                        self.topo[v].append(fu)
                #expand A, fA
                for i in incrA:
                    self.A.append(i + currV)
                for i in incrfA:
                    self.fA.append(i + currV)

                currDeg += self.incr
                currV   += incrV
                    
        








class topoPolarStar(Topology):

    """
    Parameters:
        d       : degree
        sn      : ennum {max, iq, paley}
        pfq     : prime power for ER graphs, <0 implies optimize for scale
        snq     : parameter 'q' for supernode graphs, <0 implies optimize for scale
        N       : number of routers
    """
    def __init__(self, d=-1, sn="max", pfq=-1, snq=-1, N=-1):
        print("PolarStar Topology Constructor!")
        Topology.__init__(self)
        self._declareClassVariables(["link_latency", "host_link_latency", "global_link_map", "bundleEndpoints"])
        self._declareParams("main",["topo","phi","d","sn_type","pfq","snq","pfV", "snV", "phi", "hosts_per_router","network_radix","total_radix","total_routers",
                                    "total_endnodes","edge","name","algorithm","adaptive_threshold","global_routes","config_failed_links",
                                    "failed_links"])
        self.global_routes      = "absolute"
        self._subscribeToPlatformParamSet("topology")

        #Configure
        self.d                  = d
        if (d<0 and N>=0):
            self.d       = np.cbrt(3*N) 
            sn           = "max"
        elif (d>=0 and N<0):
            if (d < 3):
                raise Exception("Degree too small\n")

        self.pfq                = pfq
        self.snq                = snq
        self.sn_type            = sn 
        if (pfq<0 and snq<0):
            self.pfq, self.snq, self.sn_type = self.optPSConfig(d, sn) 
        else:
            if (pfq<0 or snq<0 or (sn!="paley" or sn!="iq")):
                raise Exception("Infeasible configuration -- specify all parameters or none")

        #Set Shape
        self.pfV                = self.pfq*self.pfq + self.pfq + 1
        self.snV                = self.snq if (self.sn_type == "paley") else 2*self.snq + 2 
      
        self.network_radix      = self.d
        self.total_routers      = self.pfV*self.snV
        self.edge               = (self.total_routers*self.d)/2
        self.name               = "PolarStar"
        self.topo               = None
        self.phi                = None
        self.hosts_per_router   = None
        self.total_radix        = None 
        self.total_endnodes     = None 


    def setEP(self):    
        if self.hosts_per_router is None:
            self.hosts_per_router   = self.network_radix//3
        self.total_radix        = self.network_radix + self.hosts_per_router
        self.total_endnodes     = self.total_routers*self.hosts_per_router

        
    def getName(self):
        return self.name


    def get_info(self):
        return "name=%s p=%d net-radix=%d radix=%d R=%d N=%d" %(self.name, self.host_link_latency, self.network_radix, self.total_radix, self.total_routers, self.total_endnodes)


    def getFileName(self):
        return "PolarStar.d_" + str(self.d) + "_pfq_" + str(self.pfq) + "_sn_" + self.sn_type + "_snq_" + str(self.snq)


    def getNumNodes(self):
        if (self.total_endnodes is None):
            self.setEP()
        return self.total_endnodes


    def getNumEdges(self): 
        if self.topo==None:
            self.make()
        return sum(len(n) for n in self.topo)//2


    def getFolderPath(self):
        return os.getcwd()+"/polarstar_data/"


    def make(self):
        if self.topo is None:
            self.topo   = self.starProd(self.pfq, self.snq, self.sn_type)

        return self.topo


    def validate(self):
        print("--> Validating PolarStar(%d):" %self.d)

        #check sizes
        if len(self.topo) != self.total_routers:
            print("    --> construction error: incorrect number of nodes")
            return 0

        # check symmetry
        for i in range(len(self.topo)):
            for j in self.topo[i]:
                if (i not in self.topo[j]):
                    print("    --> construction not undirected, adjacency matrix asymmetric")
                    return 0

        #construct networkx graph (undirected, no multi-edges) for further validation
        nx_graph    = nx.Graph()
        for i in range(len(self.topo)):
            nx_graph.add_node(i)
            for j in self.topo[i]:
                nx_graph.add_edge(i, j)

        # check if graph is connected
        if not nx.is_connected(nx_graph):
            print("     --> construction error: not conncected")
            return 0

        # check max degree
        max_degree = max(dict(nx_graph.degree).values())
        if max_degree != self.d:
             print("     --> construction error: incorrect degrees")
             return 0

        #check diameter
        diameter = nx.diameter(nx_graph)
        if diameter > 3:
            print("     --> construction error: incorrect diameter")
            return 0

        return 1


    def generate(self, validate=False, save=False):
        print("------> Generating Polarstar topology!!")
        self.make()
        self.setEP()
        
        if validate:
            if not self.validate():
                raise Exception("Validation not passed!")  

        #if save:
        if True: #mandatory save for importing in polarfly.cc
            filename    = self.getFileName()
            folderpath  = self.getFolderPath()
            self.save(folderpath, filename)



    def save(self, folderpath, filename):

        if not os.path.exists(folderpath):
            os.makedirs(folderpath)


        file    = folderpath + filename
        with open(file+".txt", "w") as f:
            print(len(self.topo), sum(len(n) for n in self.topo)//2, file=f)
            for node in self.topo:
                print( " ".join(str(e) for e in node) + " ", file=f)




    #derive the degree distribution between supenrode (joiner) and structure (PF) graphs with largest config
    def optPSConfig(self, d, sg):
        scale   = 0
        sn_type = "iq"
        strq    = 0
        snq     = 0
        pf_max  = 0
        sn_max  = 0
        #sweep over all possible degrees of PolarFly structure graph
        for pfd in range(3, d+1):
            snd     = d - pfd
            pfq     = pfd - 1
            pfV     = pfq*pfq + pfq + 1
            if (not isPowerOfPrime(pfq)):
                continue 
    
            #analyze paley supernode
            paleyq  = snd*2 + 1
            if (sg != "iq" and isPowerOfPrime(paleyq) and (paleyq%4 == 1)):
                paleyV  = paleyq
                V       = paleyV*pfV
                if (V > scale):
                    scale   = V
                    sn_type = "paley"
                    strq    = pfq
                    snq     = paleyq
                    pf_max  = pfV
                    sn_max  = paleyV

            #analyze IQ supernode
            iqq     = snd
            if (sg != "paley" and ((iqq % 4 == 0) or (iqq % 4 == 3))):
                iqV    = 2*iqq + 2 
                V       = iqV*pfV
                if (V > scale):
                    scale   = V
                    sn_type = "iq"
                    strq    = pfq
                    snq     = iqq
                    pf_max  = pfV
                    sn_max  = iqV

        if (strq==0 and snq==0):
            raise Exception("Infeasible configuration")
    
        print("max PolarStar scale at degree " + str(d) + " is " + str(scale) + " vertices")
        print("Supernode Type = " + sn_type + ", Supernode q = " + str(snq) + ", PolarFly q = " + str(strq))
        print("SuperNodeSize = " + str(sn_max))
        print("Num SuperNodes: " + str(pf_max))
    
        return strq, snq, sn_type


    #compute star product of supernode (joiner) with structure (polarfly)
    def starProd(self, pfq, snq, sn):
        snG         = Paley(snq) if (sn=="paley") else IQ(snq)
        phi, adj_sn = snG.make()
        snV         = len(adj_sn)

        pfG         = topoPolarFly(q=pfq) 
        adj_pf      = pfG.make()
        pfV         = len(adj_pf)    

        #add self-loops to PF quadrics
        num_quads   = 0 
        for i in range(pfV):
            is_quad = (len(adj_pf[i]) == pfq)
            if (is_quad):
                adj_pf[i].append(i)
                num_quads += 1
        assert(num_quads == pfq+1)

        psV         = pfV*snV
        assert(psV == self.total_routers)
        adj_ps      = [[] for _ in range(psV)]

        #add intra-supernode edges
        for i in range(pfV):
            for j in range(snV):
                u   = i*snV + j
                for k in adj_sn[j]:
                    v   = i*snV + k                 
                    adj_ps[u].append(v)

        #add inter-supernode edges
        for i in range(pfV):
            cov = [False for _ in range(snV)]
            for j in adj_pf[i]:
                for u_off in range(snV):
                    u   = i*snV + u_off
                    v   = j*snV + phi[u_off]
                    #add edge (i,u) -> (j,phi[u]) if i<j
                    #if (i==j), add at most one edge corresponding to the self loop
                    if ((i < j) or ((i==j) and (not cov[u_off]) and (not cov[phi[u_off]]) and u!=v)):
                        adj_ps[u].append(v)
                        adj_ps[v].append(u)
                        if (i==j):
                            cov[u_off]      = True
                            cov[phi[u_off]] = True

        for u in range(psV):
            adj_ps[u].sort()

        return adj_ps
    
    
    def build(self, endpoint):

        if self._check_first_build():
            sst.addGlobalParams("params_%s"%self._instance_name, self._getGroupParams("main"))

        if self.host_link_latency is None: 
            self.host_link_latency  = self.link_latency

        links   = dict()
        def getLink(name1, name2):
            # Sort name1 and name2 so order doesn't matter
            if str(name1) < str(name2):
                name = "link_%s_%s"%(name1, name2)
            else:
                name = "link_%s_%s"%(name2, name1)
            if name not in links:
                links[name] = sst.Link(name)
            return links[name]
            
        #1. Generate the adjacency list for the polarstar topology
        nxFound     = False
        if 'networkx' in sys.modules:
            nxFound = True
        self.generate(validate=nxFound, save=True)
                           
        node_num    = 0
        #2. Iterate over each router
        for router in range(self.total_routers):

            #3. First instantiate a high radix router component
            rtr = self._instanceRouter(self.total_radix,router)

            topology = rtr.setSubComponent(self.router.getTopologySlotName(),"merlin.polarstar")
            self._applyStatisticsSettings(topology)
            topology.addParams(self._getGroupParams("main"))

            port = 0

            #3. Then connect the hosts_per_router endpoints to each router
            for localnodeID in range(self.hosts_per_router):

                nodeID = router*(self.hosts_per_router)+localnodeID
                (ep, port_name) = endpoint.build(nodeID, {})

                node_num = node_num+1
                if ep:
                    nicLink = sst.Link("nic_%d_%d"%(router, localnodeID))
                    if self.bundleEndpoints:
                       nicLink.setNoCut()
                    nicLink.connect( (ep, port_name, self.host_link_latency), (rtr, "port%d"%port, self.host_link_latency) )
                port = port+1
                node_num = node_num+1
            
            #4. Now connect the remaining ports to the neighbors
            neighbor_list = self.topo[router]
            for neighbor in neighbor_list:
                rtr.addLink(getLink(router,neighbor),"port%d"%port,self.link_latency)
                port = port+1

            #5. Check if the total ports being used are less than the radix of the router.
            # For some routers, you will see there could be empty ports
            assert(port <= self.total_radix)








