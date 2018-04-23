#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"

int main(){
  int fd, i;
  struct data_t * data_pt;
  int * array;
  
  fd = shm_open(region_name, O_RDWR, S_IRWXU);
  
  data_pt = (struct data_t *)mmap(NULL, sizeof(struct data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(data_pt == MAP_FAILED){
    exit(-1);
  }

  array = (int *)malloc(sizeof(int) * STR_SIZE);
  
  memcpy((void *)array, (void *)data_pt->data, STR_SIZE);

  printf("Follower: ");
  for(i = 0; i < STR_SIZE; ++i){
    printf("%d ", array[i]);
  }
  printf("\n");

  free(array);
  
  return 0;
}
