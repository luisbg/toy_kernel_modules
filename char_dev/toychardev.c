#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define ID "toy_char_dev"
#define ID_SIZE 12

static struct miscdevice toy_dev;

static ssize_t toy_read(struct file *unused, char __user *buf,
			 size_t count, loff_t *ppos)
{
	/* Give string to user
	 * Convenient wrapper of copy_to_user()
	 */
	return simple_read_from_buffer(buf, count, ppos, ID,
				       strlen(ID));
}

ssize_t toy_write(struct file *unused, const char __user *buf, size_t count,
		   loff_t *f_pos)
{
	char tmp[ID_SIZE + 1];
	int ret;

	/* Get string from user
	 * Convenient wrapper of copy_from_user()
	 */
	ret = simple_write_to_buffer(tmp, ID_SIZE, f_pos, buf, count);
	tmp[ID_SIZE] = '\0';
	if (ret < 0)
		return ret;

	/* Check if strings match */
	if (strcmp(tmp, ID) != 0)
		return -EINVAL;

	pr_err("misc: equal strings\n");
	return ret + 1;
}

static const struct file_operations toy_fops = {
	.owner = THIS_MODULE,
	.read = toy_read,
	.write = toy_write,
};

static struct miscdevice toy_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "toychardev",
	.fops = &toy_fops,
};

static int __init toy_char_dev_init(void)
{
	pr_err("toychardev: init\n");
	return misc_register(&toy_dev);
}

static void __exit toy_char_dev_exit(void)
{
	pr_err("toychardev: exit\n");
	misc_deregister(&toy_dev);
}

module_init(toy_char_dev_init);
module_exit(toy_char_dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luis de Bethencourt");
MODULE_DESCRIPTION("Toy Char Device Driver");
