/******************************************************************************
* 
* dense_mm.c
* 
* This program implements a dense matrix multiply and can be used as a
* hypothetical workload. 
*
* Usage: This program takes a single input describing the size of the matrices
*        to multiply. For an input of size N, it computes A*B = C where each
*        of A, B, and C are matrices of size N*N. Matrices A and B are filled
*        with random values. 
*
* Written Sept 6, 2015 by David Ferry
******************************************************************************/

#include <stdio.h>  //For printf()
#include <stdlib.h> //For exit() and atoi()
#include <assert.h> //For assert()
#include <time.h>   //For clock_gettime() and strcut timespec

const int num_expected_args = 3;
const unsigned sqrt_of_UINT32_MAX = 65536;

// The following line can be used to verify that the parallel computation
// gives identical results to the serial computation. If the verficiation is
// successful then the program executes normally. If the verification fails
// the program will terminate with an assertion error.
//#define VERIFY_PARALLEL

int main( int argc, char* argv[] ){

	unsigned index, row, col; //loop indicies
	unsigned matrix_size, squared_size;
	double *A, *B, *C;
	#ifdef VERIFY_PARALLEL
	double *D;
	#endif
	
	//for measuring time of computations
	struct timespec curr_time;
	int iterations = 1;
	
	if( argc != 2 && argc != 3 ){
		printf("Usage: ./dense_mm <size of matrices> <enable multiple executions (optional;default=1)>\n");
		exit(-1);
	}
	
	if(argc == 3){
	  iterations = atoi(argv[2]);
	}

	matrix_size = atoi(argv[1]);
	
	if( matrix_size > sqrt_of_UINT32_MAX ){
		printf("ERROR: Matrix size must be between zero and 65536!\n");
		exit(-1);
	}

	squared_size = matrix_size * matrix_size;

	printf("Generating matrices...\n");

	A = (double*) malloc( sizeof(double) * squared_size );
	B = (double*) malloc( sizeof(double) * squared_size );
	C = (double*) malloc( sizeof(double) * squared_size );
	#ifdef VERIFY_PARALLEL
	D = (double*) malloc( sizeof(double) * squared_size );
	#endif

	for( index = 0; index < squared_size; index++ ){
		A[index] = (double) rand();
		B[index] = (double) rand();
		C[index] = 0.0;
		#ifdef VERIFY_PARALLEL
		D[index] = 0.0;
		#endif
	}

	printf("Multiplying matrices...\n");
	
        double sum = 0;
	unsigned int count = 0;
	unsigned long max = 0, min = 0;
	
	while(count < iterations){
	
	  //start of time measurement
	  unsigned long it_start, it_stop;

	  clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
	  it_start = curr_time.tv_nsec;

#pragma omp parallel for private(col, row, index)
	  for( col = 0; col < matrix_size; col++ ){
	    for( row = 0; row < matrix_size; row++ ){
	      for( index = 0; index < matrix_size; index++){
		C[row*matrix_size + col] += A[row*matrix_size + index] *B[index*matrix_size + col];
	      }	
	    }
	  }
	  
	  //end of time measurement
	  clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
	  it_stop = curr_time.tv_nsec;
	  
	  unsigned long elasped_time = it_stop - it_start;
	  sum += elasped_time;

	  if(max == 0){
	    max = elasped_time;
	  }
	  if(min == 0){
	    min = elasped_time;
	  }

	  if(elasped_time > max){
	    max = elasped_time;
	  }
	  if(elasped_time < min){
	    min = elasped_time;
	  }
	  
	  ++count;
	}

	#ifdef VERIFY_PARALLEL
	printf("Verifying parallel matrix multiplication...\n");
	for( col = 0; col < matrix_size; col++ ){
		for( row = 0; row < matrix_size; row++ ){
			for( index = 0; index < matrix_size; index++){
			D[row*matrix_size + col] += A[row*matrix_size + index] *B[index*matrix_size + col];
			}	
		}
	}

	for( index = 0; index < squared_size; index++ ) 
		assert( C[index] == D[index] );
	#endif //ifdef VERIFY_PARALLEL
	
	printf("Multiplication done!\n");
	
	printf("Took an average of %f ns!\n", sum/iterations);
	printf("The maximum time was %lu ns!\n", max);
	printf("The minimum time was %lu ns!\n", min);
	
	return 0;
}
