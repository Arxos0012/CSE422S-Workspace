#include <time.h>
#include <stdio.h>

int main(){

  printf("Checking Clock Resolutions....\n");
  printf("Clock\t\t\tSeocnds Res\tNano Res\n");
  printf("--------------------------------------------------------\n");

  //Getting Resolutons
  
  struct timespec realtime, realtime_coarse, monotonic,
    monotonic_coarse, monotonic_raw, boottime,
    process, thread;

  clock_getres(CLOCK_REALTIME, &realtime);
  clock_getres(CLOCK_REALTIME_COARSE, &realtime_coarse);
  clock_getres(CLOCK_MONOTONIC, &monotonic);
  clock_getres(CLOCK_MONOTONIC_COARSE, &monotonic_coarse);
  clock_getres(CLOCK_MONOTONIC_RAW, &monotonic_raw);
  clock_getres(CLOCK_BOOTTIME, &boottime);
  clock_getres(CLOCK_PROCESS_CPUTIME_ID, &process);
  clock_getres(CLOCK_THREAD_CPUTIME_ID, &thread);

  printf("REALTIME\t\t%ld\t\t%ld\n", realtime.tv_sec, realtime.tv_nsec);
  printf("REALTIME_COARSE\t\t%ld\t\t%ld\n", realtime_coarse.tv_sec, realtime_coarse.tv_nsec);
  printf("MONOTONIC\t\t%ld\t\t%ld\n", monotonic.tv_sec, monotonic.tv_nsec);
  printf("MONOTONIC_COARSE\t%ld\t\t%ld\n", monotonic_coarse.tv_sec, monotonic_coarse.tv_nsec);
  printf("MONOTONIC_RAW\t\t%ld\t\t%ld\n", monotonic_raw.tv_sec, monotonic_raw.tv_nsec);
  printf("BOOTTIME\t\t%ld\t\t%ld\n", boottime.tv_sec, boottime.tv_nsec);
  printf("PROCESS_CPUTIME\t\t%ld\t\t%ld\n", process.tv_sec, process.tv_nsec);
  printf("THREAD_CPUTIME\t\t%ld\t\t%ld\n", thread.tv_sec, thread.tv_nsec);
 
  return 0;
}
