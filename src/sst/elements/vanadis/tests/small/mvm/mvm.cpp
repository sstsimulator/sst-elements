#include "mvm.h"

int main() {

	int i, j;

    // Matrix A
    float matrix_A[9] = {
	0.8907, 0.1018, 0.9211, 
	0.6168, 0.8393, 0.3699, 
	0.4607, 0.2052, 0.1159
    };

    // Matrix B
    float matrix_B[9] = {
	0.4969, 0.0362, 0.4582, 
	0.0123, 0.5043, 0.4484, 
	0.4671, 0.2418, 0.4760
    };

	int k, l;

    // Create input vectors 
    float in_vec1[3] = { 0.4626, 0.9365, 0.9829 };
    float in_vec2[3] = { 0.0214, 0.6014, 0.1476 };

    // Create output vectors
    float out_vec1[3] = { 0.0, 0.0, 0.0 };
    float out_vec2[3] = { 0.0, 0.0, 0.0 };

    // Get lengths of matrices and vectors
/*    int matrix_A_length = sizeof(matrix_A) / sizeof(matrix_A[0]);
    int matrix_B_length = sizeof(matrix_B) / sizeof(matrix_B[0]);
    int vec1_length = sizeof(vec_1) / sizeof(vec_1[0]);
    int vec2_length = sizeof(vec_2) / sizeof(vec_2[0]);
*/
    // Set matrix A and B
    mvm_set(matrix_A, 0); //, matrix_A_length);
    mvm_set(matrix_B, 1); //, matrix_B_length);

    // Load input array 1 and 2 
    mvm_load(in_vec1, 0); //, vec1_length);
    mvm_load(in_vec2, 1); //, vec2_length);

    // Compute MVM on array 1 and 2
    mvm_compute(0);
    mvm_compute(1);

    // Store output array 2 to out_vec2
    mvm_store(out_vec2, 1); //, vec_2_length);

    // Move array 1 to array 2
    mvm_move(0, 1);
    
    // Compute MVM on array 2
    mvm_compute(1);

    // Store output array 2 to out_vec1
    mvm_store(out_vec1, 1);
}
