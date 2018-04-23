#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/time.h>

#include <asm/uaccess.h>


/* DO NOT MODIFY THIS CHECK. If you see the message in the error line below
 * when compliling your module, that means your kernel has not been configured
 * correctly. Complete exercises 1-3 of the lab before continuing
 */
#ifdef CONFIG_PREEMPT_NOTIFIERS
#include <linux/preempt.h>
#else
#error "Your kernel must be built with CONFIG_PREEMPT_NOTIFIERS enabled"
#endif

#include <sched_monitor.h>

#define MAX_PREEMPTS 32768

struct preemption_tracker
{
  /* notifier to register us with the kernel's callback mechanisms */
  struct preempt_notifier notifier;
  bool enabled;
  
  unsigned int list_size;
  struct list_head preemption_list;
};


/* Information tracked on each preemption. */
struct preemption_entry
{
  char core; // which core to run on
  
  unsigned long long start_time, // time process started execution
    end_time,  // time process stopped execution
    exec_time, // time spent executing whilst on a core
    wait_time; // time spent waiting whilst preempted
  
  // name of preemption process
  char * preempting_process;
  
  struct list_head list_node;  // list node for process
};


/* Get the current time, in nanoseconds. Should be consistent across cores.
 * You are encouraged to look at the options in:
 *      include/linux/timekeeping.h
 */
static inline unsigned long long
get_current_time(void)
{
  struct timespec ts;
  getnstimeofday(&ts);
  
  return timespec_to_ns(&ts);
}

/* 
 * Utilities to save/retrieve a tracking structure for a process
 * based on the kernel's file pointer.
 *
 * DO NOT MODIFY THE FOLLOWING 2 FUNCTIONS
 */
static inline void
save_tracker_of_process(struct file               * file,
                        struct preemption_tracker * tracker)
{
    file->private_data = tracker;
}

static inline struct preemption_tracker *
retrieve_tracker_of_process(struct file * file)
{
    return (struct preemption_tracker *)file->private_data;
}

/*
 * Utility to retrieve a tracking structure based on the 
 * preemption_notifier structure.
 *
 * DO NOT MODIFY THIS FUNCTION
 */
static inline struct preemption_tracker *
retrieve_tracker_of_notifier(struct preempt_notifier * notifier)
{
    return container_of(notifier, struct preemption_tracker, notifier);
}

/* 
 * Callbacks for preemption notifications.
 *
 * monitor_sched_in and monitor_sched_out are called when the process that
 * registered a preemption notifier is either scheduled onto or off of a
 * processor.
 *
 * You will use these functions to create a linked_list of 'preemption_entry's
 * With this list, you must be able to represent information such as:
 *      (i)   the amount of time a process executed before being preempted
 *      (ii)  the amount of time a process was scheduled out before being
 *            scheduled back on 
 *      (iii) the cpus the process executes on each time it is scheduled
 *      (iv)  the number of migrations experienced by the process
 *      (v)   the names of the processes that preempt this process
 *
 * The data structure 'preemption_tracker' will be necessary to track some of
 * this information.  The variable 'pn' provides unique state that persists
 * across invocations of monitor_sched_in and monitor_sched_out. Add new fields
 * to it as needed to provide the information above.
 */

static void
monitor_sched_in(struct preempt_notifier * pn,
                 int                       cpu)
{
  struct preemption_tracker * tracker;
  
  struct preemption_entry * entry;
  struct list_head * head;
  unsigned long long start_time;
  
  tracker = retrieve_tracker_of_notifier(pn);
  
  //creating and setting up preemption entry
  entry = kmalloc(sizeof(struct preemption_entry), GFP_KERNEL);
  INIT_LIST_HEAD(&(entry->list_node));
  entry->core = (char)cpu;
  
  start_time = get_current_time();
  entry->start_time = start_time;
  entry->exec_time = 0;
  entry->preempting_process = "None";

  //getting head of preemption list
  head = &(tracker->preemption_list);

  //adding entry to tail of list and incrementing list size
  list_add_tail(&(entry->list_node), head);  
  tracker->list_size++;

  //if we added the first entry of the list, set wait time to 0
  if(tracker->list_size == 1){
    entry->wait_time = 0;
  }
  //otherwise calculate the wait time to current entry
  else{
    struct preemption_entry * prev_entry;
    struct list_head * node;
    
    //getting previous entry
    node = entry->list_node.prev;
    prev_entry = list_entry(node, struct preemption_entry, list_node);
    
    //calculating wait time
    entry->wait_time = start_time - prev_entry->end_time;
  }
  
  printk(KERN_INFO "sched_in for process %s\n", current->comm);
}

static void
monitor_sched_out(struct preempt_notifier * pn,
                  struct task_struct      * next)
{
  struct preemption_tracker * tracker;
  
  unsigned long long end_time;
  struct preemption_entry * tail;
  struct list_head * node;
      
  tracker = retrieve_tracker_of_notifier(pn);

  end_time = get_current_time();
  
  //getting tail of preemption entry list
  node = tracker->preemption_list.prev;
  
  //making sure the previous node isn't the head of the list
  if(node != &(tracker->preemption_list)){
    tail = list_entry(node, struct preemption_entry, list_node);
    
    //setting tail's end time and calculating execution time
    tail->end_time = end_time;
    tail->exec_time = end_time - tail->start_time;
    
    //recording name of preemting process
    if(next != NULL){
      tail->preempting_process = next->comm;
    }
    else{
      tail->preempting_process = "None";
    }
  }

  printk(KERN_INFO "sched_out for process %s\n", current->comm);
}

static struct preempt_ops
notifier_ops = 
{
  .sched_in  = monitor_sched_in,
  .sched_out = monitor_sched_out
};

/*** END preemption notifier ***/


/*
 * Device I/O callbacks for user<->kernel communication 
 */

/*
 * This function is invoked when a user opens the file /dev/sched_monitor.
 * You must update it to allocate the 'tracker' variable and initialize 
 * any fields that you add to the struct
 */
static int
sched_monitor_open(struct inode * inode,
                   struct file  * file)
{
    struct preemption_tracker * tracker;

    printk(KERN_DEBUG "Process %d (%s) tried to open " DEV_NAME ", but this is disabled\n",
        current->pid, current->comm);
    
    tracker = kmalloc(sizeof(struct preemption_tracker), GFP_KERNEL);
    preempt_notifier_init(&(tracker->notifier), &notifier_ops);
    tracker->enabled = false;
    INIT_LIST_HEAD(&(tracker->preemption_list));
    tracker->list_size = 0;
    
    /* Save tracker so that we can access it on other file operations from this process */
    save_tracker_of_process(file, tracker);

    return 0;
}

/* This function is invoked when a user closes /dev/sched_monitor.
 * You must update is to free the 'tracker' variable allocated in the 
 * sched_monitor_open callback, as well as free any other dynamically 
 * allocated data structures you may have allocated (such as linked lists)
 */
static int
sched_monitor_flush(struct file * file,
                    fl_owner_t    owner)
{
  struct preemption_tracker * tracker;
  
  struct preemption_entry *curr_entry, *temp_store;
  struct list_head * head;
  
  tracker = retrieve_tracker_of_process(file);
  
  /* Unregister notifier */
  if (tracker->enabled) {
    preempt_notifier_unregister(&(tracker->notifier));
    tracker->enabled = false;
  }
  
  head = &(tracker->preemption_list);
  
  //print list information for each entry and then delete
  list_for_each_entry_safe(curr_entry, temp_store, head, list_node){
    
    printk("Execution Time:\t%llu\n", curr_entry->exec_time);
    printk("Wait Time:\t%llu\n", curr_entry->wait_time);
    printk("Core Used:\t%u\n", curr_entry->core);
    printk("Preempted by:\t%s\n\n", curr_entry->preempting_process);
    
    list_del(&(curr_entry->list_node));  
    kfree(curr_entry);
  }
    
  //free allocated memory for preemption tracker
  kfree(tracker);

  printk(KERN_DEBUG "Process %d (%s) closed " DEV_NAME "\n",
	 current->pid, current->comm);

  return 0;
}

/* 
 * Enable/disable preemption tracking for the process that opened this file.
 * Do so by registering/unregistering preemption notifiers.
 */
static long
sched_monitor_ioctl(struct file * file,
                    unsigned int  cmd,
                    unsigned long arg)
{
    struct preemption_tracker * tracker = retrieve_tracker_of_process(file);

    switch (cmd) {
        case ENABLE_TRACKING:
            if (tracker->enabled) {
                printk(KERN_ERR "Tracking already enabled for process %d (%s)\n",
                    current->pid, current->comm);
                return 0;
            }

            preempt_notifier_register(&(tracker->notifier));
	    tracker->enabled = true;

            printk(KERN_DEBUG "Process %d (%s) trying to enable preemption tracking via " DEV_NAME ". Not implemented\n",
                current->pid, current->comm);

            break;


        case DISABLE_TRACKING:
            if (!tracker->enabled) {
                printk(KERN_ERR "Tracking not enabled for process %d (%s)\n",
                    current->pid, current->comm);
                return 0;
            }

            preempt_notifier_unregister(&(tracker->notifier));
	    tracker->enabled = false;

            printk(KERN_DEBUG "Process %d (%s) trying to disable preemption tracking via " DEV_NAME ". Not implemented\n",
                current->pid, current->comm);

            break;

        default:
            printk(KERN_ERR "No such ioctl (%d) for " DEV_NAME "\n", cmd);
            return -ENOIOCTLCMD;
    }

    return 0;
}

/* User read /dev/sched_monitor
 *
 * In this function, you will copy an entry from the list of preemptions
 * experienced by the process to user-space.
 */
static ssize_t
sched_monitor_read(struct file * file,
                   char __user * buffer,
                   size_t        length,
                   loff_t      * offset)
{
  struct preemption_tracker * tracker;
  struct preemption_entry *curr_entry, *temp_entry;
  struct list_head * head;
  spinlock_t lock;
  unsigned long flags;
  unsigned int amnt, i, info_size;
  
  preemption_info_t entry_data;
  
  tracker = retrieve_tracker_of_process(file);
  info_size = sizeof(preemption_info_t);

  //checking that the buffer is of a valid size
  if(length % info_size){
    return -EINVAL;
  }
  
  /*calcuating number of entries to copy and setting
    it to list_size if greater
   */
  amnt = length / info_size;
  if(amnt > tracker->list_size){
    printk("%u entries were request, but only %u entries could be copied!\n",
	   amnt, tracker->list_size);
    amnt = tracker->list_size;
  }
  
  //initializing lock and locking list
  spin_lock_init(&lock);
  spin_lock_irqsave(&lock, flags);
  
  //getting starting node for copy
  head = &(tracker->preemption_list);
  
  //copying over entry data and deleting them after copied
  i = 0;
  list_for_each_entry_safe(curr_entry, temp_entry, head, list_node){
    
    int x;
    void * user_entry;
    unsigned long bytes_not_copied;
    
    //break if we have copied amnt elements
    if(i == amnt) break;
    
    //setting up where to write in buffer
    user_entry = (void *)(buffer + (i++)*info_size);
    
    //extracting data from the current entry
    entry_data.core = curr_entry->core;
    entry_data.exec_time = curr_entry->exec_time;
    entry_data.wait_time = curr_entry->wait_time;
    for(x = 0; x < TASK_COMM_LEN; ++x){
      entry_data.preempting_process[x] = curr_entry->preempting_process[x];
    }
    
    //copying data to userspace
    bytes_not_copied =
      copy_to_user(user_entry, (void *)(&entry_data), info_size);
    
    if(bytes_not_copied){
      printk("Failed to copy %lu bytes to user memory!\n", bytes_not_copied);
    }
    
    //decrementing list size and deleting the current node and its corresponding entry
    tracker->list_size--;
    list_del(&(curr_entry->list_node));
    kfree(curr_entry);
  }
  
  //unlocking list
  spin_unlock_irqrestore(&lock, flags);
  
  printk(KERN_DEBUG "Process %d (%s) read " DEV_NAME "\n", current->pid, current->comm);
  
  return amnt * info_size;
}

static struct file_operations
dev_ops = 
{
    .owner = THIS_MODULE,
    .open = sched_monitor_open,
    .flush = sched_monitor_flush,
    .unlocked_ioctl = sched_monitor_ioctl,
    .compat_ioctl = sched_monitor_ioctl,
    .read = sched_monitor_read,
};

static struct miscdevice
dev_handle = 
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = SCHED_MONITOR_MODULE_NAME,
    .fops = &dev_ops,
};
/*** END device I/O **/



/*** Kernel module initialization and teardown ***/
static int
sched_monitor_init(void)
{
    int status;

    /* Create a character device to communicate with user-space via file I/O operations */
    status = misc_register(&dev_handle);
    if (status != 0) {
        printk(KERN_ERR "Failed to register misc. device for module\n");
        return status;
    }

    /* Enable preempt notifiers globally */
    preempt_notifier_inc();

    printk(KERN_INFO "Loaded sched_monitor module. HZ=%d\n", HZ);

    return 0;
}

static void 
sched_monitor_exit(void)
{
    /* Disable preempt notifier globally */
    preempt_notifier_dec();

    /* Deregister our device file */
    misc_deregister(&dev_handle);

    printk(KERN_INFO "Unloaded sched_monitor module\n");
}

module_init(sched_monitor_init);
module_exit(sched_monitor_exit);
/*** End kernel module initialization and teardown ***/


/* Misc module info */
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Jabari Booker, Sam Westerman");
MODULE_DESCRIPTION ("CSE 422S Lab 1");
