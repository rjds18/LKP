#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hashtable.h>
#include <linux/slab.h>



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jae-Won");

static DEFINE_HASHTABLE(a, 3);

struct mystruct {
  int data;
  struct hlist_node hash;
};

static int __init test_init(void)
{
  struct mystruct *first, *second, *third;
  int bucket;
  int key = 1;
  first = kmalloc(sizeof(*first), GFP_KERNEL);
  first->data = 8;
  hash_add(a, &first->hash, first->data);

  first->data = 20;
  hash_add(a, &first->hash, first->data);
 
  first->data = 1;
  hash_add(a, &first->hash, first->data);
  
  /*
  second = kmalloc(sizeof(*second), GFP_KERNEL);
  second->data = 17;
  hash_add(a, &second->hash, second->data);

  third = kmalloc(sizeof(*third), GFP_KERNEL);
  third->data = 27;
  hash_add(a, &third->hash, third->data);
  */
  
 
  hash_for_each_possible(a, first, hash, key)
    {
       printk(KERN_INFO "hash possible data=%d is in %d\n", key, first->data);
    }
  hash_for_each(a, bucket, first, hash)
    {
      printk(KERN_INFO "data=%d is in bucket %d\n", first->data, bucket);
    }
  
  kfree(first);
  
  //kfree(second);
  //kfree(third);
}

static void __exit test_exit(void)
{
  printk(KERN_INFO "Goodbye\n");
}

module_init(test_init);
module_exit(test_exit);
