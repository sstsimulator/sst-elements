
#include <sst_config.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

int verbose;
char* imageFile;
int dimX;
int dimY;
int ranks;
uint32_t* commData;
uint64_t* bytesData;
int outputMode;

void printOptions() {
	printf("SST Spyplot Generator\n");
	printf("============================================================\n\n");
	printf("-f <imagefile>     Generate image in <imagefile>\n");
	printf("-v                 Use verbose output\n");
	printf("-h                 Print options\n");
}

void autoCalculateRanks() {
	ranks = 0;
	int continueChecking = 1;
	char* nameBuffer = (char*) malloc(sizeof(char) * 128);

	while(continueChecking) {
		sprintf(nameBuffer, "ember-%d.spy", ranks);
		FILE* checkFile = fopen(nameBuffer, "rt");

		if(NULL == checkFile) {
			break;
		} else {
			fclose(checkFile);
			ranks++;
		}
	}
}

void setCommData(const int x, const int y, const uint32_t value) {
	commData[x * ranks + y] = value;
}

uint32_t getCommData(const int x, const int y) {
	return commData[x * ranks + y];
}

void setBytesData(const int x, const int y, const uint64_t value) {
	bytesData[x * ranks + y] = value;
}

uint64_t getBytesData(const int x, const int y, const uint64_t value) {
	return bytesData[x * ranks + y];
}

void parseOptions(int argc, char* argv[]) {

	verbose = 0;
	imageFile = NULL;
	dimX = 15;
	dimY = 15;
	ranks = 0;

	for(int i = 0; i < argc; ++i) {
		if(strcmp(argv[i], "-f") == 0) {
			imageFile = (char*) malloc(sizeof(char) * (strlen(argv[i] + 1)));
			strcpy(imageFile, argv[i]);
			i++;
		} else if(strcmp(argv[i], "-v") == 0) {
			verbose++;
		} else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
			printOptions();
			exit(0);
		}
	}

	if(NULL == imageFile) {
		imageFile = (char*) malloc(sizeof(char) * 2);
		strcpy(imageFile, "-");
	}
}

int main(int argc, char* argv[]) {
	parseOptions(argc, argv);

	if(verbose) {
		printf("Locating Spyplot information...\n");
	}

	autoCalculateRanks();

	if(verbose) {
		printf("Located %d ranks of Spyplot information.\n", ranks);
	}

	if(0 == ranks) {
		printf("Could not locate Spyplot rank information (\"ember-<rank id>.py\")\n");
		exit(-1);
	}

	////////////////////////////////////////////////////////////////////////////////

	if(verbose) {
		printf("Generating a communication matrix of %d x %d...\n", (ranks), (ranks));
	}

	commData = (uint32_t*) malloc(sizeof(uint32_t) * (ranks * ranks));
	bytesData = (uint64_t*) malloc(sizeof(uint64_t) * (ranks * ranks));

	if(verbose) {
		printf("Initializing communication matrix of %d x %d...\n", (ranks), (ranks));
	}

	// Initialize everything to zero
	for(int i = 0; i < ranks; ++i) {
		for(int j = 0; j < ranks; ++j) {
			setCommData(i, j, (uint32_t) 0);
			setBytesData(i, j, (uint64_t) 0);
		}
	}

	if(verbose) {
		printf("Processing spy files..\n");
	}

	////////////////////////////////////////////////////////////////////////////////

	char* spynameBuffer = (char*) malloc(sizeof(char) * 256);
	char* lineBuffer = (char*) malloc(sizeof(char) * 1024);

	for(int i = 0; i < ranks; ++i) {
		sprintf(spynameBuffer, "ember-%d.spy", i);

		if(verbose) {
			printf("Processing rank %d (%s)...\n", i, spynameBuffer);
		}

		// Open the rank info
		FILE* rankInfo = fopen(spynameBuffer, "rt");

		while(! feof(rankInfo)) {
			if(verbose) {
				printf("Reading next line...\n");
			}

			int nextBufferIndex = 0;
			char nextChar = (char) getc(rankInfo);

			while((!feof(rankInfo)) && (nextChar != '\n')) {
				lineBuffer[nextBufferIndex] = nextChar;
				nextBufferIndex++;

				nextChar = (char) fgetc(rankInfo);
			}

			if(strlen(lineBuffer) < 6) {
				break;
			}

			if(verbose) {
				printf("Tokenizing and converting into ranks and communication information...\n");
			}

			// Set the very last character to be a NULL
			lineBuffer[nextBufferIndex] = '\0';

			char* commPartner = strtok(lineBuffer, " ");
			char* sendCount   = strtok(NULL, " ");
			char* sendBytes   = strtok(NULL, " ");

			uint32_t iCommPartner = (uint32_t) atoi(commPartner);
			uint64_t iSendBytes = (uint64_t) atol(sendBytes);

			if(verbose) {
				printf("Attributing information: partner rank %d, send count=%s, send bytes=%s...\n",
					iCommPartner, sendCount, sendBytes);
			}

			// Set data into our matrix
			setCommData(i, iCommPartner, 1);
			setBytesData(i, iCommPartner, iSendBytes);
			setCommData(iCommPartner, i, 1);
			setBytesData(iCommPartner, i, iSendBytes);
		}

		if(verbose) {
			printf("Completed processing, closing statistics file.\n");
		}

		// Close this rank info
		fclose(rankInfo);
	}

	////////////////////////////////////////////////////////////////////////////////

	if(strcmp(imageFile, "-") == 0) {
		for(int i = 0; i < ranks; ++i) {
			printf("%8d |  ", i);

			for(int j = 0; j < ranks; ++j) {
				printf("%3" PRIu32 , getCommData(i, j));
			}

			printf("\n");
		}
	}

	////////////////////////////////////////////////////////////////////////////////
}
