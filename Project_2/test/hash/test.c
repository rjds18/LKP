#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hashtable.h>
#include <linux/slab.h>



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jae-Won");

static DEFINE_HASHTABLE(myHTable, 3);

struct mystruct {
  int data;
  struct hlist_node hash;
};

static int store_hash_value(int val){

  struct mystruct *myHash;
  myHash = kmalloc(sizeof(*myHash), GFP_KERNEL);
  myHash->data = val;
  hash_add(myHTable, &myHash->hash, myHash->data);
}

static void test_hash_table(void){
  int i;
  int bucket;
  struct mystruct *curpos;
  hash_for_each(myHTable, bucket, curpos, hash)
    {
      printk("%d ", curpos->data);
    }
  printk("\n");
  
}

static void remove_hash_table(void){
  int bucket;
  struct mystruct *pos;
  struct hlist_node *tmp = NULL;
  hash_for_each_safe(myHTable, bucket, tmp, pos, hash)
    {
      hash_del(&pos->hash);
      kfree(pos);
    }
  kfree(tmp);
}


static int __init test_init(void)
{
  store_hash_value(1);
  store_hash_value(2);
  store_hash_value(3);
  store_hash_value(4);
  store_hash_value(5);
  test_hash_table();
  remove_hash_table();

}

static void __exit test_exit(void)
{
  printk(KERN_INFO "Goodbye\n");
}

module_init(test_init);
module_exit(test_exit);
