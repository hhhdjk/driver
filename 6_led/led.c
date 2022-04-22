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

#define GPIOLED "gpioled"
dev_t devid; /* 设备号 */
int major = 102;  /* 主设备号 */
int minor = 1;     /* 次设备号 */
struct cdev cdev;   /* 字符设备 */
struct class *class;
struct device *device;
struct device_node *nd; //设备节点
int gpio_led;
struct property *comppro; //属性值


/* Opens the device. */
static int dtsled_open(struct inode * inode, struct file * file)
{

   // file->private_data = &gpioled;

	printk("chrdevbase_open\r\n");
	return 0;

}


static int dtsled_close(struct inode * inode, struct file * file)
{

   //struct gpioled_dev *dev = file->private_data;

	printk("chrdevbase_release\r\n");
  /* do nothing for now */
  return 0;
}


static ssize_t dtsled_write(struct file * file, const __user char * buf, size_t count, loff_t *ppos)
{

    //struct gpioled_dev *dev = file->private_data;

	printk("chrdevbase_write\r\n");
 	return 0;
}


static const struct file_operations gpioled_fops = {
	.owner = THIS_MODULE,
	.open = dtsled_open,
	.write = dtsled_write,
	.release = dtsled_close
};

/* 驱动入口函数*/
static int __init led_init(void)
{
    int ret;

    /* 注册字符设备 */
    if(major)
    {
        //给定主设备号
        devid = MKDEV(major,minor);
       ret=register_chrdev_region(devid, 1, GPIOLED);
    }
    else
    {
        /* 不给定主设备号 */
        ret = alloc_chrdev_region(&devid, 0, 1, GPIOLED);
        major = MAJOR(devid);
        minor = MINOR(devid);
    }
    if(ret < 0)
    {
        goto faile_devid;
    }
    printk(" major=%d, minor=%d\r\n", major, minor);

    /* 添加字符设备 */
    cdev.owner = gpioled_fops.owner;
    cdev_init(&cdev, &gpioled_fops);
    ret = cdev_add(&cdev, devid, 1);
    if(ret < 0)
    {
        goto faile_cdev;
    }

    /* 自动创建设备节点 */
    class = class_create(THIS_MODULE, GPIOLED);
    if (IS_ERR(class))
    {
        ret = PTR_ERR(class);
        goto faile_class;
    }    

    device = device_create(class, NULL, devid, NULL, GPIOLED);
     if (IS_ERR(device))
    {
        ret = PTR_ERR(device);
        goto faile_device;
    }

    /* 获取设备树节点 */
    nd = of_find_node_by_path("/gpio-leds/led@2");
	if(nd == NULL)
	{
		ret = -EINVAL;
		goto faile_findnd;
	}    

    /* 获取LED所对应的GPIO */
    gpio_led = of_get_named_gpio(nd, "gpios", 0);
    if(gpio_led < 0)
    {
        printk("can not find led gpio\r\n");
        ret = -EINVAL;
        goto faile_gpio_led;
    }
    printk("led_gpio num=%d\r\n", gpio_led);


    /* 申请gpio */
    ret = gpio_request(gpio_led, "gpio-led");
    if(!ret)
    {
        printk("Faile to request the led gpio\r\n");
        ret = -EINVAL;
        goto faile_gpio_led;
    }

    /* 使用IO，设置为输入或输出 */
    ret = gpio_direction_output(gpio_led, 1);
    if(ret)
    {
        goto faile_gpio_out;
    }

    /* 输出低电平，点亮led灯 */
    gpio_set_value(gpio_led, 1);
    
    return 0;

faile_gpio_out:
    gpio_free(gpio_led);
faile_gpio_led:
faile_findnd:
faile_device:
    class_destroy(class);    
faile_class:
    cdev_del(&cdev);    
faile_cdev:
    unregister_chrdev_region(devid, 1);
faile_devid:
    return ret;

    
}

/* 驱动出口函数 */
static void __exit led_exit(void)
{


    /* 删除字符设备 */
    cdev_del(&cdev);

    //销毁设备
	device_destroy(class, devid);

	//销毁类
	class_destroy(class);

    /* 注销设备号 */
    unregister_chrdev_region(devid, 1); 

    // gpio_set_value(gpio_led, 0);

    // /* 释放IO */
    // gpio_free(gpio_led);   

}


/* 驱动加载与卸载 */
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("huangzhuyu");

