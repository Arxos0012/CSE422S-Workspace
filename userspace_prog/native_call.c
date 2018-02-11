#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

//#define __NR_setuid 23
//#define __NR_getuid 24

int main(){
  
  long user_id = syscall(__NR_getuid);
  //long user_id = getuid();
  printf("Current User ID: %ld\n", user_id);
  
  int return_val = syscall(__NR_setuid, 0);
  //int return_val = setuid(0);

  if(return_val != 0){
    printf("Error: setuid failed! Reason: %s\n", strerror(errno));
  }
  else{
    printf("User id set to 0!");
  }
  
  user_id = syscall(__NR_getuid);

  printf("New User ID: %ld\n", user_id);

  return return_val;
}
