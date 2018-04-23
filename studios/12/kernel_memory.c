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
#include <linux/kthread.h>
#include <linux/mm.h>

#include <asm/uaccess.h>


typedef struct{
  unsigned int array[8];
}datatype_t;

static uint nr_structs = 2000;
module_param(nr_structs, uint, 0644); 

static struct task_struct * kthread = NULL;

static unsigned int nr_pages,
  order, 
  nr_structs_per_page;

static struct page * pages;

static unsigned int
my_get_order(unsigned int value)
{
  unsigned int shifts = 0;
  
  if (!value)
    return 0;
  
  if (!(value & (value - 1)))
    value--;
  
  while (value > 0) {
    value >>= 1;
    shifts++;
  }
  
  return shifts;
}

static void * get_virt_addr(struct page * p){  
  unsigned long pfn, p_addr;
  
  pfn = page_to_pfn(p);
  p_addr = PFN_PHYS(pfn);
  return __va(p_addr);
}

static int
thread_fn(void * data)
{
  unsigned int i, j, k;
  
  nr_structs_per_page = PAGE_SIZE / sizeof(datatype_t);
  nr_pages = nr_structs / nr_structs_per_page;
  nr_pages = (nr_structs % nr_structs_per_page != 0) ?
    nr_pages + 1 : nr_pages;
  order = my_get_order(nr_pages);
  
  pages = alloc_pages(GFP_KERNEL, order);
  if(pages == NULL){
    printk("Unable to allocate pages!\n");
  }
  else{
    
    for(i = 0; i < nr_pages; ++i){
      struct page * curr = pages + i;
      void * page_addr = get_virt_addr(curr);
      for(j = 0; j < nr_structs_per_page; ++j){
	datatype_t * curr_struct = (datatype_t *)(page_addr + j);
	for(k = 0; k < 8; ++k){
	  curr_struct->array[k] = i*nr_structs_per_page*8 + j*8 + k;
	}
      }
    }
    
  }
  
  while(!kthread_should_stop()) {
    schedule();
  }
  
  return 0;
}

static int
kernel_memory_init(void)
{
  printk(KERN_INFO "Loaded kernel_memory module\n");
  
  kthread = kthread_create(thread_fn, NULL, "k_memory");
  if (IS_ERR(kthread)) {
    printk(KERN_ERR "Failed to create kernel thread\n");
    return PTR_ERR(kthread);
  }
  
  wake_up_process(kthread);
  
  return 0;
}

static void 
kernel_memory_exit(void)
{
  unsigned int i, j, k;
  
  kthread_stop(kthread);
  
  for(i = 0; i < nr_pages; ++i){
    struct page * curr = pages + i;
    void * page_addr = page_address(curr);
    for(j = 0; j < nr_structs_per_page; ++j){
      datatype_t * curr_struct = (datatype_t *)(page_addr + j);
      for(k = 0; k < 8; ++k){
	unsigned int val = i*nr_structs_per_page*8 + j*8 + k;
	if(curr_struct->array[k] != val){
	  printk("Missed calculation for val: %u\n", val);
	}
      }
    }
  }
  
  __free_pages(pages, order);

  printk(KERN_INFO "Unloaded kernel_memory module\n\n");
}

module_init(kernel_memory_init);
module_exit(kernel_memory_exit);

MODULE_LICENSE ("GPL");
