#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define COUNTFS_MAGIC 0x20160408
#define TMPSIZE 12

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luis de Bethencourt");

static struct dentry *countfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data);
static int countfs_create_file(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl);
static ssize_t countfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset);
static ssize_t countfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset);

static struct file_system_type countfs_type = {
	.owner		= THIS_MODULE,
	.name		= "countfs",
	.mount		= countfs_mount,
	.kill_sb	= kill_litter_super,
	.fs_flags	= FS_USERNS_MOUNT,
};

static const struct super_operations countfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static const struct file_operations countfs_file_ops = {
	.open		= simple_open,
	.read		= countfs_read_file,
	.write		= countfs_write_file,
};

static const struct inode_operations countfs_dir_inode_ops = {
	.create		= countfs_create_file,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.rmdir		= simple_rmdir,
	.rename		= simple_rename,
};

static struct inode *countfs_get_inode(struct super_block *sb,
		const struct inode *dir, umode_t mode, dev_t dev)
{
	atomic_t *counter = NULL;
	struct inode *ret = new_inode(sb);

	pr_info("cfs: get_inode\n");

	if (ret) {
		ret->i_mode = mode;
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = CURRENT_TIME;

		counter = kmalloc(sizeof(counter), GFP_KERNEL);
		atomic_set(counter, 0);

		if (mode & S_IFREG) {
			pr_info("cfs: get_inode: creating a regular file\n");

			ret->i_fop = &countfs_file_ops;
			ret->i_private = counter;
		} else {
			pr_info("cfs: get_inode: creating a directory\n");

			ret->i_op = &countfs_dir_inode_ops;
			ret->i_fop = &simple_dir_operations;
		}
	}

	return ret;
}

static int countfs_fill_super(struct super_block *sb, void *data, int silent)
{
	static struct tree_descr toy_files[] = { {""} };
	struct inode *inode;
	int err;

	pr_info("cfs: fill_super\n");

	err = simple_fill_super(sb, COUNTFS_MAGIC, toy_files);
	if (err)
		return err;

	sb->s_op = &countfs_s_ops;

	inode = countfs_get_inode(sb, NULL, S_IFDIR, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

static int countfs_create_file(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl)
{
	struct inode *inode;

	pr_info("cfs: create_file: %s\n", dentry->d_name.name);

	inode = countfs_get_inode(dir->i_sb, dir, mode | S_IFREG, 0);
	if (!inode)
		return -ENOSPC;

	d_instantiate(dentry, inode);
	dget(dentry);
	dir->i_mtime = dir->i_ctime = CURRENT_TIME;

	return 0;
}

static struct dentry *countfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	pr_info("cfs: mount\n");

	return mount_nodev(fs_type, flags, data, countfs_fill_super);
}

static ssize_t countfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	int c, len;
	char tmp[TMPSIZE];
	atomic_t *counter = (atomic_t *) filp->private_data;

	pr_info("cfs: read\n");

	c = atomic_read(counter);
	atomic_inc(counter);

	len = snprintf(tmp, TMPSIZE, "%d\n", c);
	return simple_read_from_buffer(buf, count, offset, tmp, len);
}

static ssize_t countfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	char tmp[TMPSIZE];
	atomic_t *counter = (atomic_t *) filp->private_data;
	long val;
	int err;

	pr_info("cfs: write\n");

	simple_write_to_buffer(tmp, TMPSIZE, offset, buf, count);
	tmp[count] = '\0';    /* ktrstrol needs a null terminated string */
	err = kstrtol(tmp, 10, &val);
	if (!err) {
		atomic_set(counter, val);
		pr_info("cfs: count is now %ld\n", val);
	}

	return count;
}

static int __init init_countfs(void)
{
	pr_info("Loading countfs\n");
	return register_filesystem(&countfs_type);
}

static void __exit exit_countfs(void)
{
	pr_info("Closing countfs\n");
	unregister_filesystem(&countfs_type);
}

module_init(init_countfs);
module_exit(exit_countfs);
