/* This file implements a demonstration syscall for an OS course. It
 *  takes no arguments and prints out a simple message to the kernel
 *  log, indicating that it was called.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/syscalls.h>

//Define a system call implementation that takes no arguments
SYSCALL_DEFINE1( onearg, int, val ){

  // print out a simple message indicating the function was called, and return SUCCESS
  printk("Someone invoked the sys_onearg system call with val: %d\n", val);
  return 0;  
}

//End of file
