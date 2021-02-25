// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <map>

void usage() {
	printf("Usage: meshconverter <number ranks> <file in> <file out>\n");
	printf("<no ranks>       Is the number of ranks represented by the mesh\n");
	printf("<file in>        Is the input mesh definition in text\n");
	printf("<file out>       Is the output mesh definition to be written in binary\n");
	exit(-1);
}

char* readLine(FILE* input) {
	char* theLine = (char*) malloc(sizeof(char) * 256);

	int nextIndex = 0;
	char nextChar;

	while( (nextChar = fgetc(input)) != '\n' &&
		(nextChar != EOF) ) {

		theLine[nextIndex] = nextChar;
		nextIndex++;
	}

	theLine[nextIndex] = '\0';
	return theLine;
}

void readNextMeshLine(FILE* mesh, uint32_t* blockID, uint32_t* refineLev,
                int8_t* xDown, int8_t* xUp,
                int8_t* yDown, int8_t* yUp,
                int8_t* zDown, int8_t* zUp) {

        char* nextLine = readLine(mesh);

        char* blockIDStr     = strtok(nextLine, " ");
        char* refineLevelStr = strtok(NULL, " ");
        char* xDStr          = strtok(NULL, " ");
        char* xUStr          = strtok(NULL, " ");
        char* yDStr          = strtok(NULL, " ");
        char* yUStr          = strtok(NULL, " ");
        char* zDStr          = strtok(NULL, " ");
        char* zUStr          = strtok(NULL, " ");

        *blockID   = (uint32_t) atoi(blockIDStr);
        *refineLev = (uint32_t) atoi(refineLevelStr);
        *xDown     = (int8_t) atoi(xDStr);
        *xUp       = (int8_t) atoi(xUStr);
        *yDown     = (int8_t) atoi(yDStr);
        *yUp       = (int8_t) atoi(yUStr);
        *zDown     = (int8_t) atoi(zDStr);
        *zUp       = (int8_t) atoi(zUStr);

	free(nextLine);
}

int main(int argc, char* argv[]) {
	printf("SST MiniAMR Mesh Converter\n");

	if(argc < 4) {
		usage();
	}

	std::map<uint32_t, uint64_t> rankIndexMap;
	uint32_t rankCount = (uint32_t) atoi(argv[1]);

	FILE* inMesh = fopen(argv[2], "rt");
	if(NULL == inMesh) {
		fprintf(stderr, "Unable to open input mesh: %s\n", argv[2]);
		exit(-1);
	}

	FILE* outMesh = fopen(argv[3], "wb");
	if(NULL == outMesh) {
		fprintf(stderr, "Unable to open output mesh: %s\n", argv[3]);
		exit(-1);
	}

	char* meshHeader = readLine(inMesh);

	char* blockCountStr  = strtok(meshHeader, " ");
        char* refineLevelStr = strtok(NULL, " ");
        char* blocksXStr     = strtok(NULL, " ");
        char* blocksYStr     = strtok(NULL, " ");
        char* blocksZStr     = strtok(NULL, " ");

	uint32_t blockCount         = (uint32_t) atoi(blockCountStr);
	uint8_t maxRefinementLevel  = (uint8_t)  atoi(refineLevelStr);
	uint32_t blocksX            = (uint32_t) atoi(blocksXStr);
	uint32_t blocksY            = (uint32_t) atoi(blocksYStr);
	uint32_t blocksZ            = (uint32_t) atoi(blocksZStr);

	fwrite(&rankCount, sizeof(rankCount), 1, outMesh);
	fwrite(&blockCount, sizeof(blockCount), 1, outMesh);
	fwrite(&maxRefinementLevel, sizeof(maxRefinementLevel), 1, outMesh);
	fwrite(&blocksX, sizeof(blocksX), 1, outMesh);
	fwrite(&blocksY, sizeof(blocksY), 1, outMesh);
	fwrite(&blocksZ, sizeof(blocksZ), 1, outMesh);

	const size_t  rankIndexOffset = ftell(outMesh);
	const uint64_t defaultIndex = (uint64_t) -1;
	uint64_t nextFileIndex = rankIndexOffset;

	// Output the initial rank start index, all set to (uint64_t) -1
	for(uint32_t i = 0; i < rankCount; i++) {
		fwrite(&defaultIndex, sizeof(defaultIndex), 1, outMesh);
		nextFileIndex += sizeof(defaultIndex);
	}

	printf("Generating block information at index: %" PRIu64 "\n",
		(uint64_t) ftell(outMesh));

	uint32_t blocksOnNode = 0;
	uint32_t nextBlockID = 0;
	uint32_t nextBlockRefineLevel = 0;
	int8_t  blockXDown = 0;
	int8_t  blockXUp = 0;
	int8_t  blockYDown = 0;
	int8_t  blockYUp = 0;
	int8_t  blockZDown = 0;
	int8_t  blockZUp = 0;

	for(uint32_t i = 0; i < rankCount; i++) {
		printf("Processing rank %" PRIu32 "...\n", i);

		char* strBlocksOnNode = readLine(inMesh);
		blocksOnNode = (uint32_t) atoi(strBlocksOnNode);

		// Update the rank index
		fseek(outMesh, rankIndexOffset + (i * sizeof(uint64_t)), SEEK_SET);
		fwrite(&nextFileIndex, sizeof(nextFileIndex), 1, outMesh);

		// Return back to original position
		fseek(outMesh, nextFileIndex, SEEK_SET);

		fwrite(&blocksOnNode, sizeof(blocksOnNode), 1, outMesh);
		nextFileIndex += sizeof(blocksOnNode);

		for(uint32_t j = 0; j < blocksOnNode; j++) {
			readNextMeshLine(inMesh, &nextBlockID,
				&nextBlockRefineLevel,
				&blockXDown,
				&blockXUp,
				&blockYDown,
				&blockYUp,
				&blockZDown,
				&blockZUp);

			fwrite(&nextBlockID, sizeof(nextBlockID), 1, outMesh);
			nextFileIndex += sizeof(nextBlockID);

			int8_t nextBlockRefineLevel_8 = (int8_t) nextBlockRefineLevel;
			fwrite(&nextBlockRefineLevel_8, sizeof(nextBlockRefineLevel_8), 1, outMesh);
			fwrite(&blockXDown, sizeof(blockXDown), 1, outMesh);
			fwrite(&blockXUp, sizeof(blockXUp), 1, outMesh);
			fwrite(&blockYDown, sizeof(blockYDown), 1, outMesh);
			fwrite(&blockYUp, sizeof(blockYUp), 1, outMesh);
			fwrite(&blockZDown, sizeof(blockZDown), 1, outMesh);
			fwrite(&blockZUp, sizeof(blockZUp), 1, outMesh);

			nextFileIndex += sizeof(nextBlockRefineLevel_8) +
				sizeof(blockXDown) + sizeof(blockXUp) +
				sizeof(blockYDown) + sizeof(blockYUp) +
				sizeof(blockZDown) + sizeof(blockZUp);
		}
	}

	fclose(inMesh);
	fclose(outMesh);

	return 0;
}
