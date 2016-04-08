#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#define TOYFS_MAGIC 0x20160408
#define TMPSIZE 12

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luis de Bethencourt");

static struct dentry *toyfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data);
static void toyfs_create_files (struct super_block *sb, struct dentry *root);
static int toyfs_open(struct inode *inode, struct file *filp);
static ssize_t toyfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset);
static ssize_t toyfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset);

static atomic_t counter;

static struct file_system_type toyfs_type = {
	.owner 		= THIS_MODULE,
	.name 		= "toyfs",
	.mount 		= toyfs_mount,
	.kill_sb 	= kill_litter_super,
	.fs_flags 	= FS_USERNS_MOUNT,
};

static struct super_operations toyfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static struct file_operations toyfs_file_ops = {
	.open 		= toyfs_open,
	.read 		= toyfs_read_file,
	.write 		= toyfs_write_file,
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
	}

	return ret;
}

static int toyfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;

	pr_info("tfs: fill_super");

	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = TOYFS_MAGIC;
	sb->s_op = &toyfs_s_ops;

	root = toyfs_get_inode(sb, NULL, S_IFDIR | 0755, 0);
	if (!root)
		return -ENOMEM;
	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	root_dentry = d_make_root(root);
	if (!root_dentry) {
		iput(root);
		return -ENOMEM;
	}
	sb->s_root = root_dentry;

	toyfs_create_files (sb, root_dentry);

	return 0;
}

static struct dentry *toyfs_create_file (struct super_block *sb,
		struct dentry *dir, umode_t mode, const char *name,
		atomic_t *counter)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	pr_info("tfs: create_file");

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);
	dentry = d_alloc(dir, &qname);
	if (!dentry)
		goto out;

	inode = toyfs_get_inode(sb, NULL, mode | 0644, 0);
	if (!inode)
		goto out_dput;

	if (mode & S_IFREG) {
		pr_info("tfs: create regular file");

		inode->i_fop = &toyfs_file_ops;
		inode->i_private = counter;
	} else {
		pr_info("tfs: create a directory");

		inode->i_op = &simple_dir_inode_operations;
		inode->i_fop = &simple_dir_operations;
	}

	d_add(dentry, inode);
	return dentry;

out_dput:
	dput(dentry);
out:
	return 0;
}

static void toyfs_create_files (struct super_block *sb, struct dentry *root)
{
	pr_info("tfs: create_files");

	atomic_set(&counter, 0);
	toyfs_create_file(sb, root, S_IFREG, "counter", &counter);
}

static struct dentry *toyfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	pr_info("tfs: mount");

	return mount_single(fs_type, flags, data, toyfs_fill_super);
}

static int toyfs_open(struct inode *inode, struct file *filp)
{
	pr_info("tfs: open");

	filp->private_data = inode->i_private;
	return 0;
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

	pr_info("tfs: write");

	simple_write_to_buffer(tmp, TMPSIZE, offset, buf, count);
	atomic_set(counter, simple_strtol(tmp, NULL, 10));

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
