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
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <asm/ioctls.h>

#define OPEN            _IO('r', 1)
#define CLOSE           _IO('r', 2)
#define CHANGE_TIMES    _IOWR('r', 3, int)

struct timer_dev
{
    dev_t devid; /* 设备号 */
    int major;  /* 主设备号 */
    int minor;     /* 次设备号 */
    struct cdev cdev;   /* 字符设备 */
    struct class *class;
    struct device *device;
    struct device_node *nd; //设备节点
    struct timer_list timer;    //定时器
    int p_value;
    int gpio_led;               //gpio 编号
};

struct timer_dev timer;


/* Opens the device. */
static int timer_open(struct inode * inode, struct file * file)
{

    file->private_data = &timer;

	return 0;

}


static int timer_close(struct inode * inode, struct file * file)
{

   //struct timer_dev *dev = file->private_data;

  return 0;
}


static ssize_t timer_write(struct file * file,const __user char * buf, size_t count, loff_t *ppos)
{

    //struct timer_dev *dev = file->private_data;

 	return 0;
}

long timer_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    struct timer_dev *timer = file->private_data;
   // char *str = (char *)arg;
    //printk("arg:%d\r\n", str)
    int ret = 0;
    switch(cmd)
    {
        case OPEN:mod_timer(&timer->timer, jiffies + msecs_to_jiffies(timer->p_value));
                  printk("timer opened\r\n");
        break;

        case CLOSE:del_timer_sync(&timer->timer);
                   printk("timer closed\r\n");
        break;

        case CHANGE_TIMES:timer->p_value = (int)arg;
                    mod_timer(&timer->timer, jiffies + msecs_to_jiffies(timer->p_value));
        break;

        default :
            printk("can't find cmd!\r\n");
            return ret;
    }

	return ret;
}


static const struct file_operations timer_fops = {
	.owner = THIS_MODULE,
	.open = timer_open,
	.write = timer_write,
	.release = timer_close,
    .unlocked_ioctl = timer_ioctl,
};

/* 定时器处理函数 */
static void timer_func(unsigned long arg)
{
    //struct timer_dev* dev = (struct timer_dev*)arg;

    printk("timer\r\n");

    mod_timer(&timer.timer, jiffies + msecs_to_jiffies(timer.p_value));

}


/* 驱动入口函数*/
static int __init led_init(void)
{
    int ret;

    /* 注册字符设备 */
    timer.major = 0;
    if(timer.major)
    {
        //给定主设备号
        timer.devid = MKDEV(timer.major,0);
       ret=register_chrdev_region(timer.devid, 1, "timer");
    }
    else
    {
        /* 不给定主设备号 */
        ret = alloc_chrdev_region(&timer.devid, 0, 1, "timer");
        timer.major = MAJOR(timer.devid);
        timer.minor = MINOR(timer.devid);
    }
    if(ret < 0)
    {
        goto faile_devid;
    }
    printk("timer major=%d, minor=%d\r\n", timer.major, timer.minor);

    /* 添加字符设备 */
    timer.cdev.owner = THIS_MODULE;
    cdev_init(&timer.cdev, &timer_fops);
    ret = cdev_add(&timer.cdev, timer.devid, 1);
    if(ret < 0)
    {
        goto faile_cdev;
    }

    /* 自动创建设备节点 */
    timer.class = class_create(THIS_MODULE, "timer");
    if (IS_ERR(timer.class))
    {
        ret = PTR_ERR(timer.class);
        goto faile_class;
    }    

    timer.device = device_create(timer.class, NULL, timer.devid, NULL, "timer");
     if (IS_ERR(timer.device))
    {
        ret = PTR_ERR(timer.device);
        goto faile_device;
    }


    /* 初始化定时器 */
    init_timer(&timer.timer);
    timer.p_value = 1000;
    timer.timer.function = timer_func;
    timer.timer.expires = jiffies + msecs_to_jiffies(timer.p_value);
    timer.timer.data = (unsigned long)&timer;
    // add_timer(&timer.timer);

    /****************************************************************************/
   
    return 0;

faile_device:
    class_destroy(timer.class);    
faile_class:
    cdev_del(&timer.cdev);    
faile_cdev:
    unregister_chrdev_region(timer.devid, 1);
faile_devid:
    return ret;

    
}

/* 驱动出口函数 */
static void __exit led_exit(void)
{
    
    
    /* 关led */
    gpio_set_value(timer.gpio_led, 0);

    /* 释放IO */
    gpio_free(timer.gpio_led);

    /* 删除定时器 */
    del_timer(&timer.timer);

    /* 删除字符设备 */
    cdev_del(&timer.cdev);

    //销毁设备
	device_destroy(timer.class, timer.devid);

	//销毁类
	class_destroy(timer.class);

    /* 注销设备号 */
    unregister_chrdev_region(timer.devid, 1);    

}


/* 驱动加载与卸载 */
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("huangzhuyu");
