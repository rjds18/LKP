#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
//#include <string.h>
#include "q2.h"

SYSCALL_DEFINE2(s2_encrypt, const char __user *, textval, int, key)
{
  return sys_s2_encrypt(text, key);
}


asmlinkage long sys_s2_encrypt(const char __user *strval, int keyval)
{
  unsigned long length;
  unsigned int retval;
  char *userdata;
  length = strnlen_user(strval, 1024);
  printk("length value: %ld\n", length);
  userdata = (char *)kmalloc(length, GFP_KERNEL);
  
  //int *outputint;
  printk("Hello World V11\n");  
  
  retval = copy_from_user(userdata, strval, length);
  if (retval == 0)
    {
      printk("Yay\n");
    }
  else
    {
      printk("Boo\n");
    }
  printk("String is %s\n", userdata);
  kfree(userdata);
  
    /*    printk("failed copy from user string\n");
    return -EFAULT;
  }
  if(copy_from_user(outputint, keyval, sizeof(outputint))){
    printk("failed copy from user int\n");
    return -EFAULT;
  }
  
  if(!outputstr)
    {
      printk("failed\n");
    }
  else
    {
      printk("succeeded\n");
    }
    */


  return 0;
}
