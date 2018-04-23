#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/types.h>

#define iters 1000000

struct task_struct * threads[4];
atomic_t shared_data;

static inline unsigned long long
get_current_time(void)
{
  struct timespec ts;
  getnstimeofday(&ts);
  
  return timespec_to_ns(&ts);
}

static int simple(void * data){
  int i;
  unsigned long long start, end;
  
  printk("%s starting!\n", current->comm);
  start = get_current_time();
  
  for(i=0; i<iters; ++i){
    atomic_add(1, &shared_data);
  }
  
  printk("%s ending!\n", current->comm);
  end = get_current_time();
  
  printk("%s took %llu ns to complete!\n", current->comm, end-start);

  while(!kthread_should_stop()){
    schedule();
  }
  return 0;
}

static int synchro_init(void){
  int i;
  
  atomic_set(&shared_data, 0);

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

  printk("shared_data = %d\n", atomic_read(&shared_data));
}

module_init(synchro_init);
module_exit(synchro_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jabari Booker");
MODULE_DESCRIPTION("Studio 10");
