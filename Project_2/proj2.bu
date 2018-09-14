#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/hashtable.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jae-Won Jang");
MODULE_DESCRIPTION("Project 2");

static char *int_str;

module_param(int_str, charp, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(int_str, "A comma-separated list of integers");

static LIST_HEAD(mylist);

struct entry {
  int val;
  struct list_head list;
};

struct hentry {
  int val;
  struct hlist_node hashlist;
};

static int store_value(int val)
{
  struct entry *prj2_list;
  prj2_list = kmalloc(sizeof(*prj2_list), GFP_KERNEL);
  prj2_list->val = val;
  list_add_tail(&prj2_list->list, &mylist);
}

static int store_value_hash(int val)
{
  struct hentry *prj2_hlist;
  prj2_hlist = kmalloc(sizeof(*prj2_hlist), GFP_KERNEL);
  prj2_hlist->val = val;
  hash_add(myHash, prj2_hlist->next, prj2_hlist->val);
  //prj2_hlist->hashlist;  
}

static int parse_params(void)
{
  int val, err = 0;
  char *p, *orig, *params;

  params = kstrdup(int_str, GFP_KERNEL);
  if(!params)
    return -ENOMEM;
  orig = params;

  while ((p = strsep(&params, ",")) != NULL){
    if (!*p)
      continue;
    err = kstrtoint(p, 0, &val);
    if (err)
      break;
    err = store_value(val);
    if (err)
      break;
  }
  kfree(orig);
  return err;
}

static void test_linked_list(void)
{
  struct list_head *position = NULL;
  struct entry *dataptr = NULL;
  list_for_each(position, &mylist)
    {
      dataptr = list_entry(position, struct entry, list);
      printk("data in the list = %d\n", dataptr->val);
    }
}

static void destroy_linked_list_and_free(void)
{
  struct list_head *pos;
  struct list_head *next;
  struct entry *data;
  list_for_each_safe(pos, next, &mylist)
    {
      data = list_entry(pos, struct entry, list);
      list_del(pos);
      kfree(data);
    }
  
}

static void run_tests(void)
{
  test_linked_list();
  // add other tests here

}

static void cleanup(void)
{
  printk(KERN_INFO "\nCleaning up...\n");
  destroy_linked_list_and_free();
}

static int proj2_proc_show(struct seq_file *m, void *v)
{
  int err = 0;
  if (!int_str) {
    printk(KERN_INFO "Missing \'int_str\' parameter, exiting \n");
    return -1;
  }
  DEFINE_HASHTABLE(myHash, 5);
  printk("Hello v2!\n");
  err = parse_params();
  if (err)
    goto out;
  run_tests();
  
out:
  cleanup();
  return err;
}

static int proj2_proc_open(struct inode *inode, struct file *file) {
  return single_open(file, proj2_proc_show, NULL);
}

static const struct file_operations proj2_fops = {
       .owner = THIS_MODULE,
       .open = proj2_proc_open,
       .read = seq_read,
       .llseek = seq_lseek,
       .release = single_release,						   
};

static int __init prj2_init(void)
{
  proc_create("proj2", 0, NULL, &proj2_fops);
  return 0;
}

static void __exit prj2_exit(void)
{
  remove_proc_entry("proj2", NULL);
}


module_init(prj2_init);
module_exit(prj2_exit);
