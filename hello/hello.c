#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luis de Bethencourt");

static int __init mod_init(void)
{
	printk(KERN_ALERT "Hello world!\n");
	return 0;
}

static void __exit mod_exit(void)
{
	printk(KERN_ALERT "Goodbye\n");
}

module_init(mod_init);
module_exit(mod_exit);
