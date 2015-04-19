
#include <sst_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
                int32_t* xDown, int32_t* xUp,
                int32_t* yDown, int32_t* yUp,
                int32_t* zDown, int32_t* zUp) {

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
        *xDown     = (int32_t) atoi(xDStr);
        *xUp       = (int32_t) atoi(xUStr);
        *yDown     = (int32_t) atoi(yDStr);
        *yUp       = (int32_t) atoi(yUStr);
        *zDown     = (int32_t) atoi(zDStr);
        *zUp       = (int32_t) atoi(zUStr);

	free(nextLine);
}

int main(int argc, char* argv[]) {
	printf("SST MiniAMR Mesh Converter\n");

	if(argc < 4) {
		usage();
	}

	uint32_t rankCount = (uint32_t) atoi(argv[1]);

	FILE* inMesh = fopen(argv[1], "rt");
	if(NULL == inMesh) {
		fprintf(stderr, "Unable to open input mesh: %s\n", argv[2]);
		exit(-1);
	}

	FILE* outMesh = fopen(argv[2], "wb");
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

	uint32_t blocksOnNode = 0;
	uint32_t nextBlockID = 0;
	uint32_t nextBlockRefineLevel = 0;
	int32_t  blockXDown = 0;
	int32_t  blockXUp = 0;
	int32_t  blockYDown = 0;
	int32_t  blockYUp = 0;
	int32_t  blockZDown = 0;
	int32_t  blockZUp = 0;

	for(uint32_t i = 0; i < rankCount; i++) {
		printf("Processing rank %" PRIu32 "...\n", i);

		char* strBlocksOnNode = readLine(inMesh);
		blocksOnNode = (uint32_t) atoi(strBlocksOnNode);

		fwrite(&blocksOnNode, sizeof(blocksOnNode), 1, outMesh);

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
			fwrite(&nextBlockRefineLevel, sizeof(nextBlockRefineLevel), 1, outMesh);
			fwrite(&blockXDown, sizeof(blockXDown), 1, outMesh);
			fwrite(&blockXUp, sizeof(blockXUp), 1, outMesh);
			fwrite(&blockYDown, sizeof(blockYDown), 1, outMesh);
			fwrite(&blockYUp, sizeof(blockYUp), 1, outMesh);
			fwrite(&blockZDown, sizeof(blockZDown), 1, outMesh);
			fwrite(&blockZUp, sizeof(blockZUp), 1, outMesh);
		}
	}

	fclose(inMesh);
	fclose(outMesh);

	return 0;
}
