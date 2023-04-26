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


# Author: Kartik Lakhotia

import sys
import itertools
try:
    from sympy.polys.domains import ZZ
except:
    print('--> MODULE NOT FOUND: sympy.polys.domains')
try:
    from sympy.polys.galoistools import (gf_irreducible_p, gf_add, gf_mul, gf_rem)
except:
    print('--> MODULE NOT FOUND: sympy.polys.galoistools')


def isPowerOfPrime(num):
    if (num<2):
        return False

    fact    = 2
    while(fact < num):
        rem = num%fact
        if (rem == 0):
            break
        else:
            fact += 1

    tmp     = num
    while(tmp>fact):
        if ((tmp%fact) > 0):
            return False
        else:
            tmp = tmp/fact
    return True



class GF():

    def __init__(self, pp):

        if not ('sympy.polys.galoistools' in sys.modules):
            raise Exception('--> Did not load Sympy Polys Galoistools : module required for field construction')

        if not ('sympy.polys.domains' in sys.modules):
            raise Exception('--> Did not load Sympy Polys Domains : module required for field construction')


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

