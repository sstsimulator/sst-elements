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


#ifndef _H_EMBER_3D_AMR_BLOCK
#define _H_EMBER_3D_AMR_BLOCK

#include <sst/core/component.h>
#include <sst/core/output.h>

using namespace SST;

namespace SST {
namespace Ember {

class Ember3DAMRBlock {
        public:
                Ember3DAMRBlock(
                        const uint32_t id,
                        const uint32_t refLevel,
                        const int32_t ref_x_down,
                        const int32_t ref_x_up,
                        const int32_t ref_y_down,
                        const int32_t ref_y_up,
                        const int32_t ref_z_down,
                        const int32_t ref_z_up
                         ) :

                        blockID(id)
			{

			// Set local copy of our variables
			refinementLevel = (uint8_t) refLevel;
			refine_x_up   = (int8_t) ref_x_up;
			refine_x_down = (int8_t) ref_x_down;
			refine_y_up   = (int8_t) ref_y_up;
			refine_y_down = (int8_t) ref_y_down;
			refine_z_up   = (int8_t) ref_z_up;
			refine_z_down = (int8_t) ref_z_down;

			// Maintain list of block communication ranks
                        commXUp   = (int32_t*) malloc(sizeof(int32_t) * 4);
                        commXDown = (int32_t*) malloc(sizeof(int32_t) * 4);
                        commYUp   = (int32_t*) malloc(sizeof(int32_t) * 4);
                        commYDown = (int32_t*) malloc(sizeof(int32_t) * 4);
                        commZUp   = (int32_t*) malloc(sizeof(int32_t) * 4);
                        commZDown = (int32_t*) malloc(sizeof(int32_t) * 4);

			for(int i = 0; i < 4; ++i) {
				commXUp[i]   = -1;
				commXDown[i] = -1;
				commYUp[i]   = -1;
				commYDown[i] = -1;
				commZUp[i]   = -1;
				commZDown[i] = -1;
			}
                }

                ~Ember3DAMRBlock() {
                        free(commXUp);
                        free(commXDown);
                        free(commYUp);
                        free(commYDown);
                        free(commZUp);
                        free(commZDown);
                }

                uint32_t getRefinementLevel() const {
                        return (uint32_t) refinementLevel;
                }

                uint32_t getBlockID() const {
                        return blockID;
                }

		int32_t getRefineXUp() const {
                        return (int32_t) refine_x_up;
                }

                int32_t getRefineXDown() const {
                        return (int32_t) refine_x_down;
                }

                int32_t getRefineYUp() const {
                        return (int32_t) refine_y_up;
                }

                int32_t getRefineYDown() const {
                        return (int32_t) refine_y_down;
                }

                int32_t getRefineZUp() const {
                        return (int32_t) refine_z_up;
                }

                int32_t getRefineZDown() const {
                        return (int32_t) refine_z_down;
                }

                int32_t* getCommXUp() const {
                        return commXUp;
                }

                int32_t* getCommXDown() const {
                        return commXDown;
                }

                int32_t* getCommYUp() const {
                        return commYUp;
                }

                int32_t* getCommYDown() const {
                        return commYDown;
                }

                int32_t* getCommZUp() const {
                        return commZUp;
                }

                int32_t* getCommZDown() const {
                        return commZDown;
                }

                void setCommXUp(const int32_t x1, const int32_t x2, const int32_t x3, const int32_t x4) {
                        commXUp[0] = x1;
                        commXUp[1] = x2;
                        commXUp[2] = x3;
                        commXUp[3] = x4;
                }

                void setCommXDown(const int32_t x1, const int32_t x2, const int32_t x3, const int32_t x4) {
                        commXDown[0] = x1;
                        commXDown[1] = x2;
                        commXDown[2] = x3;
                        commXDown[3] = x4;
                }

                void setCommYUp(const int32_t x1, const int32_t x2, const int32_t x3, const int32_t x4) {
                        commYUp[0] = x1;
                        commYUp[1] = x2;
                        commYUp[2] = x3;
                        commYUp[3] = x4;
                }

                void setCommYDown(const int32_t x1, const int32_t x2, const int32_t x3, const int32_t x4) {
                        commYDown[0] = x1;
                        commYDown[1] = x2;
                        commYDown[2] = x3;
                        commYDown[3] = x4;
                }

                void setCommZUp(const int32_t x1, const int32_t x2, const int32_t x3, const int32_t x4) {
                        commZUp[0] = x1;
                        commZUp[1] = x2;
                        commZUp[2] = x3;
                        commZUp[3] = x4;
                }

                void setCommZDown(const int32_t x1, const int32_t x2, const int32_t x3, const int32_t x4) {
                        commZDown[0] = x1;
                        commZDown[1] = x2;
                        commZDown[2] = x3;
                        commZDown[3] = x4;
                }

		void print() {
			printf("BlockID %" PRIu32 " @ Level: %" PRIu8 "\n", blockID, refinementLevel);
			printf("Refine X up:   %7" PRId8 ", X={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%" PRId32 "}\n",
				refine_x_up, commXUp[0], commXUp[1], commXUp[2], commXUp[3]);
			printf("Refine X down: %7" PRId8 ", X={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%" PRId32 "}\n",
				refine_x_down, commXDown[0], commXDown[1], commXDown[2], commXDown[3]);
			printf("Refine Y up:   %7" PRId8 ", X={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%" PRId32 "}\n",
				refine_y_up, commYUp[0], commYUp[1], commYUp[2], commYUp[3]);
			printf("Refine Y down: %7" PRId8 ", X={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%" PRId32 "}\n",
				refine_y_down, commYDown[0], commYDown[1], commYDown[2], commYDown[3]);
			printf("Refine Z up:   %7" PRId8 ", X={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%" PRId32 "}\n",
				refine_z_up, commZUp[0], commZUp[1], commZUp[2], commZUp[3]);
			printf("Refine Z down: %" PRId8 ", X={ %" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 "}\n",
				refine_z_down, commZDown[0], commZDown[1], commZDown[2], commZDown[3]);
		}

		void printFile(FILE* f) {
			fprintf(f, "Block ID %" PRIu32 " @ Level: %" PRIu32 "\n", blockID, refinementLevel);
			fprintf(f, "- Refine X up:   Refinement: %7" PRId32 ", Comm={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%7" PRId32 "}\n",
				refine_x_up, commXUp[0], commXUp[1], commXUp[2], commXUp[3]);
			fprintf(f, "- Refine X down: Refinement: %7" PRId32 ", Comm={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%7" PRId32 "}\n",
				refine_x_down, commXDown[0], commXDown[1], commXDown[2], commXDown[3]);
			fprintf(f, "- Refine Y up:   Refinement: %7" PRId32 ", Comm={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%7" PRId32 "}\n",
				refine_y_up, commYUp[0], commYUp[1], commYUp[2], commYUp[3]);
			fprintf(f, "- Refine Y down: Refinement: %7" PRId32 ", Comm={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%7" PRId32 "}\n",
				refine_y_down, commYDown[0], commYDown[1], commYDown[2], commYDown[3]);
			fprintf(f, "- Refine Z up:   Refinement: %7" PRId32 ", Comm={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%7" PRId32 "}\n",
				refine_z_up, commZUp[0], commZUp[1], commZUp[2], commZUp[3]);
			fprintf(f, "- Refine Z down: Refinement: %7" PRId32 ", Comm={ %7" PRId32 ",%7" PRId32 ",%7" PRId32 ",%7" PRId32 "}\n",
				refine_z_down, commZDown[0], commZDown[1], commZDown[2], commZDown[3]);
		}

        private:
                uint32_t blockID;
                uint8_t refinementLevel;

                int8_t refine_x_up;
                int8_t refine_x_down;
                int8_t refine_y_up;
                int8_t refine_y_down;
                int8_t refine_z_up;
                int8_t refine_z_down;

                int32_t* commXUp;
                int32_t* commXDown;
                int32_t* commYUp;
                int32_t* commYDown;
                int32_t* commZUp;
                int32_t* commZDown;
	};
}
}

#endif
