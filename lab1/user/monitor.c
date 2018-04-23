#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sched_monitor.h>

#define ENTRY_LIST_SIZE 5

int 
main(int     argc,
     char ** argv)
{
  int fd, status, i, num;
  unsigned long read_size;
  preemption_info_t entries[ENTRY_LIST_SIZE];
  
  fd = open(DEV_NAME, O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "Could not open %s: %s\n", DEV_NAME, strerror(errno));
    return -1;
  }
  
  status = enable_preemption_tracking(fd);
  if (status < 0) {
    fprintf(stderr, "Could not enable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
    close(fd);
    return -1;
  }
  
  for(i = 0; i<10000000; ++i){
    num *= 2;
  }


  status = disable_preemption_tracking(fd);
  if (status < 0) {
    fprintf(stderr, "Could not disable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
    close(fd);
    return -1;
  }
  
  read_size = read(fd, &entries, sizeof(preemption_info_t) * ENTRY_LIST_SIZE, NULL)
  while(read_size != 0){
    int i;
    for(i = 0; i < read_size; ++i){
      printf("Execution Time:\t%llu\n", entries[i].exec_time);
      printf("Wait Time:\t%llu\n", entries[i].wait_time);
      printf("Core Used:\t%u\n", entries[i].core);
      printf("Preempted by:\t%s\n\n", entries[i].preempting_process);
    }
    
    read_size = read(fd, &entries, sizeof(preemption_info_t) * ENTRY_LIST_SIZE, NULL);
  }
  
  close(fd);
  
  printf("Monitor ran to completion!\n");
  
  return 0;
}
