///home/china/NanoPi2/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
//#include <linux/mm.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/spinlock.h>

#define hello_MAJOR 100
#define hello_device "hello"

char kernel_buf[] = "first kernel test";
char r_buf[] = {0};

spinlock_t lock;
unsigned long irqflag;
int delay = 0;

long ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{

	return 0;
}


loff_t hello_llseek (struct file *file, loff_t ppos, int size)
{

	return 0;
}


ssize_t hello_read (struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	
	memcpy(r_buf , kernel_buf, sizeof(kernel_buf));

	if( 0 != (copy_to_user(buf , r_buf , size))){
		printk("kernel read error\r\n");
		return -EINVAL;
	}

	return sizeof(kernel_buf); 
}


ssize_t hello_write (struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned char w_buf[] = {0};
	printk("size:%d\r\n", size);
	if( 0 != (copy_from_user(w_buf , buf , size))){
		printk("kernel write error\r\n");
		return -EINVAL;
	}
	printk("[From user] %s\r\n", w_buf);
	return size;
}


int hello_open (struct inode *node, struct file *file)
{
	
	
	
	spin_lock_irqsave(&lock, irqflag);
	delay = 100;
	spin_unlock_irqrestore(&lock, irqflag);
	printk("APP open\r\n");
	return 0;
}


int hello_release (struct inode *node, struct file *file)
{
	
	printk("APP close\r\n");
	return 0;
}

struct file_operations hello_ops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
	.release = hello_release,
	.llseek = hello_llseek,
	.unlocked_ioctl = ioctl,
};


static int __init hello_drv_init(void)
{
	printk("hello_drv_init\r\n");
	spin_lock_init(&lock);
	if( 0 > (register_chrdev(hello_MAJOR,hello_device, &hello_ops))){
		printk(KERN_WARNING hello_device "can't get major number\r\n");
	}
	return 0;
}

static void __exit hello_drv_exit(void)
{
	printk("hello_drv_exit\r\n");
	unregister_chrdev(hello_MAJOR, hello_device);
	return;
}

module_init(hello_drv_init);
module_exit(hello_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUANGZHUYU");

