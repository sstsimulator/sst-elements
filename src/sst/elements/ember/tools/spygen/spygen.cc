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


#include <sst_config.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define IMAGE_BINARY 0
#define IMAGE_TEXT   1

int verbose;
char* imageFile;
int dimX;
int dimY;
int ranks;
uint32_t* commData;
uint64_t* bytesData;
int outputMode;
int outputType;

void printOptions() {
	printf("SST Spyplot Generator\n");
	printf("============================================================\n\n");
	printf("-f <imagefile>     Generate image in <imagefile>\n");
	printf("-m                 0 = output in binary mode\n");
	printf("-v                 Use verbose output\n");
	printf("-h                 Print options\n");
	printf("-s                 0 = output sends, 1 = output bytes\n");
	printf("-x                 Pixels in X for each rank\n");
        printf("-y                 Pixels in Y for each rank\n");
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

uint64_t getBytesData(const int x, const int y) {
	return bytesData[x * ranks + y];
}

void parseOptions(int argc, char* argv[]) {

	verbose = 0;
	imageFile = NULL;
	dimX = 15;
	dimY = 15;
	ranks = 0;
	outputMode = 0;
	outputType = IMAGE_BINARY;

	for(int i = 0; i < argc; ++i) {
		if(strcmp(argv[i], "-f") == 0) {
			imageFile = (char*) malloc(sizeof(char) * (strlen(argv[i] + 1)));
			strcpy(imageFile, argv[i + 1]);
			i++;
		} else if(strcmp(argv[i], "-v") == 0) {
			verbose++;
		} else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
			printOptions();
			exit(0);
		} else if(strcmp(argv[i], "-m") == 0) {
			// Choose the output type of file (binary or text)
			if(strcmp(argv[i+1], "b" )== 0) {
				outputType = IMAGE_BINARY;
			} else if(strcmp(argv[i+1], "t") == 0) {
				outputType = IMAGE_TEXT;
			} else {
				printOptions();
				exit(-1);
			}

			i++;
		} else if(strcmp(argv[i], "-s") == 0) {
			if(strcmp(argv[i+2], "0") == 0) {
				outputMode = 0;
			} else if(strcmp(argv[i+1], "1") == 0) {
				outputMode = 1;
			} else {
				outputMode = 0;
			}

			i++;
		} else if(strcmp(argv[i], "-x") == 0) {
			dimX = atoi(argv[i+1]);
			i++;
		} else if(strcmp(argv[i], "-y") == 0) {
			dimY = atoi(argv[i+1]);
			i++;
		}
	}

	if(NULL == imageFile) {
		imageFile = (char*) malloc(sizeof(char) * 2);
		strcpy(imageFile, "-");
	}
}

void copy(char* dest, const char* source, int count) {
	for(int i = 0; i < count; ++i) {
		dest[i] = source[i];
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
	} else {
		// Generate a PPM file
		FILE* imagePPM = NULL;

		switch(outputType) {
		case IMAGE_BINARY:
			imagePPM = fopen(imageFile, "wb");
			// Magic number
			fprintf(imagePPM, "P6\n");
			break;
		case IMAGE_TEXT:
			imagePPM = fopen(imageFile, "wt");
			// Magic number
			fprintf(imagePPM, "P3\n");
			break;
		}

		// Image dimensions
		fprintf(imagePPM, "%" PRIu32 " %" PRIu32 "\n", (ranks * dimX), (ranks * dimY));
		// Max colour value
		fprintf(imagePPM, "255\n");

		uint32_t max_send_count = 1;
		uint64_t max_bytes_count = 1;

		for(int i = 0; i < ranks; ++i) {
			for(int j = 0; j < ranks; ++j) {
				max_send_count = getCommData(i, j) > max_send_count ? getCommData(i, j) : max_send_count;
				max_bytes_count = getBytesData(i, j) > max_bytes_count ? getBytesData(i, j) : max_bytes_count;
			}
		}

		char whiteBlock[3];
		whiteBlock[0] = (char)255;
		whiteBlock[1] = (char)255;
		whiteBlock[2] = (char)255;

		char commBlock[3];

		for(int i = 0; i < ranks; ++i) {
			for(int m = 0; m < dimY; ++m) {
				for(int j = 0; j < ranks; ++j) {
					for(int k = 0; k < dimX; ++k) {
						if(outputMode == 0) {
							if(getCommData(i, j) == 0) {
								switch(outputType) {
								case IMAGE_BINARY:
									fwrite(whiteBlock, sizeof(char), 3, imagePPM);
									break;
								case IMAGE_TEXT:
									fprintf(imagePPM, "255 255 255 ");
									break;
								}
							} else {
								switch(outputType) {
								case IMAGE_BINARY:
									commBlock[0] = 0;
									commBlock[1] = (char) (255.0 * (getCommData(i, j) / ((double) max_send_count)));
									commBlock[2] = 0;

									fwrite(commBlock, sizeof(char), 3, imagePPM);
									break;
								case IMAGE_TEXT:
									fprintf(imagePPM, "%d %d %d ", 0,
										(int) (255.0 * (getCommData(i, j) / ((double) max_send_count))), 0);
									break;
								}
							}
						} else if(outputMode == 1) {
							if(getBytesData(i, j) == 0) {
								switch(outputType) {
								case IMAGE_BINARY:
									fwrite(whiteBlock, sizeof(char), 3, imagePPM);
									break;
								case IMAGE_TEXT:
									fprintf(imagePPM, "255 255 255 ");
									break;
								}
							} else {
								switch(outputType) {
								case IMAGE_BINARY:
									commBlock[0] = 0;
									commBlock[1] = (char) (255.0 * (getBytesData(i, j) / ((double) max_bytes_count)));
									commBlock[2] = 0;

									fwrite(commBlock, sizeof(char), 3, imagePPM);
									break;
								case IMAGE_TEXT:
									fprintf(imagePPM, "%d %d %d ", 0,
										(int) (255.0 * (getBytesData(i, j) / ((double) max_bytes_count))), 0);
									break;
								}
							}
						}
					}
				}

				if(outputType == IMAGE_TEXT) {
					fprintf(imagePPM, "\n");
				}
			}
		}

		fclose(imagePPM);
	}

	////////////////////////////////////////////////////////////////////////////////
}
