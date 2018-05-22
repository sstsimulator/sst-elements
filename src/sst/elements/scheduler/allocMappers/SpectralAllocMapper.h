// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SPECTRALALLOCMAPPER_H_
#define SPECTRALALLOCMAPPER_H_

#include "AllocMapper.h"

#include "sst/core/rng/mersenne.h"

#include <map>
#include <vector>

using namespace std;

namespace SST {
    namespace Scheduler {

    class Machine;

    //Spectral Mapping algorithm is based on the paper
    // Leordeanu, M.; Hebert, M., "A spectral technique for correspondence problems using pairwise
    // constraints," Computer Vision, 2005. ICCV 2005. Tenth IEEE International Conference on ,
    // vol.2, no., pp.1482,1489 Vol. 2, 17-21 Oct. 2005 doi: 10.1109/ICCV.2005.20

    class SpectralAllocMapper : public AllocMapper {

        public:
            SpectralAllocMapper(const Machine & mach, bool alloacateAndMap , int rngSeed = -1);
            ~SpectralAllocMapper();

            string getSetupInfo(bool comment) const;

            //allocation & mapping function
            void allocMap(const AllocInfo & ai,
                          std::vector<long int> & usedNodes,
                          std::vector<int> & taskToNode);

        private:
            //SST::RNG::MersenneRNG randNG;   //random number generator
            vector<pair<unsigned int, unsigned long int> > possPairs; //possible pairs
            list<unsigned long int> pairIndices; //indices of available pairs - for optimization
            vector<map<int,int> >* commMatrix;

            //returns the approximate principle eigenvector for the given tasks & nodes
            //uses Power (Von Mises) iteration
            //matrix should be symmetric & positive
            vector<double>* principalEigenVector(const unsigned int maxIteration = 100,
                                                 const double epsilon = 5e-3) const; //error margin
            //O(inVector.size()^2)
            //multiply with M matrix - refer to paper for definition
            vector<double>* multWithM(const vector<double> & inVector) const;
            void normalize(vector<double> & inVector) const;
        };

    }
}

#endif /* SPECTRALALLOCMAPPER_H_ */

