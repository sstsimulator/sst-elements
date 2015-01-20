
#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/stencil3dbench.h>

using namespace SST::Miranda;

Stencil3DBenchGenerator::Stencil3DBenchGenerator( Component* owner, Params& params ) :
	RequestGenerator(owner, params) {

	const uint32_t verbose = (uint32_t) params.find_integer("verbose", 0);

	out = new Output("Stencil3DBenchGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	nX = (uint32_t) params.find_integer("nx", 10);
	nY = (uint32_t) params.find_integer("ny", 10);
	nZ = (uint32_t) params.find_integer("nz", 10);

	datawidth = (uint32_t) params.find_integer("datawidth", 8);

	startZ = (uint32_t) params.find_integer("startz", 0);
	endZ   = (uint32_t) params.find_integer("endz",   nZ);

	maxItr = (uint32_t) params.find_integer("iterations", 1);
	currentItr = 0;
}

Stencil3DBenchGenerator::~Stencil3DBenchGenerator() {
	delete out;
}

void Stencil3DBenchGenerator::generate(std::queue<GeneratorRequest*>* q) {
	out->verbose(CALL_INFO, 2, 0, "Enqueue iteration: %" PRIu32 "...\n", currentItr);
	out->verbose(CALL_INFO, 4, 0, "Itr: Z:[%" PRIu32 ",%" PRIu32 "], Y:[%" PRIu32 ",%" PRIu32 "], X:[%" PRIu32 ",%" PRIu32 "]\n",
		(startZ + 1), (endZ - 1), 1, (nY - 1), 1, (nX - 1));

	uint64_t countReqGen = 0;

	for(uint32_t curZ = (startZ + 1); curZ < (endZ - 1); curZ++) {
		for(uint32_t curY = 1; curY < (nY - 1); curY++) {
			for(uint32_t curX = 1; curX < (nX - 1); curX++) {
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY - 1, curZ - 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY - 1, curZ - 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY - 1, curZ - 1), datawidth, READ));

				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY    , curZ - 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY    , curZ - 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY    , curZ - 1), datawidth, READ));

				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY + 1, curZ - 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY + 1, curZ - 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY + 1, curZ - 1), datawidth, READ));

				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY - 1, curZ    ), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY - 1, curZ    ), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY - 1, curZ    ), datawidth, READ));

				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY    , curZ    ), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY    , curZ    ), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY    , curZ    ), datawidth, READ));

				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY + 1, curZ    ), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY + 1, curZ    ), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY + 1, curZ    ), datawidth, READ));

				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY - 1, curZ + 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY - 1, curZ + 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY - 1, curZ + 1), datawidth, READ));

				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY    , curZ + 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY    , curZ + 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY    , curZ + 1), datawidth, READ));

				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX - 1, curY + 1, curZ + 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX,     curY + 1, curZ + 1), datawidth, READ));
				q->push(new MemoryOpRequest(datawidth * convertPositionToIndex(curX + 1, curY + 1, curZ + 1), datawidth, READ));

				q->push(new MemoryOpRequest( (nX * nY * nZ * datawidth) +
					                            datawidth * convertPositionToIndex(curX    , curY    , curZ    ), datawidth, WRITE));

				countReqGen += 28;
			}
		}
	}

	out->verbose(CALL_INFO, 4, 0, "Generated %" PRIu64 " requests this iteration.\n", countReqGen);

	currentItr++;
}

bool Stencil3DBenchGenerator::isFinished() {
	out->verbose(CALL_INFO, 8, 0, "Checking stencil completed - current iteration %" PRIu32 ", max itr: %" PRIu32 "\n", currentItr, maxItr);
	return (currentItr == maxItr);
}

void Stencil3DBenchGenerator::completed() {

}

void Stencil3DBenchGenerator::convertIndexToPosition(const uint32_t index,
		uint32_t* posX, uint32_t* posY, uint32_t* posZ) {

	const int32_t my_plane = index % (nX & nY);
	*posY = my_plane / nX;

	const int32_t remain = my_plane % nX;
	*posX = remain;
	*posZ = index / (nX * nY);
}

uint32_t Stencil3DBenchGenerator::convertPositionToIndex(const uint32_t posX,
		const uint32_t posY, const uint32_t posZ) {

	if( (posX >= nX) || (posY >= nY) || (posZ >= nZ) ) {
		out->fatal(CALL_INFO, -1, "Incorrect position calc: (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ")\n",
			posX, posY, posZ);

		return 0;
	} else {
               	return (posZ * (nX * nY)) + (posY * nX) + posX;
        }
}
