  #include <smmintrin.h>
  #include <x86intrin.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <stdint.h>
  #include <time.h>
  #include <assert.h>

  #define NUM_RUN 1024
  #define ARRAY_SIZE (16384*1024)

  #define CACHE_HIT 50
  #define TICKS_PER_NS 2

  int main(int argc, const char * * argv) {

      register uint64_t time_control_start, time_control_diff;
      register uint64_t time_rowhit_start, time_rowhit_diff;

      register uint64_t i = 0;
      register uint64_t nontemp_output = 0;
      register uint64_t rand_val = 0;

      // Junk variable for rdtscp calls
      int junk = 0;
      
      // Align a large memory space to 64-byte boundaries
      uint64_t *memory_array = NULL;
      posix_memalign(&memory_array, 64, ARRAY_SIZE*sizeof(uint64_t));

      // Ensure that allocated memory actually exists (with data == indices), but is not present in the cache
      for(i = 0; i < ARRAY_SIZE; i++) {
              memory_array[i] = i;
              _mm_clflush(&memory_array[i]);
              _mm_mfence();
      }

      srand(time(NULL));

      for(int y = 0; y < NUM_RUN; y++){
          // First, determine a random array index to probe 
          nontemp_output = (nontemp_output+rand())%ARRAY_SIZE;
          _mm_mfence();

          // Measure the time it takes to access the random array index
          time_control_start = __rdtscp(&junk);
          nontemp_output = memory_array[nontemp_output];
          time_control_diff = (__rdtscp(&junk) - time_control_start);

          // Make sure to flush after measurement
          _mm_mfence();
          _mm_clflush(&memory_array[nontemp_output]);

          // Now, find an address within the same DRAM row as the address we previously accessed
          nontemp_output = (nontemp_output+(rand()%32))%ARRAY_SIZE;
          _mm_mfence();

          // Measure the time it takes to access this row neighbor
          time_rowhit_start = __rdtscp(&junk);
          nontemp_output = memory_array[nontemp_output];
          time_rowhit_diff = (__rdtscp(&junk) - time_rowhit_start);
        
          // Make sure to flush after measurement
          _mm_mfence();
          _mm_clflush(&memory_array[nontemp_output]);
          
          // If we've encountered an errant cache hit (say, through prefetching), try again for this data point
          if (time_rowhit_diff < CACHE_HIT || time_control_diff < CACHE_HIT) {
              y--;
          } else {
              // Print the estimated time of each
              printf("%lu\t%lu\n", time_control_diff/(TICKS_PER_NS), time_rowhit_diff/(TICKS_PER_NS));
          }
      }
      
      free(memory_array);
  }
