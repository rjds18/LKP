#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/errno.h>

static char *int_str;

/* [X1: point 1]
 * When system programmers want to add new functions to the Linux kernel, instead of bloating 
 * the codespace with seldomly used functions, they tend to rely on the concept of Modules.
 * There are several ethical practices which need to be followed in order to prevent tainting
 * the kernel, to enforce this (use GPL!), each module developer is required to specify in their 
 * module, the source code the type of their used license > hence MODULE_LICENSE is used.
 * MODULE_AUTHOR and MODULE_DESCRIPTION are additional information to describe who wrote this 
 * module and description for the others to read.
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("[YOUR NAME]");
MODULE_DESCRIPTION("LKP Exercise 4");

/* [X2: point 1]
 * Unlike typical programming where we use argc / argv to pass command line arguments, in this case
 * we have to use the module_param() macro. 
 * The format in this case is:
 * 1st parameter = parameter name
 * 2nd parameter = data type (character pointer = charp)
 * 3rd parameter = permissions bits.
 */
module_param(int_str, charp, S_IRUSR | S_IRGRP | S_IROTH);

/* [X3: point 1]
 * this is basically describing what argument that this module can take.
 */
MODULE_PARM_DESC(int_str, "A comma-separated list of integers");

/* [X4: point 1]
 * this basically creates a list_head titled "mylist"
 * this is defined in the list.h as this:
 * #define LIST_HEAD(name) \ 
 *        struct list_head name = LIST_HEAD_INIT(name)
 */
static LIST_HEAD(mylist);

/* [X5: point 1]
 * creating an internal structure of entry which will contain the (val) for each string's value
 * and store the list of all parsed strings.
 */
struct entry {
	int val;
	struct list_head list;
};

static int store_value(int val)
{

	/* [X6: point 10]
	 * Allocate a struct entry of which val is val
	 * and add it to the tail of mylist.
	 * Return 0 if everything is successful.
	 * Otherwise (e.g., memory allocation failure),
	 * return corresponding error code in error.h (e.g., -ENOMEM).
	 */
  struct entry *ex4_list;
  ex4_list = kmalloc(sizeof(*ex4_list), GFP_KERNEL);
  ex4_list->val = val;
  list_add_tail(&ex4_list->list, &mylist);
}

static void test_linked_list(void)
{
	/* [X7: point 10]
	 * Print out value of all entries in mylist.
	 */
  struct list_head *position = NULL;
  struct entry *dataptr = NULL;
  list_for_each( position, &mylist)
    {
      dataptr = list_entry(position, struct entry, list);
      printk("data in the list = %d\n", dataptr->val);
    }
  
}


static void destroy_linked_list_and_free(void)
{
	/* [X8: point 10]
	 * Free all entries in mylist.
	 */
  struct list_head *pos;
  struct list_head *next;
  struct entry *data;
  list_for_each_safe(pos, next, &mylist)
    {
      data = list_entry(pos, struct entry, list);
      list_del(pos);
      
      kfree(data);
      kfree(pos);
    }

  
}


static int parse_params(void)
{
	int val, err = 0;
	char *p, *orig, *params;


	/* [X9: point 1]
	 * kstrdup is used to allocate space for and copy an existing string. In this particular 
	 * case, it is understanding the input pointer (int_str) and copying that into the other
	 * character pointer called params. This is done so that we can use params to parse.
	 */
	params = kstrdup(int_str, GFP_KERNEL);
	if (!params)
		return -ENOMEM;
	orig = params;

	/* [X10: point 1]
	 * this particular while condition is simply dereferencing the character pointer input
	 * which was obtained from int_str and looping until comma doesn't appear anymore.
	 */
	while ((p = strsep(&params, ",")) != NULL) {
		if (!*p)
			continue;
		/* [X11: point 1]
		 * kstrtoint is used to convert a string to int (str_to_int). basically this proj's
		 * goal is to parse string per comma and storing int into that value, so this is
		 * the part where that magic happens. It will convert particular string to int
		 * and then when it is successful, we will store that value into the val input
		 * which was previously initialized. However, if (err) is raised to true, then
		 * we will break out of the loop as we have detected the error. kstrtoint should
		 * return 0 on success.
		 */
		err = kstrtoint(p, 0, &val);
		if (err)
			break;

		/* [X12: point 1]
		 * Assuming everything happened well previosuly , store_value function will 
		 * copy the value variable into the entry structure we defined before. Otherwise
		 * if something bad happens, we will break out of the loop and throw an error.
		 */
		err = store_value(val);
		if (err)
			break;
	}

	/* [X13: point 1]
	 * kfree is used to free up the allocated memory (remove dangling pointers)
	 */
	kfree(orig);
	return err;
}

static void run_tests(void)
{
	/* [X14: point 1]
	 * this will run the function test_linked_list() function in order to run tests for 
	 * our defined linked list.
	 */
	test_linked_list();
}

static void cleanup(void)
{
	/* [X15: point 1]
	 * printk simply prints the message into the dmesg for the debugging purposes and KERN_INFO
	 * is an informational message (one of the eight different log levels)
	 */
	printk(KERN_INFO "\nCleaning up...\n");

	destroy_linked_list_and_free();
}

static int __init ex4_init(void)
{
	int err = 0;

	/* [X16: point 1]
	 * this condition checks whether during the insmod phase, int_str parameter is inserted
	 * or not, if not inserted, then it throws this error.
	 */
	if (!int_str) {
		printk(KERN_INFO "Missing \'int_str\' parameter, exiting\n");
		return -1;
	}

	/* [X17: point 1]
	 * as insmod is now initialized, we will call our previously defined parse_params function
	 * and if this returns anything other than 0 (which means successful), we will forcefully
	 * change the jump into "out" macro which is defined below (basically exiting)
	 */
	err = parse_params();
	if (err)
		goto out;

	/* [X18: point 1]
	 * We will now run the tests to check the linked list.
	 */
	run_tests();
out:
	/* [X19: point 1]
	 * In the case we get an error from the previous functions, we will forcefully jump into
	 * this branch where we will clean up the mess and return an err variable to debug
	 */
	cleanup();
	return err;
}

static void __exit ex4_exit(void)
{
	/* [X20: point 1]
	 * this is needed in order to be called when rmmod is used to remove the module.
	 */
	return;
}

/* [X21: point 1]
 * macro which will inititate the kernel module upon getting called insmod
 */
module_init(ex4_init);

/* [X22: point 1]
 * macro which will exit / remove kernel module upon getting called via rmmod
 */
module_exit(ex4_exit);
