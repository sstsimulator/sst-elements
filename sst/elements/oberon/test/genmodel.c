// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>

#include <oberoninst.h>

int main(int argc, char* argv[]) {

	FILE* model_file = fopen(argv[1], "wb");

	const int32_t jump = OBERON_INST_JUMP;
	const int32_t jump_forward_8 = OBERON_INST_JUMPREL;
	const int32_t jump_const_8   = 32;
	const int32_t zero = 0;

	const int64_t fifteen = 15;
	const int64_t one_hundred = 100;

	const int32_t push_inst = OBERON_INST_PUSH_LIT_I64;
	const int32_t add_i64   = OBERON_INST_IADD;
	const int32_t pop_i64   = OBERON_INST_POP_I64;
	const int32_t push_i64  = OBERON_INST_PUSH_I64;
	const int32_t print_i64 = OBERON_INST_PRINT_I64;
	const int32_t store_loc = 20;

	const int32_t halt_inst = OBERON_INST_HALT;

	// index 0
	fwrite(&jump_forward_8, sizeof(int32_t), 1, model_file);
	// index 4
	fwrite(&jump_const_8, sizeof(int32_t), 1, model_file);

	// Create 28 bytes for literals
	// index 8
	fwrite(&fifteen, sizeof(int64_t), 1, model_file);
	//fwrite(&zero, sizeof(int32_t), 1, model_file);
	// index 16
	fwrite(&one_hundred, sizeof(int64_t), 1, model_file);
	//fwrite(&zero, sizeof(int32_t), 1, model_file);
	// index 24
	fwrite(&zero, sizeof(int32_t), 1, model_file);
	// index 28
	fwrite(&zero, sizeof(int32_t), 1, model_file);
	// index 32
	fwrite(&zero, sizeof(int32_t), 1, model_file);

	// index 36
	fwrite(&push_inst, sizeof(int32_t), 1, model_file);

	// index 40
	fwrite(&fifteen, sizeof(int64_t), 1, model_file);

	// index 48
	fwrite(&push_inst, sizeof(int32_t), 1, model_file);

	// index 52
	fwrite(&one_hundred, sizeof(int64_t), 1, model_file);

	// index 60
	fwrite(&add_i64, sizeof(int32_t), 1, model_file);

	// index 64
	fwrite(&pop_i64, sizeof(int32_t), 1, model_file);

	// index 72
	fwrite(&store_loc, sizeof(int32_t), 1, model_file);

	// index 76
	fwrite(&push_i64, sizeof(int32_t), 1, model_file);

	// index 80
	fwrite(&store_loc, sizeof(int32_t), 1, model_file);

	// index 84
	fwrite(&print_i64, sizeof(int32_t), 1, model_file);

	// index 96
	fwrite(&halt_inst, sizeof(int32_t), 1, model_file);

	// index 88
	fwrite(&jump, sizeof(int32_t), 1, model_file);

	// index 92
	fwrite(&zero, sizeof(int32_t), 1, model_file);

	fclose(model_file);

	return 0;
}
