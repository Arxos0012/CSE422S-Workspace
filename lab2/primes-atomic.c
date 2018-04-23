#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/time.h>

#define NUM_CORES 4   //number of cores for the raspberry pi 3

/* Misc module info */
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Jabari Booker");
MODULE_DESCRIPTION ("CSE 422S Lab 2");

/* Module Parameters */
static unsigned long num_threads = 1;   //number of threads to be used my module (at least 1)
static unsigned long upper_bound = 10;   //upper bound of number to scan for primes (at least 2)

module_param(num_threads, ulong, 0);
MODULE_PARM_DESC(num_threads, "Number of threads to be used by module (at least 1)");
module_param(upper_bound, ulong, 0);
MODULE_PARM_DESC(upper_bound, "Upper bound of number to scan for primes (at least 2)");

/* Internal Tracking Variables  */
static struct task_struct ** threads;   //pointers to each thread
static bool alloc_failure = true;   //denotes if an allocation failure has occured
                                    //true until absolutely certain that all allocations succeed

atomic_t reached_fir_barr;   //denotes when the first synchronization barrier is reached
atomic_t reached_sec_barr;   //denotes when the second synchronization barrier is reached

static unsigned long long init_time = 0;   //time that module initialization starts
static unsigned long long first_barr_time = 0;   //time at which all threads reach 1st barrier
static unsigned long long second_barr_time = 0;   //time at which all threads reach 2nd barrier

/* Computation Tracking Variables */
static unsigned int * crossed;   //tracks how many times each thread has "crossed-out" a number
atomic_t * numbers;   //range of numbers to scan
static unsigned long numbers_size = 0;
static unsigned int curr = 0;   //position of the current number for a given thread
atomic_t threads_done;   //denotes if computation has finished in each thread

/* Sets a timestamp variable */
static void set_timestamp(unsigned long long * timestamp){
  struct timespec ts;
  
  if(!timestamp) return;
  
  getnstimeofday(&ts);
  
  *timestamp = timespec_to_ns(&ts);
} 

/* Helper function to get the position of a multiple x*n, where
   x = start and n = mutiplier. The result is stored in mult_pos
*/
static void get_mult_pos(unsigned long * mult_pos,
			 unsigned long start,
			 unsigned int multiplier)
{
  *mult_pos = (start * multiplier) - 2;
}

/* Synchronizes threads before calculation */
static void first_sync(void){
  
  //once all threads reach the barrier they are synched
  if(atomic_inc_return(&reached_fir_barr) == num_threads){
        
    //recording time at which second barrier is reached
    set_timestamp(&first_barr_time);
        
  }
  //if not all threads have reached the barrier, spin until they do
  else{
    while(atomic_read(&reached_fir_barr) != num_threads){}
  }
}

/* Synchronizes threads after calculation */
static void second_sync(void){
    
  //once all threads reach the barrier they are synched
  if(atomic_inc_return(&reached_sec_barr) == num_threads){
        
    //recording time at which second barrier is reached
    set_timestamp(&second_barr_time);
        
  }
  //if not all threads have reached the barrier, spin until they do
  else{
    while(atomic_read(&reached_sec_barr) != num_threads){}
  }
}

/* 'Crosses out' the primes in the list of integers and records how
   many times it happens for each thread */
static void find_primes(unsigned int * counter){
  
  unsigned long pos;
  
  //initializing spin lock
  spinlock_t lock;
  unsigned long ic;
  spin_lock_init(&lock);
  
  spin_lock_irqsave(&lock, ic);
  pos = curr;
  spin_unlock_irqrestore(&lock, ic);

  while(pos < numbers_size){    
    
    unsigned int val = atomic_read(numbers + pos);
    
    //found prime, 'cross out' its multiples
    if(val != 0){
      
      unsigned long mult_pos;
      unsigned int multiplier = 2;
      get_mult_pos(&mult_pos, val, multiplier);
            
      //'crossing out' multiples of prime and incrementing counter
      while(mult_pos < numbers_size){
	
	atomic_set(numbers + mult_pos, 0);
	
	++(*counter);
        get_mult_pos(&mult_pos, val, ++multiplier);
      }
      
    }
    
    spin_lock_irqsave(&lock, ic);
    pos = ++curr;
    spin_unlock_irqrestore(&lock, ic);
  }
  
}

static int thread_fn(void * counter){
  
  //counter for how many times the thread has 'crossed out a number'
  unsigned int * times_crossed = (unsigned int *)counter;
  
  //waiting for all threads to be synchronized
  first_sync();
  
  find_primes(times_crossed);
  
  //waiting for all threads to resynchronize
  second_sync();
  
  //printk("Synched and done!");

  //indicates all thread are done processing
  atomic_inc(&threads_done);
  
  //thread waiting to be stopped and preempted to prevent thread from spinning on CPU
  while(!kthread_should_stop()){
    schedule();
  }
  
  return 0;
}

static int primes_init(void){
  unsigned long i;
  unsigned char invalid_params;
  
  //getting init time
  set_timestamp(&init_time);
  
  //used to detect if the module parameters are invalid
  invalid_params = 0;
  
  /*initializing all variables for case in which invalid parameters are passed
    or failures occur*/
  crossed = 0;
  numbers = 0;
  threads = 0;
  atomic_set(&threads_done, num_threads);
  atomic_set(&reached_fir_barr, 0);
  atomic_set(&reached_sec_barr, 0);
    
  //checking that module parameters are vaild
  invalid_params = (num_threads < 1) ? 1 : 0; 
  invalid_params = (upper_bound < 2) ? invalid_params | 2 : invalid_params;
  
  if(invalid_params){
    if(invalid_params & 1) printk("Invalid number of threads! Must be at lesast 1!\n");
    if(invalid_params & 2) printk("Invalid upper bound! Must be at lesast 2!\n");
    return -1;
  }
    
  //allocating numbers and checking success
  numbers_size = upper_bound - 1;
  numbers = kmalloc(sizeof(atomic_t) * numbers_size, GFP_KERNEL);
  if(!numbers){
    printk("Failed to allocate memory for the number analyzed for primes!\n");
    return -1;
  }
  //setting to contain all numbers 2 to upper_bound
  for(i = 0; i < numbers_size; ++i){
    atomic_set(numbers + i, i+2);
  }


  //allocating crossed and checking success
  crossed = kmalloc(sizeof(unsigned int) * num_threads, GFP_KERNEL);
  if(!crossed){
    printk("Failed to allocate memory to store number of times each thread 'crosses out' a number!\n");
    return -1;
  }
  
  //setting counters to 0 for each thread
  for(i = 0; i < num_threads; ++i){
    crossed[i] = 0;
  }
  
  
  //allocating thread  pointer array and checking success
  threads = kmalloc(sizeof(struct task_struct *) * num_threads, GFP_KERNEL);
  if(!threads){
    printk("Failed to allocate memory to keep track of thread information!\n");
    return -1;
  }
  
  //setting threads_done to denote that all threads have not yet finished computation
  atomic_set(&threads_done, 0);
  
  //indicating that all allocations were successful
  alloc_failure = false;
  
  //creating and running threads
  for(i = 0; i < num_threads; ++i){
    struct task_struct * thread = kthread_create(thread_fn, crossed + i, "Eratosthenes %lu", i);
    threads[i] = thread;
    kthread_bind(thread, i % NUM_CORES);
    wake_up_process(thread);
  }
  
  return 0;
}

static void primes_exit(void){
  
  //checking whether of not the threads finished their computation
  if(atomic_read(&threads_done) != num_threads){
    printk("Not all threads were able to complete their computation! CAUTION: There may be a memory leak due to this!\n");
    return;
  }
  
  //occurs only when program successfully ran with no allocation failures
  if(!alloc_failure){
    unsigned long i, p, total_crosses;
    unsigned int val;
    bool new_line;
    
    //stopping all threads
    for(i = 0; i < num_threads; ++i){
      kthread_stop(threads[i]);
    }
    
    //printing out and counting primes found
    printk("Primes found:\n");
    p = 0;
    new_line = true;
    for(i = 0; i < numbers_size; ++i){
      val = atomic_read(numbers + i);
      if(val){
	printk("%u\t", val);
	++p;
	new_line = true;
      }
      if(!(p % 8) && new_line){
	printk("\n");
	new_line = false;
      }
    }
    if(p % 8) printk("\n");
    printk("\n");
    
    //summing total number of times numbers were 'crossed out' between all threads
    total_crosses = 0;
    for(i = 0; i < num_threads; ++i){
      total_crosses += crossed[i];
    }
    
    printk("%lu primes were found!\n", p);
    printk("%lu non-primes were found!\n", numbers_size - p);
    printk("Number of Unnecessary Cross-outs: %lu\n\n", 
	   total_crosses - numbers_size + p);
    printk("Upper bound: %lu\n", upper_bound);
    printk("Number of threads: %lu\n", num_threads);
    printk("Initalization time stamp: %llu\n", init_time);
    printk("First sychronization barrier time stamp: %llu\n", first_barr_time);
    printk("Second sychronization barrier time stamp: %llu\n", second_barr_time);
  }
  
  //deallocating counters, numbers analyzed, and threads
  kfree(crossed);
  kfree(numbers);
  kfree(threads);
}

module_init(primes_init);
module_exit(primes_exit);

