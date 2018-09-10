#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

static char *int_str;

/* [X1: point 1]
 * Explain following in here.
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("[YOUR NAME]");
MODULE_DESCRIPTION("LKP Exercise 4");

/* [X2: point 1]
 * Explain following in here.
 */
module_param(int_str, charp, S_IRUSR | S_IRGRP | S_IROTH);

/* [X3: point 1]
 * Explain following in here.
 */
MODULE_PARM_DESC(int_str, "A comma-separated list of integers");

/* [X4: point 1]
 * Explain following in here.
 */
static LIST_HEAD(mylist);

/* [X5: point 1]
 * Explain following in here.
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
}

static void test_linked_list(void)
{
	/* [X7: point 10]
	 * Print out value of all entries in mylist.
	 */
}


static void destroy_linked_list_and_free(void)
{
	/* [X8: point 10]
	 * Free all entries in mylist.
	 */
}


static int parse_params(void)
{
	int val, err = 0;
	char *p, *orig, *params;


	/* [X9: point 1]
	 * Explain following in here.
	 */
	params = kstrdup(int_str, GFP_KERNEL);
	if (!params)
		return -ENOMEM;
	orig = params;

	/* [X10: point 1]
	 * Explain following in here.
	 */
	while ((p = strsep(&params, ",")) != NULL) {
		if (!*p)
			continue;
		/* [X11: point 1]
		 * Explain following in here.
		 */
		err = kstrtoint(p, 0, &val);
		if (err)
			break;

		/* [X12: point 1]
		 * Explain following in here.
		 */
		err = store_value(val);
		if (err)
			break;
	}

	/* [X13: point 1]
	 * Explain following in here.
	 */
	kfree(orig);
	return err;
}

static void run_tests(void)
{
	/* [X14: point 1]
	 * Explain following in here.
	 */
	test_linked_list();
}

static void cleanup(void)
{
	/* [X15: point 1]
	 * Explain following in here.
	 */
	printk(KERN_INFO "\nCleaning up...\n");

	destroy_linked_list_and_free();
}

static int __init ex4_init(void)
{
	int err = 0;

	/* [X16: point 1]
	 * Explain following in here.
	 */
	if (!int_str) {
		printk(KERN_INFO "Missing \'int_str\' parameter, exiting\n");
		return -1;
	}

	/* [X17: point 1]
	 * Explain following in here.
	 */
	err = parse_params();
	if (err)
		goto out;

	/* [X18: point 1]
	 * Explain following in here.
	 */
	run_tests();
out:
	/* [X19: point 1]
	 * Explain following in here.
	 */
	cleanup();
	return err;
}

static void __exit ex4_exit(void)
{
	/* [X20: point 1]
	 * Explain following in here.
	 */
	return;
}

/* [X21: point 1]
 * Explain following in here.
 */
module_init(ex4_init);

/* [X22: point 1]
 * Explain following in here.
 */
module_exit(ex4_exit);
