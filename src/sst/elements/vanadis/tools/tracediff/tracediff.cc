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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>

void read_line( FILE* input_file, char* buffer ) {
	buffer[0] = '\0';
	int next_index = 0;

	while( ! feof( input_file ) ) {
		int tmp_char = fgetc( input_file );

		if( EOF ==  tmp_char ) {
			// Nothing to do here
			break;
		} else if( '\n' == tmp_char ) {
			// New line, nothing to do here
			break;
		} else {
			buffer[next_index] = static_cast<char>( tmp_char );
		}

		next_index++;
	}

	buffer[next_index] = '\0';
};

void generate_error( int left_line, int right_line, const char* error_msg ) {
	fprintf(stderr, "Error left-line: %d, right-line: %d, cause: %s\n",
		left_line, right_line, error_msg);
}

int main( int argc, char* argv[] ) {

	char* left_file_path  = NULL;
	char* right_file_path = NULL;

	if( argc < 3 ) {
		fprintf(stderr, "usage: tracediff <file1> <file2>\n");
		exit(1);
	}

	left_file_path  = argv[1];
	right_file_path = argv[2];

	FILE* left_file  = fopen( left_file_path, "rt" );
	FILE* right_file = fopen( right_file_path, "rt" );

	if( left_file == NULL ) {
		fprintf(stderr, "File: %s cannot be opened.\n", left_file_path);
		exit(1);
	}

	if( right_file == NULL ) {
		fprintf(stderr, "File: %s cannot be opened\n", right_file_path);
	}

	char* left_buffer  = new char[4096];
	char* right_buffer = new char[4096];

	assert( left_buffer  != NULL );
	assert( right_buffer != NULL );

	///////////////////////////////////////////////////////////////////////

	bool keep_processing = true;

	int left_line_index  = 1;
	int right_line_index = 1;

	while( keep_processing ) {
		if( feof( left_file ) ) {
			if( feof( right_file ) ) {
				// end of file at the same time
				keep_processing = false;
			} else {
				generate_error( left_line_index, right_line_index, "Left file end but right file end not reached, right file is longer.");
				keep_processing = false;
			}
		} else {
			if( feof( right_file ) ) {
				generate_error( left_line_index, right_line_index, "Right file end reached, but left file end not reached, left file is longer.");
				keep_processing = false;
			} else {
				read_line( left_file,  left_buffer  );
				read_line( right_file, right_buffer );

				if( strcmp( left_buffer, right_buffer ) != 0 ) {
					fprintf(stderr, "left: \"%s\" right: \"%s\"\n", left_buffer, right_buffer);
					generate_error( left_line_index, right_line_index, "Lines do not match.");
					keep_processing = false;
				}

				left_line_index++;
				right_line_index++;
			}
		}
	}

	///////////////////////////////////////////////////////////////////////

	delete[] left_buffer;
	delete[] right_buffer;

	if( left_file != NULL ) {
		fclose(left_file);
	}

	if( right_file != NULL ) {
		fclose(right_file);
	}

	return 0;
}
