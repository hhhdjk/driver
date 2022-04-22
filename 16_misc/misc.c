#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/miscdevice.h>


/* struct miscdevice  {
	int minor;
	const char *name;
	const struct file_operations *fops;
	struct list_head list;
	struct device *parent;
	struct device *this_device;
	const struct attribute_group **groups;
	const char *nodename;
	umode_t mode;
}; */

static struct miscdevice *misc;
ssize_t misc_read (struct file *file, char __user *buf, size_t count, loff_t *ppos)
{

	return 0;
}


ssize_t misc_write (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{

	return 0;
}


int misc_open (struct inode *inode, struct file *file)
{

	return 0;
}

int misc_release (struct inode *inode, struct file *file)
{

	return 0;
}


struct file_operations misc_ops = {
	.owner		=THIS_MODULE,
	.open		=misc_open,
	.read		=misc_read,
	.write		=misc_write,
	.release	=misc_release,

};


static int __init misc_drv_init(void)
{
	int err;
	printk("misc_drv_init\r\n");
	misc = kzalloc( sizeof(struct miscdevice) , GFP_KERNEL);
	if(!misc){
		printk("kzalloc failed\r\n");
		return -EINVAL;
	}

	misc->name 		= "misc_test";
	misc->minor 	= MISC_DYNAMIC_MINOR;
	misc->fops		= &misc_ops;

	/* 	注册misc */
	err = misc_register(misc);
	if(err)
		printk("misc_register error\r\n");

	return err;
}

static void __exit misc_drv_exit(void)
{
	printk("misc_drv_exit\r\n");
	misc_deregister(misc);
	return;
}

module_init(misc_drv_init);
module_exit(misc_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUANGZHUYU");	 				

