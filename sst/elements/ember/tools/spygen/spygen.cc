
#include <sst_config.h>
#include <stdio.h>

int verbose;
char* imageFile;
int dimX;
int dimY;
int ranks;
char* commData;
uint64_t bytesData;

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

void setCommData(const int x, const int y, const char value) {
	commData[(x * ranks + y] = value;
}

char getCommData(const int x, const int y) {
	return commData[x * ranks + y];
}

void setBytesData(const int x, const int y, const uint64_t value) {
	bytesData[x * ranks + y] = value;
}

void getBytesData(const int x, const int y, const uint64_t value) {
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
			imageFile = (char*) malloc(sizeof(char) * (strlen(argv[i] + 1));
			strcpy(imageFile, argv[i]);
			i++;
		} else if(strcmp(argv[i], "-v" == 0) {
			verbose++;
		} else if(strcmp(argv[i], "-h" == 0 || strcmp(argv[i], "-help") == 0) {
			printOptions();
			exit(0);
		}
	}

	if(NULL == imageFile) {
		printOptions();
		printf("\n\nYou did not specify a file location!\n\n");
		exit(-1);
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
		printf("Generating a communication matrix of %d x %d...\n", (dimX * ranks), (dimY * ranks));
	}

	commData = (char*) malloc(sizeof(char) * (ranks * ranks));
	bytesData = (uint64_t*) malloc(sizeof(uint64_t) * (ranks * ranks));

	// Initialize everything to zero
	for(int i = 0; i < ranks; ++i) {
		for(int j = 0; i < ranks; ++j) {
			setCommData(i, j, (char) 0);
			setBytesData(i, u, (uint64_t) 0);
		}
	}

	////////////////////////////////////////////////////////////////////////////////

}
