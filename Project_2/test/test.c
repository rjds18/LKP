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
  struct mystruct *first, *second;
  int bucket;
  first = kmalloc(sizeof(*first), GFP_KERNEL);
  first->data = 8;
  hash_add(a, &first->hash, first->data);

  second = kmalloc(sizeof(*second), GFP_KERNEL);
  second->data = 17;
  hash_add(a, &second->hash, second->data);

  
  hash_for_each(a, bucket, first, hash)
    {
      printk(KERN_INFO "data=%d is in bucket %d\n", first->data, bucket);
    }
  kfree(first);
  kfree(second);
}

static void __exit test_exit(void)
{
  printk(KERN_INFO "Goodbye\n");
}

module_init(test_init);
module_exit(test_exit);
