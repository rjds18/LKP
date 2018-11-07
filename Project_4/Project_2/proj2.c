#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/types.h>
#include <linux/hashtable.h>
#include <linux/rbtree.h>
#include <linux/radix-tree.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jae-Won Jang");
MODULE_DESCRIPTION("Project 2");

static char *int_str;

module_param(int_str, charp, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(int_str, "A comma-separated list of integers");

static LIST_HEAD(mylist);
static DEFINE_HASHTABLE(myHTable, 3);
struct rb_root root = RB_ROOT;
RADIX_TREE(radix_tree, GFP_NOIO);

struct entry {
  int val;
  struct list_head list;
  struct hlist_node hash;
  struct rb_node rbnode;
  struct radix_tree_node radix_node;
};

static int store_value(int val)
{
  struct entry *prj2_list;
  prj2_list = kmalloc(sizeof(*prj2_list), GFP_KERNEL);
  prj2_list->val = val;
  list_add_tail(&prj2_list->list, &mylist);
  return 0;
}

static int store_hash_value(int val){

  struct entry *myHash;
  myHash = kmalloc(sizeof(*myHash), GFP_KERNEL);
  myHash->val = val;
  hash_add(myHTable, &myHash->hash, myHash->val);
  return 0;
}

int insert_rb_tree(struct rb_root *tree_root, struct entry *insertion)
{
  struct rb_node **new = &(tree_root->rb_node);
  struct rb_node *parent = NULL;

  while (*new){
    struct entry *this = container_of(*new, struct entry, rbnode);
    parent = *new;
    if (this->val > insertion->val)
      {
        new = &(*new)->rb_left;
      }
    else if (this->val < insertion->val)
      {
        new = &(*new)->rb_right;
      }
    else
      {
        return -1;
      }
  }
  rb_link_node(&insertion->rbnode, parent, new);
  rb_insert_color(&insertion->rbnode, tree_root);
  return 0;
}

static int rb_tree_insert(int value)
{                                  
  struct entry *target = kmalloc(sizeof(*target), GFP_KERNEL);
  target->val = value;
  insert_rb_tree(&root, target);
  return 0;
}

static int radix_insert(int key)
{
  struct entry *rnode;
  rnode = kmalloc(sizeof(*rnode), GFP_KERNEL);
  rnode-> val = key;
  radix_tree_insert(&radix_tree, key, rnode);
  return 0;
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
    err = store_hash_value(val);
    err = rb_tree_insert(val);
    err = radix_insert(val);
    if (err)
      break;
  }
  kfree(orig);
  return err;
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
static void remove_hash_table(void){
  int bucket;
  struct entry *pos;
  struct hlist_node *tmp = NULL;
  hash_for_each_safe(myHTable, bucket, tmp, pos, hash)
    {
      hash_del(&pos->hash);
      kfree(pos);
    }
  kfree(tmp);
}

static void rb_tree_remove(struct rb_root *root)
{
  struct rb_node *iternode;
  for (iternode = rb_first(root); iternode; iternode = rb_next(iternode))
    {
      struct entry *temp = rb_entry(iternode, struct entry, rbnode); 
      rb_erase(&(temp->rbnode), root);
      kfree(temp);
    }
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

static void cleanup(void)
{
  printk(KERN_INFO "\nCleaning up...\n");
  destroy_linked_list_and_free();
  remove_hash_table();
  rb_tree_remove(&root);
  radix_remove();
}

static int proj2_proc_show(struct seq_file *m, void *v)
{
  int err = 0;
  struct list_head *position = NULL;
  struct entry *dataptr = NULL;
  int bucket;
  struct entry *curpos;
  struct rb_node *iternode;  
  void **slot;
  struct radix_tree_iter iter;
  struct entry *temp2 = kmalloc(sizeof(*temp2), GFP_KERNEL);
  
  if (!int_str) {
    printk(KERN_INFO "Missing \'int_str\' parameter, exiting \n");
    return -1;
  }
  err = parse_params();
  if (err)
    goto out;
  //  run_tests();
  seq_printf(m, "Linked List = ");
  list_for_each(position, &mylist)
    {
      dataptr = list_entry(position, struct entry, list);
      //printk(KERN_CONT "%d ", dataptr->val);
      seq_printf(m, "%d ", dataptr->val);
    }
  seq_printf(m, "\n");

  seq_printf(m, "Hash Table = ");
  hash_for_each(myHTable, bucket, curpos, hash)
    {
      // printk(KERN_CONT "%d ", curpos->val);
      seq_printf(m, "%d ", curpos->val);
    }
  seq_printf(m,"\n");
  seq_printf(m, "R-B Tree = ");
  for (iternode = rb_first(&root); iternode; iternode = rb_next(iternode))
    {
      struct entry *temp = rb_entry(iternode, struct entry, rbnode);       
      //      printk(KERN_CONT "%d ", temp->val);
      seq_printf(m, "%d ", temp->val);
    }
  seq_printf(m, "\n");
  seq_printf(m, "Radix Tree = ");
  radix_tree_for_each_slot(slot, &radix_tree, &iter, 0)
   {
     temp2 = radix_tree_lookup(&radix_tree, iter.index);
     if(temp2 == NULL)
       {
         return -1;
       }
     else{
       //       printk(KERN_CONT "%d ", temp->val);
       seq_printf(m, "%d ", temp2->val);
     }
   }
  seq_printf(m, "\n");
  // run_tests();
  

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
