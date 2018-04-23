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

  printf("1\n");
  
  fd = shm_open(region_name, O_RDWR | O_CREAT, S_IRWXU);

  printf("2\n");
  
  if(ftruncate(fd, sizeof(struct data_t)) != 0){
    exit(-1);
  }
  
  printf("2\n");
  
  data_pt = (struct data_t *)mmap(NULL, sizeof(struct data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(data_pt == MAP_FAILED){
    exit(-1);
  }

  printf("4\n");
  
  array = (int *)malloc(sizeof(int) * STR_SIZE);
  srand(2415);

  printf("Leader: ");
  for(i = 0; i < STR_SIZE; ++i){
    array[i] = rand();
    printf("%d ", array[i]);
  }
  printf("\n");

  memcpy((void *)data_pt->data, (void *)array, STR_SIZE);

  free(array);
  
  return 0;
}
