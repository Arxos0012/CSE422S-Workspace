#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>

#define iters 1000000

struct task_struct * threads[4];
volatile int shared_data = 0;

static int simple(void * data){
  int i;
  bool should_stop;
  
  for(i=0; i<iters; ++i){
    shared_data++;
  }

  should_stop = false;
  while(!should_stop){
    should_stop = kthread_should_stop();
    schedule();
  }
  return 0;
}

static int synchro_init(void){
  
  int i;
  for(i = 0; i < 4; ++i){
    struct task_struct * thread = kthread_create(simple, NULL, "proc %d", i);
    threads[i] = thread;
    kthread_bind(thread, i);
    wake_up_process(thread);
  }
  
  return 0;
}

static void synchro_exit(void){
  int i;
  for(i = 0; i < 4; ++i){
    kthread_stop(threads[i]);
  }

  printk("shared_data = %d\n", shared_data);
}

module_init(synchro_init);
module_exit(synchro_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jabari Booker");
MODULE_DESCRIPTION("Studio 10");
