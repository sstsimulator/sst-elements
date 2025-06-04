#!/usr/bin/env python

# Copyright 2023 Intel Corporation
# SPDX-License-Identifier: BSD-3-Clause

# Authors: Kartik Lakhotia, Sai Prabhakar Rao Chenna


import sst
from sst.merlin.base import *


from os import path, makedirs

import os
import sys
import itertools

try:
    from sympy.polys.domains import ZZ
except ImportError:
    pass
try:
    from sympy.polys.galoistools import (gf_irreducible_p, gf_add, gf_mul, gf_rem)
except ImportError:
    pass
try:
    import networkx as nx
except ImportError:
    pass


#############################################
######### FINITE FIELD GENERATOR ############
#############################################
class GF():

    def __init__(self, pp):

        if not ('sympy.polys.galoistools' in sys.modules):
            raise ImportError('--> Did not load Sympy Polys Galoistools : module required for field construction')

        if not ('sympy.polys.domains' in sys.modules):
            raise ImportError('--> Did not load Sympy Polys Domains : module required for field construction')


        self.pp = int(pp)
        p, n    = self.factor() 

        if not self.isPrime(p):
            raise ValueError("p must be a prime number, not %s" % p)
        if n <= 0:
            raise ValueError("n must be a positive integer, not %s" % n)
        self.p = p
        self.n = n
        if n == 1:
            self.reducing = (1, 0)
        else:
            for c in itertools.product(range(p), repeat=n):
                poly = (1, *c)
                if gf_irreducible_p(poly, p, ZZ):
                    self.reducing = poly
                    break

        self.elems  = [i for i in range(self.pp)]
        self.polys  = [self.computeCoeffs(i) for i in range(self.pp)]
        
        self.add_mat, self.mul_mat, self.addI_mat, self.mulI_mat    = self.fieldGen() 


    def getElems(self):
        return self.elems

    def getFactors(self):
        return self.p, self.n

    
    def isPrime(self, p):
        if (p <= 1):
            return False
        for i in range(2, p):
            if (p%i == 0):
                return False
        return True


    def factor(self):
        n               = int(1) 
        p               = self.pp
        tmp             = self.pp

        #prime power should be greater than 1
        if (tmp <= 1):
            print("pp must be a prime power greater than 1")
            exit(0)

        for i in range(2,self.pp):
            if ((self.pp%i)==0):
                p       = i
                tmp     = tmp//i
                while((tmp%i) == 0):
                    n   += 1
                    tmp = tmp//i
                #if pp is primepower, tmp must be 1
                if(tmp != 1):
                    print("pp is not a prime power")
                    exit(0)
            if (tmp==1):
                break
        return p, n 


    def computeCoeffs(self, i):
        coeffs = [0 for j in range(self.n)]
        tmp = int(i)
        cid = 0
        while(tmp > 0):
            coeffs[self.n-1-cid] = tmp%self.p
            cid += 1
            tmp = int(tmp/self.p)
        return coeffs
    

    def computeIndex(self, coeffs):
        index = 0
        if (len(coeffs) > self.n):
            coeffs  = gf_rem(coeffs, self.reducing, self.p, ZZ)
        for i in range(len(coeffs)):
            index = index*self.p + coeffs[i]
        assert(index < self.pp)
        return index


    def fieldGen(self):
        add_mat = [[0]*self.pp for i in range(self.pp)]
        mul_mat = [[0]*self.pp for i in range(self.pp)]
        addI_mat= [0 for i in range(self.pp)]
        mulI_mat= [1 for i in range(self.pp)]       

        #Create finite field matrices
        for i in range(self.pp):
            poly_i  = self.computeCoeffs(i)
            for j in range(self.pp):
                x               = self.polys[i]
                y               = self.polys[j]
                add_mat[i][j]   = self.computeIndex(gf_add(x, y, self.p, ZZ))
                mul_mat[i][j]   = self.computeIndex(gf_rem(gf_mul(x, y, self.p, ZZ), self.reducing, self.p, ZZ))
                if (add_mat[i][j] == 0):
                    addI_mat[i] = j
                if (mul_mat[i][j] == 1):
                    mulI_mat[i] = j

        #Validate finite field matrices
        uniqAI  = [0 for i in range(self.pp)] #inverses should be unique
        uniqMI  = [0 for i in range(self.pp)]
        for i in range(self.pp):
            assert(add_mat[0][i] == i)
            assert(mul_mat[0][i] == 0)
            assert(add_mat[i][addI_mat[i]] == 0)
            assert(uniqAI[addI_mat[i]] == 0)
            uniqAI[addI_mat[i]] += 1
            if (i>0):
                assert(mul_mat[i][mulI_mat[i]] == 1)
                assert(uniqMI[mulI_mat[i]] == 0)
                uniqMI[mulI_mat[i]] += 1
            for j in range(self.pp):
                assert(add_mat[i][j] == add_mat[j][i])
                assert(mul_mat[i][j] == mul_mat[j][i])

        return add_mat, mul_mat, addI_mat, mulI_mat

    def add(self, x, y):
        return self.add_mat[x][y] 

    def addInv(self, x):
        return self.addI_mat[x]

    #return x-y
    def sub(self, x, y):
        return self.add_mat[x][self.addI_mat[y]]

    def mul(self, x, y):
        return self.mul_mat[x][y]
    
    def mulInv(self, x):
        return self.mulI_mat[x]

    #return x/y
    def div(self, x, y):
        assert(y != 0)
        return self.mul_mat[x][self.mulI_mat[y]]


    def getPrimitiveElem(self):
        for i in range(self.pp):
            if self.isPrimitiveElem(i):
                return i

    def getAllPrimitiveElems(self):
        return [i for i in self.pp if self.isPrimitiveElem(i)]

    def isPrimitiveElem(self, primitive) -> bool:
        elems = [0]
        tmp = 1 
        for _ in range(1,self.pp):
            tmp = self.mul(tmp,primitive)
            elems.append(tmp)

        if all(elem in elems for elem in self.elems):
            return True
        else:
            return False
#############################################
#############################################


class topoPolarFly(Topology):

    """
    Parameters:
        q       : prime power < 256, degree = q+1
        N       : number of routers
    """
    def __init__(self,q = -1, N = -1):
        Topology.__init__(self)
        self._declareClassVariables(["link_latency","host_link_latency","global_link_map","bundleEndpoints"])
        self._declareParams("main",["topo","q","hosts_per_router","network_radix","total_radix","total_routers",
                                    "total_endnodes","edge","name","algorithm","adaptive_threshold","global_routes","config_failed_links",
                                    "failed_links", "GF", "vec_len"])
        self.global_routes = "absolute"
        self._subscribeToPlatformParamSet("topology")
        
        #Set the q value (size of Gallois field) and throw an error if it is not from the q candidates list

        """
        Parameters:
            q: size of Gallois Field
        """
        q_candidates = [2, 3, 4, 5, 7, 8, 9, 11, 13, 16, 17, 19, 23, 25, 27, 29, 31, 32, 37, 41, 43, 47, 49, 
                        53, 59, 61, 64, 67, 71, 73, 79, 81, 83, 89, 97, 101, 103, 107, 109, 113, 121, 125, 127, 
                        128, 131, 137, 139, 149, 151, 157, 163, 167, 169, 173, 179, 181, 191, 193, 197, 199, 211, 
                        223, 227, 229, 233, 239, 241, 243, 251]

        if(q == -1 and N != -1):
            if N >= 65000:
                self.q = 251
            else:
                desired_q = sqrt(N) 
                self.q = q_candidates[min(range(len(q_candidates)), key = lambda i: abs(q_candidates[i]-desired_q))]
        elif(q != -1 and N == -1):
            if (q in q_candidates):
                self.q = q
            else:
                raise Exception("Polarfly: specified q is not a prime power less than 256")
        else:
            raise Exception("Polarfly: invalid combination of arguments in constructor")

        #Set Shape
        self.network_radix      = self.q+1
        self.total_routers      = (self.q**2+ self.q + 1)
        self.edge               = (self.q*(self.q+1)*(self.q+1))//2 
        self.name               = 'PolarFly'
        self.topo               = None
        self.hosts_per_router   = None
        self.total_radix        = None 
        self.total_endnodes     = None 

        #For Erdos-Renyi graph construction
        self.GF                 = None
        self.vec_len            = 3


    def setEP(self):
        if (self.hosts_per_router is None):
            self.hosts_per_router   = self.network_radix//2 
        self.total_radix            = self.hosts_per_router + self.network_radix
        self.total_endnodes         = self.hosts_per_router*self.total_routers
        


    def getName(self):
        return self.name


    def get_info(self):
        return "name=%s p=%d net-radix=%d radix=%d R=%d N=%d" %(self.name, self.host_link_latency, self.network_radix, self.total_radix, self.total_routers, self.total_endnodes)


    def getFileName(self):
        return "PolarFly.q_" + str(self.q)


    def getNumNodes(self):
        if (self.total_endnodes is None):
            self.setEP()
        return self.total_endnodes


    def getNumEdges(self):
        if self.topo == None:
            self.make()
        return int(sum(len(n) for n in self.topo) / 2)

        
    def getFolderPath(self):
        return os.getcwd()+"/polarfly_data/"


    #return v1.v2
    def ERVecDP(self, v1, v2):
        assert(len(v1)==self.vec_len)
        assert(len(v2)==self.vec_len)
        dp  = int(0)
        for i in range(self.vec_len):
            dp  = self.GF.add(dp, self.GF.mul(v1[i], v2[i])) 
        return dp


    def make(self):
        if self.topo is None:
            self.GF     = GF(self.q)
            q           = self.q
            V           = self.total_routers
            node_map    = {}  
            vecs        = []
            graph       = [[] for _ in range(V)]

            # get all 1-d subspace vectors of PG(2,q)
            for d1 in range(q):
                for d2 in range(q):
                    v = (d1,d2,1)
                    vecs.append(v)
            for d1 in range(q):
                v = (d1, 1, 0)
                vecs.append(v)
            v = (1, 0, 0)
            vecs.append(v)

            for idx, v in enumerate(vecs):
                node_map[v] = idx

            # add connection between 1-d subspace vectors
            for idx,v in enumerate(vecs):
                src = idx
                for vv in vecs:
                    dp = self.ERVecDP(v, vv)
                    if (dp == 0):
                        dst = node_map[vv]
                        if dst != src:
                            graph[src].append(dst)

            self.topo   = graph
            
        return self.topo


    def validate(self):
        print("--> Validating Polarfly(%d):" %self.q)
        # check sizes
        if len(self.topo) != self.q**2 + self.q + 1:
            print("     --> construction error: incorrect number of nodes")
            return 0

        # construct a nx graph (undirected, no multi-edges) for further validation
        nx_graph = nx.Graph()
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
        if max_degree != self.q+1:
             print("     --> construction error: incorrect degrees")
             return 0

        #check diameter
        diameter = nx.diameter(nx_graph)
        if diameter != 2:
            print("     --> construction error: incorrect diameter")
            return 0

        return 1


    def generate(self, validate=False, save=False):
        print("----> Generating Polarfly topology!!")
        self.make()
        self.setEP()

        if validate:
            if not self.validate():
                raise Exception("Validation not passed!")

        if save:
            filename = self.getFileName()
            folderpath = self.getFolderPath()
            self.save(folderpath,filename)



    def save(self, folderpath, filename):
        if not path.exists(folderpath):
            makedirs(folderpath)
        
        file = folderpath + filename

        with open(file+".txt", "w") as f:
            print(len(self.topo), int(sum(len(n) for n in self.topo) / 2), file=f)
            for node in self.topo:
                print( " ".join(str(e) for e in node) + " ", file=f)
        

    def _build_impl(self, endpoint):
        if self._check_first_build():
            sst.addGlobalParams("params_%s"%self._instance_name, self._getGroupParams("main"))

        if self.host_link_latency is None:
            self.host_link_latency = self.link_latency

        links = dict()
        def getLink(name1, name2):
            # Sort name1 and name2 so order doesn't matter
            if str(name1) < str(name2):
                name = "link_%s_%s"%(name1, name2)
            else:
                name = "link_%s_%s"%(name2, name1)
            if name not in links:
                links[name] = sst.Link(name)
            return links[name]

        #1. Generate the adjacency list for the polarfly topology
        nxFound     = False
        if 'networkx' in sys.modules:
            nxFound = True
        self.generate(validate=nxFound, save=True)

        node_num = 0
        #2. Iterate over each router
        for router in range(self.total_routers):
            #3. First instantiate a high radix router component
            rtr = self._instanceRouter(self.total_radix,router)

            topology = rtr.setSubComponent(self.router.getTopologySlotName(),"merlin.polarfly")
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
            # For some routers, you will see there would be empty ports
            assert(port < self.total_radix+1)





