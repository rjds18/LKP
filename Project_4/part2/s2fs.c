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

#define s2fs_MAGIC 0x8BA491f6

MODULE_DESCRIPTION("My kernel module");
MODULE_AUTHOR("Jae-Won Jang");
MODULE_LICENSE("GPL");

static struct dentry *s2fs_mount(struct file_system_type *fs_type, int flags,
                       const char *dev_name, void *data);

struct inode *s2fs_get_inode(struct super_block *sb,
			     const struct inode *dir, umode_t mode, dev_t dev);

static void s2fs_put_super(struct super_block *sb)
{
  pr_debug("s2fs super block destroyed\n");
}

static const struct super_operations s2fs_ops = {
						 .put_super = s2fs_put_super,
};

static int s2fs_fill_super(struct super_block *sb, void *data, int silent)
{
  struct inode *root = NULL;

  sb->s_magic = s2fs_MAGIC;
  sb->s_op = &s2fs_ops;  

  root = s2fs_get_inode(sb, NULL, 0, 0);
  if (!root)
    {
      pr_err("inode allocation failed\n");
      return -ENOMEM;
    }
  sb->s_root = d_make_root(root);
  if (!sb->s_root)
    {
      pr_err("root creation failed\n");
      return -ENOMEM;
    }  
  return 0;
}

static struct dentry *s2fs_mount(struct file_system_type *fs_type, int flags,
                       const char *dev_name, void *data)
{
  return mount_bdev(fs_type, flags, dev_name, data, s2fs_fill_super);
}

struct inode *s2fs_get_inode(struct super_block *sb,
			     const struct inode *dir, umode_t mode, dev_t dev)
{
  struct inode *inode = new_inode(sb);
  if (inode)
    {
      inode->i_ino = 0;
      inode->i_sb = sb;
      inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
      inode_init_owner(inode, NULL, S_IFDIR);
    }
  return inode;
}

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
static int part1_proc_show(struct seq_file *m, void *v)
{
  struct task_struct *task = &init_task; // getting global current pointer to the swapper
  struct list_head *list;
  int space = 0;  
  printk("Loading proctree Module...\n");
  recursive_search(task, space);
  return 0;
}

static int part1_proc_open(struct inode *inode, struct file *file) {
  return single_open(file, part1_proc_show, NULL);
}

static struct file_system_type s2fs_fs_type = {
					       .owner = THIS_MODULE,
					       .name  = "s2fs",
					       .mount = s2fs_mount,
					       .kill_sb = kill_block_super,
					       .fs_flags = FS_REQUIRES_DEV,
};

static const struct file_operations s2fs_fops = {
       .owner = THIS_MODULE,
       .open = part1_proc_open,
       .read = seq_read,
       .llseek = seq_lseek,
       .release = single_release,
};

static int __init proj4_init(void)
{
  proc_create("s2fs", 0, NULL, &s2fs_fops);
  return 0;
}

static void __exit proj4_exit(void)
{
  printk("Bye World!\n");
  remove_proc_entry("s2fs", NULL);
}

module_init(proj4_init);
module_exit(proj4_exit);
