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
#include <linux/mm_types.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>



//#include <stdio.h>

#define TMPSIZE 2000
#define s2fs_MAGIC 0x8BA491f6

MODULE_DESCRIPTION("My kernel module");
MODULE_AUTHOR("Jae-Won Jang");
MODULE_LICENSE("GPL");

static struct dentry *s2fs_mount(struct file_system_type *fs_type, int flags,
                       const char *dev_name, void *data);

void *recursive_add(struct task_struct *task, int space, struct super_block *sb_inp, struct dentry *rootd);
/*-------------------filesystem operations------------------*/
struct inode *s2fs_make_inode(struct super_block *sb,
			      int mode, int *pid)
{
  struct inode *inode = new_inode(sb);
  if (inode)
    {
      printk("pid %d\n", *pid);
      inode->i_ino = get_next_ino();
      inode->i_mode = mode;
      inode->i_sb = sb;
      inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
      inode->i_private = pid;
    }
  return inode;
}


static int get_task_info(int pid, char* data)
{
  int len;
  struct pid *struct_pid = find_get_pid(pid);
  struct task_struct *task_pid = pid_task(struct_pid, PIDTYPE_PID);
  printk("%p %p\n", (struct_pid), task_pid);
  if (task_pid != NULL || struct_pid != NULL)
    {
      //printk("Here %p %p\n", data, task_pid, struct_pid);
      len = snprintf(data, TMPSIZE, "Task Name: %s\nTask State: %ld\nPID: %d\nTGID: %d\nPPID: %d\nStart Time: %lld\nDynamic Priorty: %d\nStatic Priorty: %d\nNormal Priorty: %d\nRTime Priorty: %d\nMmap Base: %ld\nVMemory Space: %ld\nVMemory Usage: %d\nNo. of VMA: %d\nTotal Pages Mapped: %ld\n",
		     task_pid->comm ,
		     task_pid->state ,
		     task_pid_vnr(task_pid) ,
		     task_tgid_nr(task_pid) ,
		     task_tgid_vnr(rcu_dereference(task_pid->real_parent)) ,
		     task_pid->start_time ,
		     task_pid->prio ,
		     task_pid->static_prio ,
		     task_pid->normal_prio ,
		     task_pid->rt_priority ,
		     task_pid->mm->mmap_base ,
		     task_pid->mm->task_size ,
		     atomic_read(&(task_pid->mm->mm_count)) ,
		     task_pid->mm->map_count ,
		     task_pid->mm->total_vm
		     
		     );
      return len;      
    }
  else
    {
      printk("Hello..?\n");
      len = snprintf(data, TMPSIZE, "Task no longer exists\n");
      return -1;
      /*
	printk(KERN_INFO "Task Name: %s\n", task_pid->comm);
	printk(KERN_INFO "Task State: %ld\n", task_pid->state);
	printk(KERN_INFO "PID: %d\n", task_pid_vnr(task_pid));
	printk(KERN_INFO "TGID: %d\n", task_tgid_nr(task_pid));
	printk(KERN_INFO "PPID: %d\n", task_tgid_vnr(rcu_dereference(task_pid->real_parent)));
	printk(KERN_INFO "Start Time: %lld\n", task_pid->start_time);
	printk(KERN_INFO "Dynamic Priorty: %d\n", task_pid->prio );
	printk(KERN_INFO "Static Priorty: %d\n", task_pid->static_prio);
	printk(KERN_INFO "Normal Priorty: %d\n", task_pid->normal_prio);
	printk(KERN_INFO "Real-Time Priorty: %d\n", task_pid->rt_priority);
	printk(KERN_INFO "Memory Map Base: %ld\n", task_pid->mm->mmap_base);
	printk(KERN_INFO "Virtual Memory Space: %ld\n", task_pid->mm->task_size);
	printk(KERN_INFO "Virtual Memory Usage: %d\n", atomic_read(&(task_pid->mm->mm_count)));
	printk(KERN_INFO "No. of Virtual Memory Address: %d\n", task_pid->mm->map_count);
	printk(KERN_INFO "Total Pages Mapped: %ld\n", task_pid->mm->total_vm);
      */
    }
  

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
  int pid_input = 0;
  int result = 0;
  char *pid_num = filp->f_path.dentry->d_iname; // this returns pid number
  //struct qstr file_name = filp->f_path.dentry->d_name;  
  //len = snprintf(tmp, TMPSIZE, "Task name: %s\n", file_name); //getting the length of string
  //loff_t is the current reading position of the file
  result = kstrtoint(pid_num, 10, &pid_input);
  //printk("PID char read is: %s\n", pid_num);
  //printk("PID int read is %d %d\n", result, pid_input);

  //printk("Hello %d\n", pid_input);
  len = get_task_info(pid_input, tmp);

  if (*offset > len)
    return 0;
  if (count > len - *offset)
    count = len - *offset;
  //calculate the count which provides info of how long char string is and use that info for copy_to_user which will return the proper byte.
  
  if (copy_to_user(buf, tmp + *offset, count)) 
    return -EFAULT;
  *offset += count;
  //printk("Count is %ld\n", count);    
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

static struct dentry *s2fs_create_dir(struct super_block *sb, struct dentry *parent, const char *dir_name, int pid)
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
	//int test = 17;
        inode = s2fs_make_inode(sb, S_IFDIR | 0644, &pid); //user can read | write + group can read + other can read
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
					struct dentry *dir, const char *file_name, int pid)
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
	//	int test = 17;
	inode = s2fs_make_inode(sb, 0x8000, &pid); //0x8000 is a defined i_mode value for regular file.
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
  //struct dentry *sub_dentry;
  struct task_struct *task;
  //struct list_head *list;
  int space = 0;
  char file_name[10];
  int pid_input;
  task = &init_task;
  pid_input = (int)task->pid;
  snprintf(file_name, 10, "%i", task->pid);


  //  printk("Loading proctree Module...\n The process is \"%s\" (pid %i)\n", task->comm, task->pid);
    
  sb->s_magic = s2fs_MAGIC;
  sb->s_op = &s2fs_ops;  

  //  int test = 17;

  
  root = s2fs_make_inode(sb, S_IFDIR | 0755, &pid_input);
  if (!root)
    {
      pr_err("inode allocation failed\n");
      return -ENOMEM;
    }
  root->i_op = &simple_dir_inode_operations;
  root->i_fop = &simple_dir_operations;

  root_dentry = d_make_root(root);
  sb->s_root = root_dentry;
  
  if (!sb->s_root)
    {
      pr_err("root creation failed\n");
      return -ENOMEM;
    }
  //  sub_dentry = s2fs_create_dir(sb, root_dentry, file_name);
  recursive_add(task, space, sb, root_dentry);
  
  
  return 0;
}

void *recursive_add(struct task_struct *task, int space, struct super_block *sb_inp, struct dentry *rootd)
{
  struct list_head *list;
  struct task_struct *holder_task;
  struct dentry *sub_dentry;
  int i;
  char file_name[10];
  
  int space_num = space;
  snprintf(file_name, 10, "%i", task->pid);
  
  for(i = 1; i < space_num; i++)
    {
      printk(KERN_CONT "  ");
    }
  printk(KERN_CONT "-%s [%i]\n", task->comm, task->pid);
  space_num++;
  
  if(list_empty(&task->children))
    {
      //printk(KERN_CONT "This process has no children\n");
      s2fs_create_file(sb_inp, rootd, file_name, (int)task->pid);
    }
  else
    {
      sub_dentry = s2fs_create_dir(sb_inp, rootd, file_name, (int)task->pid);
      s2fs_create_file(sb_inp, sub_dentry, file_name, (int)task->pid);
      list_for_each(list, &task->children)    {
	//s2fs_create_file(sb_inp, sub_dentry, file_name);
     	holder_task = list_entry(list, struct task_struct, sibling);
	if(!(list_empty(&task->children)))
	  {
	    recursive_add(holder_task, space_num, sb_inp, sub_dentry);
	  }
      }
    }
  return 0; //remove this if part 1 doesn't work
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
