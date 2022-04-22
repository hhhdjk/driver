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

struct led_dev
{
	struct cdev cdev;	//字符设备
	struct class *class;	//类
	struct device *device;	//设备
	dev_t devid;			//设备号
	int major;				//主设备号
	int minor;				//次设备号
};

struct led_dev new_led;

/* Opens the device. */
static int led_open(struct inode * inode, struct file * file)
{
	file->private_data = &new_led;
	return 0;

}

static int led_close(struct inode * inode, struct file * file)
{
	struct led_dev *devices =(struct led_dev*)file->private_data;
    return 0;
}

static ssize_t led_write(struct file * file, __user char * buf, size_t count, loff_t *ppos)
{
 	return 0;
}

/* 
 * 字符设备操作集合
 */
static const struct file_operations new_led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
	.release = led_close
};

static int led_probe(struct platform_device *pdev)
{
    printk("led driver probe called\r\n");
    int i;
    unsigned int val = 0;
    int ret = 0;

    struct resource *led_resource[3];
    for(i = 0; i<3; i++)
    {
        led_resource[i] = platform_get_resource(pdev,IORESOURCE_MEM , i);
        if(led_resource[i] == NULL)
        {
            return -EINVAL;
        }
    }
	
	/* 创建字符设备节点 */
	new_led.device = device_create(new_led.class, NULL, new_led.devid, NULL,"LED");
	if (IS_ERR(new_led.device)){
		ret = PTR_ERR(new_led.device);
		goto destory_all;
	}
		
        
    return 0;

destory_all:


}

static int led_remove(struct platform_device *pdev)
{
    printk("led driver remove\r\n");

	//销毁设备
	device_destroy(new_led.class, new_led.devid);

    return 0;
}

static struct platform_driver leddrv = 
{
    .probe = led_probe,
    .remove = led_remove,
    .driver = {
        .name = "Nanopi2-led"
    },
};

static int __init leddrv_init(void)
{
	int ret;
	/*  设备号 */
	if(new_led.major){
		new_led.devid = MKDEV(new_led.major, 0);
		ret = register_chrdev_region(new_led.devid, 1, "LED");
	}
	else{
			ret = alloc_chrdev_region(&new_led.devid, 0, 1, "LED");
			new_led.major = MAJOR(new_led.devid);
			new_led.minor = MINOR(new_led.devid);
	}
    if(ret < 0){
		printk("register fail!\n");
		return -EINVAL;
	}
    printk("new_led major=%d, minor=%d\n",new_led.major, new_led.minor);

	/* 注册字符设备 */
	new_led.cdev.owner = THIS_MODULE;
	cdev_init(&new_led.cdev, &new_led_fops);
	if ( cdev_add(&new_led.cdev, new_led.devid, 1) ){
		unregister_chrdev_region(new_led.devid, 1);
		return -EINVAL;
	}

	/* 创建设备类 */
	new_led.class = class_create(THIS_MODULE, "LED");
	if (IS_ERR(new_led.class)){
		cdev_del(&new_led.cdev);
		return PTR_ERR(new_led.class);
	}
		

	return platform_driver_register(&leddrv);
}

static void __exit leddrv_exit(void)
{
	//销毁类
	class_destroy(new_led.class);	

	//删除字符设备
	cdev_del(&new_led.cdev);

	//注销设备号
	unregister_chrdev_region(new_led.devid, 1);
	return platform_driver_unregister(&leddrv);
}

module_init(leddrv_init);
module_exit(leddrv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("huangzhuyu");
