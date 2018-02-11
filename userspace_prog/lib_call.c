#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

int main(){
  long user_id = getuid();
  printf("Current User ID: %ld\n", user_id);
  
  int return_val = setuid(0);

  if(return_val != 0){
    printf("Error: setuid failed! Reason: %s\n", strerror(errno));
  }
  else{
    printf("User id set to 0!");
  }
  
  user_id = getuid();

  printf("New User ID: %ld\n", user_id);

  return return_val;
}
