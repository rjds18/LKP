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

#include <linux/fs.h>
#include <linux/pagemap.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

//#include <stdio.h>

#define TMPSIZE 17
#define s2fs_MAGIC 0x8BA491f6

MODULE_DESCRIPTION("My kernel module");
MODULE_AUTHOR("Jae-Won Jang");
MODULE_LICENSE("GPL");

static struct dentry *s2fs_mount(struct file_system_type *fs_type, int flags,
                       const char *dev_name, void *data);


/*-------------------filesystem operations------------------*/
struct inode *s2fs_make_inode(struct super_block *sb,
			     int mode)
{
  struct inode *inode = new_inode(sb);
  if (inode)
    {
      inode->i_ino = get_next_ino();
      inode->i_mode = mode;
      inode->i_sb = sb;
      inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    }
  return inode;
}

static int s2fs_open(struct inode *inode, struct file *filp)
{
  filp->private_data = inode->i_private;
  return 0;
}

static ssize_t s2fs_read_file(struct file *filp, char *buf, size_t count, loff_t *offset)
{
  int len;
  char tmp[TMPSIZE];
  char *hello = "Hello World!"; //string hello world
  
  len = snprintf(tmp, TMPSIZE, "%s\n", hello); //getting the length of string

  //loff_t is the current reading position of the file

  if (*offset > len)
    return 0;
  if (count > len - *offset)
    count = len - *offset;
  
  
  //calculate the count which provides info of how long char string is and use that info for copy_to_user which will return the proper byte.
  
  if (copy_to_user(buf, tmp + *offset, count)) 
    return -EFAULT;
  *offset += count;

  printk("Count is %ld\n", count);
      
  return count;

}

static ssize_t s2fs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset)
{
  atomic_t *counter = (atomic_t *) filp->private_data;
  char tmp[TMPSIZE];

  if (*offset != 0)
    return -EINVAL;

  if (count >= TMPSIZE)
    return -EINVAL;

  memset(tmp, 0, TMPSIZE);
  if (copy_from_user(tmp, buf, count))
    return -EFAULT;
    
  atomic_set(counter, simple_strtol(tmp, NULL, 10));
  return count;
}

static struct file_operations s2fs_fops = {
       .open = s2fs_open,
       .read = s2fs_read_file,
       .write = s2fs_write_file,
};

static struct dentry *s2fs_create_dir(struct super_block *sb, struct dentry *parent, const char *dir_name)
{
        struct dentry *dentry = parent;
        struct inode *inode;
        struct qstr qname; //quick string

        qname.name = dir_name;
        qname.len = strlen (dir_name);
        qname.hash = full_name_hash(dentry, dir_name, qname.len); //salt, name, length
        dentry = d_alloc(dentry, &qname);
        if (! dentry)
          goto out;

        inode = s2fs_make_inode(sb, S_IFDIR | 0644); //user can read | write + group can read + other can read
        if (! inode)
          goto out_dput;
        inode->i_op = &simple_dir_inode_operations;
        inode->i_fop = &simple_dir_operations;

        d_add(dentry, inode);
        return dentry;

 out_dput:
        dput(dentry);
 out:
        return 0;
}


static struct dentry *s2fs_create_file (struct super_block *sb,
		struct dentry *dir, const char *file_name)
{
	struct dentry *dentry = dir;
	struct inode *inode;
	struct qstr qname; //quick string
	qname.name = file_name;
	qname.len = strlen (file_name);
	qname.hash = full_name_hash(dentry, file_name, qname.len); //same business as before.
	
	dentry = d_alloc(dentry, &qname); //allocate a dcache entry
	if (!dentry)
		goto out;
	inode = s2fs_make_inode(sb, 0x8000); //0x8000 is a defined i_mode value for regular file.
	if (!inode)
		goto out_dput;
	inode->i_fop = &s2fs_fops;
	
	d_add(dentry, inode);
	return dentry;
	
  out_dput:
	dput(dentry); //removing a dentry if things go wrong (condition provided above)
  out:
	return 0;
}


/*---------------------superblock stuff------------------------*/
static void s2fs_put_super(struct super_block *sb)
{
  pr_debug("s2fs super block destroyed\n");
}

static const struct super_operations s2fs_ops = {
						 .statfs = simple_statfs,
						 .drop_inode = generic_delete_inode,
						 .put_super = s2fs_put_super,
};

static int s2fs_fill_super(struct super_block *sb, void *data, int silent)
{
  struct inode *root;

  struct dentry *root_dentry;
  struct dentry *sub_dentry;
  
  sb->s_magic = s2fs_MAGIC;
  sb->s_op = &s2fs_ops;  

  root = s2fs_make_inode(sb, S_IFDIR | 0755);
  if (!root)
    {
      pr_err("inode allocation failed\n");
      return -ENOMEM;
    }
  root->i_op = &simple_dir_inode_operations;
  root->i_fop = &simple_dir_operations;

  root_dentry = d_make_root(root);
  sb->s_root = root_dentry;

  //sb->s_root = d_make_root(root);
  if (!sb->s_root)
    {
      pr_err("root creation failed\n");
      return -ENOMEM;
    }


  sub_dentry = s2fs_create_dir(sb, root_dentry, "foo");
  if (sub_dentry)
    {
      printk("in the if loop\n");
      s2fs_create_file(sb, sub_dentry, "bar");
    }
  
  return 0;
}

static struct dentry *s2fs_mount(struct file_system_type *fs_type, int flags,
                       const char *dev_name, void *data)
{
  struct dentry *const entry = mount_nodev(fs_type, flags, data, s2fs_fill_super);
  if (IS_ERR(entry))
    pr_err("s2fs mounting failed\n");
  else
    pr_debug("s2fs mounted\n");
  return entry;
}

static struct file_system_type s2fs_type = {
					       .owner = THIS_MODULE,
					       .name  = "s2fs",
					       .mount = s2fs_mount,
					       .kill_sb = kill_litter_super,
					       .fs_flags = FS_REQUIRES_DEV,
};



/* -------------------------part 1------------------------------ */
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
/*
static int part1_proc_show(struct seq_file *m, void *v)
{
  struct task_struct *task = &init_task; // getting global current pointer to the swapper
  int space = 0;  
  printk("Loading proctree Module...\n");
  recursive_search(task, space);
  return 0;
}

static int part1_proc_open(struct inode *inode, struct file *file) {
  return single_open(file, part1_proc_show, NULL);
}
*/

static int __init s2fs_init(void)
{
  return register_filesystem(&s2fs_type);
  //proc_create("s2fs", 0, NULL, &s2fs_fops);
  //return 0;
}

static void __exit s2fs_exit(void)
{
  printk("Bye World!\n");
  //remove_proc_entry("s2fs", NULL);
  unregister_filesystem(&s2fs_type);
}

module_init(s2fs_init);
module_exit(s2fs_exit);
