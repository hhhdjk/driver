#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <dt-bindings/soc/s5p4418-base.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_irq.h>
#include <asm-generic/gpio.h>
#include <asm/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

int 	major;		/* 主设备号 */
int 	minor;		/* 次设备还 */
dev_t 	d_id;		/* 设备id */

struct cdev 	d_cdev;			/* 字符设备 */
struct class 	*d_class;		/* 字符设备类 */
struct device 	*d_device;		/* 字符设备节点 */

#define		D_NAME	"LED"
#define 	minor_num	1



int p_open (struct inode *inode, struct file *file)
{

	return 0;
}

int p_release (struct inode *inode, struct file *file)
{

	return 0;
}

ssize_t p_read (struct file *file, char __user *buf, size_t count, loff_t *ppos)
{

	return 0;
}
ssize_t p_write (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{

	return 0;
}

static struct file_operations f_operations = {
	.owner		= THIS_MODULE,
	.read 		= p_read,
	.write 		= p_write,
	.open 		= p_open,
	.release 	= p_release,
};


int p_probe(struct platform_device *pdev)
{
	
	//int ret;
	int i, j;
	struct resource *res;
	printk("led driver probe called pdev %d\r\n" , pdev->num_resources);

	/* 获取设备资源 */
	res = platform_get_resource(pdev , IORESOURCE_MEM , 0);
	printk("[0]:0x%lx , %d , %s\r\n" , res->start , resource_size(res) , res->name);

	res = platform_get_resource(pdev , IORESOURCE_MEM , 1);
	printk("[1]:0x%lx , %d , %s\r\n" , res->start , resource_size(res) , res->name);

	res = platform_get_resource(pdev , IORESOURCE_MEM , 2);
	printk("[2]:0x%lx , %d , %s\r\n" , res->start , resource_size(res) , res->name);


	/* 创建设备 */
	for(i=0 ; i < minor_num ; i++){
		d_device = device_create(d_class , NULL , MKDEV(major , i) , NULL , D_NAME"%d" , i);
		if(IS_ERR(d_device)){
			for(j=0 ; j < i; j++){
				device_destroy(d_class , MKDEV(major , j));
			}
				class_destroy(d_class);
				cdev_del(&d_cdev);
				unregister_chrdev_region(d_id , 1);
		}
	}

	return 0;
}
int p_remove(struct platform_device *pdev)
{
	printk("p_remove called\n");
	device_destroy(d_class , MKDEV(major , 0));
	return 0;
}

// static const struct platform_device_id id_table[] = {
// 	{""},
// 	{},
// 	{},
// };

static struct platform_driver led = {
	.probe		= p_probe,
	.remove		= p_remove,
	.driver = {
		.name = "Nanopi-led",
	},
};


static int __init pf_init(void)
{
	int ret;
	/* 设备号 */
	if(major){
				d_id = MKDEV(major , minor);
				ret = register_chrdev_region(d_id , 1 ,D_NAME);
	}else{
				ret = alloc_chrdev_region(&d_id , 0 ,minor_num, D_NAME);
				major = MAJOR(d_id);
				minor = MINOR(d_id);
	}
	if(ret  < 0){
				printk("register fail!\n");
				return -EINVAL;
	}
	printk("new_led major=%d, minor=%d\n",major, minor);

	/* 注册字符设备 */
	d_cdev.owner = f_operations.owner;
	cdev_init(&d_cdev , &f_operations);
	if( cdev_add(&d_cdev ,d_id ,  1) ){
		printk("cdev_add err\r\n");
		unregister_chrdev_region(d_id , 1);
		return -EINVAL;
	}

	/* 创建设备类 */
	d_class = class_create(THIS_MODULE , "LED_CLASS");
	if(IS_ERR(d_class)){
		printk("class_create err\r\n");
		cdev_del(&d_cdev);
		unregister_chrdev_region(d_id , 1);
		return PTR_ERR(d_class);
	}
	
	return platform_driver_register(&led);
}

static void __exit pf_exit(void)
{	
	unregister_chrdev_region(d_id , 1);
	platform_driver_unregister(&led);
	return;
}

module_init(pf_init);
module_exit(pf_exit);

MODULE_AUTHOR("hzy");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("this is a platform");
MODULE_VERSION("1.0");


