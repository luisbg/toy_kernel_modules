#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luis de Bethencourt");

static int __init hello_init(void)
{
	int x;

	pr_debug("Hello world!\n");

	for (x = 0; x < 5; x++) {
		udelay(1000);
		pr_debug("hello: nice sleep for 1000 microseconds\n");
	}

	return 0;
}

static void __exit hello_exit(void)
{
	pr_debug("Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);
