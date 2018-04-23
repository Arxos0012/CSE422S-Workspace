#ifndef MEM_H
#define MEM_H

#define STR_SIZE 10 

const char * region_name;

struct data_t{
  volatile int write_guard;
  volatile int read_guard;
  volatile int delete_guard;
  volatile int data[STR_SIZE];
};

#endif
