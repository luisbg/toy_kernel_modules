#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/atomic.h>
#include <linux/uaccess.h>

#define TOYFS_MAGIC 0x20160408
#define TMPSIZE 12

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luis de Bethencourt");

static struct dentry *toyfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data);
static struct dentry *toyfs_create_file(struct super_block *sb,
		umode_t mode, const char *name);
static int toyfs_user_create_file(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl);
static ssize_t toyfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset);
static ssize_t toyfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset);

static atomic_t counter;

static struct file_system_type toyfs_type = {
	.owner		= THIS_MODULE,
	.name		= "toyfs",
	.mount		= toyfs_mount,
	.kill_sb	= kill_litter_super,
	.fs_flags	= FS_USERNS_MOUNT,
};

static const struct super_operations toyfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static const struct file_operations toyfs_file_ops = {
	.open		= simple_open,
	.read		= toyfs_read_file,
	.write		= toyfs_write_file,
};

static const struct inode_operations toyfs_dir_inode_ops = {
	.create		= toyfs_user_create_file,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.rmdir		= simple_rmdir,
	.rename		= simple_rename,
};

static struct inode *toyfs_get_inode(struct super_block *sb,
		const struct inode *dir, umode_t mode, dev_t dev)
{
	struct inode *ret = new_inode(sb);

	pr_info("tfs: get_inode");

	if (ret) {
		ret->i_mode = mode;
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = CURRENT_TIME;

		if (mode & S_IFREG) {
			pr_info("tfs: get_inode: creating a regular file");

			ret->i_fop = &toyfs_file_ops;
			ret->i_private = &counter;
		} else {
			pr_info("tfs: get_inode: creating a directory");

			ret->i_op = &toyfs_dir_inode_ops;
			ret->i_fop = &simple_dir_operations;
		}
	}

	return ret;
}

static int toyfs_fill_super(struct super_block *sb, void *data, int silent)
{
	static struct tree_descr toy_files[] = { {""} };
	struct inode *inode;
	int err;

	pr_info("tfs: fill_super");

	err = simple_fill_super(sb, TOYFS_MAGIC, toy_files);
	if (err)
		return err;

	sb->s_op = &toyfs_s_ops;

	atomic_set(&counter, 0);

	inode = toyfs_get_inode(sb, NULL, S_IFDIR, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	toyfs_create_file(sb, S_IFREG, "counter");

	return 0;
}

static struct dentry *toyfs_create_file(struct super_block *sb,
		umode_t mode, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	pr_info("tfs: create_file");

	qname.name = name;
	qname.len = strlen(name);
	qname.hash = full_name_hash(name, qname.len);
	dentry = d_alloc(sb->s_root, &qname);
	if (!dentry)
		goto out;

	inode = toyfs_get_inode(sb, NULL, mode | 0644, 0);
	if (!inode)
		goto out_dput;

	d_add(dentry, inode);
	return dentry;

out_dput:
	dput(dentry);
out:
	return 0;
}

static int toyfs_user_create_file(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl)
{
	struct inode *inode;

	pr_info("tfs: create");

	inode = toyfs_get_inode(dir->i_sb, dir, mode | S_IFREG, 0);
	if (!inode)
		return -ENOSPC;

	d_instantiate(dentry, inode);
	dget(dentry);
	dir->i_mtime = dir->i_ctime = CURRENT_TIME;

	return 0;
}

static struct dentry *toyfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	pr_info("tfs: mount");

	return mount_nodev(fs_type, flags, data, toyfs_fill_super);
}

static ssize_t toyfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	int c, len;
	char tmp[TMPSIZE];
	atomic_t *counter = (atomic_t *) filp->private_data;

	pr_info("tfs: read");

	c = atomic_read(counter);
	atomic_inc(counter);

	len = snprintf(tmp, TMPSIZE, "%d\n", c);
	return simple_read_from_buffer(buf, count, offset, tmp, len);
}

static ssize_t toyfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	char tmp[TMPSIZE];
	atomic_t *counter = (atomic_t *) filp->private_data;
	long val;
	int err;

	pr_info("tfs: write");

	simple_write_to_buffer(tmp, TMPSIZE, offset, buf, count);
	tmp[count] = '\0';    /* ktrstrol needs a null terminated string */
	err = kstrtol(tmp, 10, &val);
	if (!err) {
		atomic_set(counter, val);
		pr_info("tfs: count is now %ld", val);
	}

	return count;
}

static int __init init_toyfs(void)
{
	pr_info("Loading toyfs\n");
	return register_filesystem(&toyfs_type);
}

static void __exit exit_toyfs(void)
{
	pr_info("Closing toyfs\n");
	unregister_filesystem(&toyfs_type);
}

module_init(init_toyfs);
module_exit(exit_toyfs);
