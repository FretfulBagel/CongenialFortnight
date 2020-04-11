#include <smmintrin.h>
#include <x86intrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define ARRAY_SIZE (16384*1024)

#define NUM_ACCESSES 1024*1024*2
#define NUM_MEASUREMENTS 1024

// Change me to apply additional load (or no load at all)
#define NUM_WORKER_PROC 5

#define TICKS_PER_NS 2

int main(int argc, const char * * argv) {

    register uint64_t time_start, time_diff;
    uint64_t time_sum = 0;
    
    volatile int nontemp_output;
    
    uint64_t average;
    
    int rand_val;
    unsigned long index;

    // Junk variable for rdtscp calls
    int junk = 0;

    // Align a large memory space to 64-byte boundaries
    uint64_t *memory_array = NULL;
    posix_memalign(&memory_array, 64, ARRAY_SIZE*sizeof(uint64_t));

    // Ensure that allocated memory actually exists (with data == indices), but is not present in the cache
    for(long i = 0; i < ARRAY_SIZE; i++) {
            memory_array[i] = i;
            _mm_clflush(&memory_array[i]);
            _mm_mfence();
    }
    
    srand(time(NULL));

    for(int x = 0; x < NUM_MEASUREMENTS; x++) { 

        // Spawn a number of worker proccesses which will continually access random array indices
        for(int i = 0; i < NUM_WORKER_PROC; i++) {
             fflush(stdout);

             if(fork() == 0) { // If we are the child process
                srand(time(NULL)+i); // Seed ourselves differently

                // Spam DRAM with requests
                for(int j = 0; j < NUM_ACCESSES; j++) {
                    index = (rand())%ARRAY_SIZE;

                    nontemp_output = memory_array[index];
                }

                exit(0); 
            } 
        }
        
        //Wait for one process to finish (the queues should be heavily loaded here)
	if(NUM_WORKER_PROC != 0) {
	        wait(NULL);
	}

        // Test while remaining processes are running...

        // Time the access time to a random index
        rand_val = rand() % ARRAY_SIZE;

        time_start = __rdtscp(&junk);
        nontemp_output = memory_array[rand_val];
        time_diff = __rdtscp(&junk) - time_start;

        time_sum += time_diff;

        // Make sure to flush out line after measurement (out of caution)
        _mm_clflush(&memory_array[rand_val]);

	printf("%d\t%lu\n", NUM_WORKER_PROC, time_diff/TICKS_PER_NS);
    }
    
    average = time_sum / NUM_MEASUREMENTS;
    printf("Average output time: %lu\n", average/TICKS_PER_NS);

    free(memory_array);
}
