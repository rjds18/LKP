#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/sched.h> //task_struct
#include <linux/sched/task.h>
#include <linux/init_task.h>
#include <linux/signal.h>
#include <linux/list.h>


MODULE_DESCRIPTION("My kernel module");
MODULE_AUTHOR("Jae-Won Jang");
MODULE_LICENSE("GPL");

void recursive_search(struct task_struct *task, int space)
{
  struct list_head *list;
  struct task_struct *holder_task;
  int space_num = space;
  int i;
  
  for(i = 1; i < space_num; i++)
    {
      printk(KERN_CONT "  ");
    }
  printk(KERN_CONT "-%s [%d]\n", task->comm, task->pid);
  space_num++;  
  list_for_each(list, &task->children)    {
    holder_task = list_entry(list, struct task_struct, sibling);
    recursive_search(holder_task, space_num);
  }  
}


static int proj4_proc_show(struct seq_file *m, void *v)
{
  struct task_struct *task = &init_task; // getting global current pointer to the swapper
  struct list_head *list;
  int space = 0;
  
  printk("Loading proctree Module...\n");
  recursive_search(task, space);
  return 0;
  
}

static int proj4_proc_open(struct inode *inode, struct file *file) {
  return single_open(file, proj4_proc_show, NULL);
}

static const struct file_operations proj4_fops = {
       .owner = THIS_MODULE,
       .open = proj4_proc_open,
       .read = seq_read,
       .llseek = seq_lseek,
       .release = single_release,
};

static int __init proj4_init(void)
{
  proc_create("proj4", 0, NULL, &proj4_fops);
  return 0;
}

static void __exit proj4_exit(void)
{
  printk("Bye World!\n");
  remove_proc_entry("proj4", NULL);
}

module_init(proj4_init);
module_exit(proj4_exit);
