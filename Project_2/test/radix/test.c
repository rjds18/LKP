#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/radix-tree.h>
#include <linux/slab.h>
#include <linux/gfp.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jae-Won");

RADIX_TREE(radix_tree, GFP_NOIO);

struct mystruct {
  struct radix_tree_node radix_node;
  int data;
};

static int radix_insert(int key)
{
  struct mystruct *rnode;
  rnode = kmalloc(sizeof(*rnode), GFP_KERNEL);
  rnode-> data = key;
  radix_tree_insert(&radix_tree, key, rnode);
  return 0;
}
static void radix_remove(void)
{
  void **slot;
  struct radix_tree_iter iter;
  radix_tree_for_each_slot(slot, &radix_tree, &iter, 0)
   {
     radix_tree_delete(&radix_tree, iter.index);
   }
}

static void view_radix(void)
{
  void **slot;
  struct radix_tree_iter iter;
  struct mystruct *temp = kmalloc(sizeof(*temp), GFP_KERNEL);
  radix_tree_for_each_slot(slot, &radix_tree, &iter, 0)
   {
     temp = radix_tree_lookup(&radix_tree, iter.index);
     if(temp == NULL)
       {
	 return;
       }
     else{
       printk(KERN_CONT "here is the digit: %d\n", temp->data);
     }
   }
}

static int __init test_init(void)
{
  int err;
  void **slot;

  err = radix_insert(1);
  err = radix_insert(2);
  err = radix_insert(3);
  err = radix_insert(4);
  view_radix();

  
  return 0;
}

static void __exit test_exit(void)
{
  printk(KERN_INFO "Goodbye\n");
}

module_init(test_init);
module_exit(test_exit);
