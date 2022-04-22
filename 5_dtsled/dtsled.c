///home/china/NanoPi2/
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
#include <linux/of_gpio.h>
#include <dt-bindings/soc/s5p4418-base.h>

#include "pinctrl-s5pxx18.h"
#include "s5pxx18-gpio.h"


#define s5pxx18_led_MAJOR 101
#define s5pxx18_led_MINOR 2
#define s5pxx18_led_device_name "led"

struct s5pxx18_led_cdrv 
{
	dev_t dev_id;
	int MAJOR;
	int MINOR;
	void * s5pxx18_led_prv;
} *s5pxx18_led_device;

struct cdev s5pxx18_led_cdev;
struct class *s5pxx18_led_class;
struct device * s5pxx18_led_devices;
struct device_node *nd; //设备节点
struct property *comppro; //属性值
int gpio_num;

// char led_statue[] = {0,};
// void __iomem * GPIOB_BASE_VA;

int s5pxx18_led_open (struct inode *node, struct file *file)
{
	// printk("APP open led\r\n");
	// GPIOB_BASE_VA = ioremap( PHYS_BASE_GPIOB , 0x6c);
	// printk("0x%lx\r\n", GPIOB_BASE_VA);
	// printk("led init...\r\n");
	// s5pxx18_gpio_device_init( GPIOB_BASE_VA , 1);
	// nx_soc_gpio_set_io_func( PAD_GPIO_B+12 , nx_gpio_padfunc_2 );
	// nx_soc_gpio_set_io_dir( PAD_GPIO_B+12 ,1  );
	return 0;
}

ssize_t s5pxx18_led_read (struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	// if(!nx_soc_gpio_get_out_value( PAD_GPIO_B+12 )){
	// 	memcpy(led_statue  , "on", sizeof("on"));
	// }
	// else{
	// 	memcpy(led_statue  , "off", sizeof("off"));
	// }

	// if( 0 != (copy_to_user(buf , led_statue , size))){
	// 	printk("kernel read error\r\n");
	// 	return -EINVAL;
	// }

	return 0; 
}


ssize_t s5pxx18_led_write (struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	// unsigned char w_buf[] = {0};
	// printk("size:%d\r\n", size);
	// if( 0 != (copy_from_user(w_buf , buf , size))){
	// 	printk("kernel write error\r\n");
	// 	return -EINVAL;
	// }
	// printk("[From user] %s\r\n", w_buf);
	// if(!strcmp (w_buf , "on")){
	// 	nx_soc_gpio_set_out_value( PAD_GPIO_B+12 , 0 );

	// }

	// if(!strcmp (w_buf , "off")){
	// 	nx_soc_gpio_set_out_value( PAD_GPIO_B+12 , 1 );

	// }
	return size;
}


int s5pxx18_led_release (struct inode *node, struct file *file)
{
	// printk("APP s5pxx18_led_release\r\n");
	// printk("APP close led\r\n");
	// nx_soc_gpio_set_out_value( PAD_GPIO_B+12 , 1 );
	// iounmap(GPIOB_BASE_VA);
	return 0;
}

long s5pxx18_led_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("APP s5pxx18_led_ioctl\r\n");

	return 0;
}


loff_t s5pxx18_led_llseek (struct file *file, loff_t ppos, int size)
{
	printk("APP s5pxx18_led_llseek\r\n");

	return 0;
}


struct file_operations s5pxx18_led_ops = {
	.owner = THIS_MODULE,
	.open = s5pxx18_led_open,
	.read = s5pxx18_led_read,
	.write = s5pxx18_led_write,
	.release = s5pxx18_led_release,
	.llseek = s5pxx18_led_llseek,
	.unlocked_ioctl = s5pxx18_led_ioctl,
};



static int s5pxx18_led_register_cdrv( struct s5pxx18_led_cdrv* led_drv )
{
	int err = 0;
	struct s5pxx18_led_cdrv* led = led_drv;

	if(led->MAJOR)
	{
		if( register_chrdev_region(led->dev_id , 1 , s5pxx18_led_device_name) )
			goto dev_id;
	}
	else
	{
		if( alloc_chrdev_region(&led->dev_id , 0 , 1 , s5pxx18_led_device_name) ){
			goto dev_id;
		}else{
			led->MAJOR = MAJOR(led->dev_id);
			led->MINOR = MINOR(led->dev_id);
		}
	}
	
	s5pxx18_led_cdev.owner = s5pxx18_led_ops.owner;
	cdev_init(&s5pxx18_led_cdev, &s5pxx18_led_ops);

	if( cdev_add(&s5pxx18_led_cdev ,  led->dev_id , 1) ){
		goto cdev_add_failed;
	}

	//自动创建设备节点
	s5pxx18_led_class = class_create(s5pxx18_led_ops.owner, s5pxx18_led_device_name);
	if (IS_ERR(s5pxx18_led_class)) {
		err = PTR_ERR(s5pxx18_led_class);
		goto out_chrdev;
	}

	s5pxx18_led_devices = device_create(
		s5pxx18_led_class, 
		NULL , 
		led->dev_id, 
		NULL , 
		s5pxx18_led_device_name );
	
	if (IS_ERR(s5pxx18_led_devices)) {
		err = PTR_ERR(s5pxx18_led_devices);
		goto out_devices;
	}

    	   /* 获取设备树节点 */
    nd = of_find_node_by_path("/gpio-leds/led@2");
	if(nd == NULL)
	{
		printk(KERN_WARNING "of_find_node_by_path get node failed\r\n");
	}    

    /* 获取LED所对应的GPIO */
    gpio_num = of_get_named_gpio(nd, "gpios", 0);
    if(gpio_num < 0)
    {
        printk("can not find led gpio\r\n");
    }
    printk("gpio num=%d\r\n", gpio_num);


    err = gpio_request(gpio_num, "mygpio");
    if(err)
    {
        printk("Faile to request the led gpio\r\n");
    }else {
        printk("request gpio ok!\r\n");
    }

    /* 使用IO，设置为输入或输出 */
    err = gpio_direction_output(gpio_num, 1);
    if(err)
    {
         printk("Faile to gpio_direction_output the led gpio\r\n");
    }

    /* 输出低电平，点亮led灯 */
    gpio_set_value(gpio_num, 1);

	return 0;

out_devices:
	class_destroy(s5pxx18_led_class);

out_chrdev:
	cdev_del(&s5pxx18_led_cdev);

cdev_add_failed:
	unregister_chrdev_region(led->dev_id , 1);

dev_id:
	return err;

}

static void s5pxx18_led_unregister_cdrv(struct s5pxx18_led_cdrv* led_drv)
{

	device_destroy(s5pxx18_led_class , led_drv->dev_id);
	class_destroy(s5pxx18_led_class);
	cdev_del(&s5pxx18_led_cdev);
	unregister_chrdev_region(led_drv->dev_id , 1);
}


static int s5pxx18_led_cdrv_init( void )
{
	int err =0;
	printk("s5pxx18_led_cdrv_init\r\n");
	s5pxx18_led_device = kzalloc( sizeof(struct s5pxx18_led_cdrv), GFP_KERNEL);
	if( !s5pxx18_led_device ){
		printk(KERN_WARNING s5pxx18_led_device_name "can't get kzalloc\r\n");
		goto failed;
	}
	s5pxx18_led_device->MAJOR = s5pxx18_led_MAJOR;
	s5pxx18_led_device->MINOR = s5pxx18_led_MINOR;
	s5pxx18_led_device->dev_id = MKDEV(s5pxx18_led_MAJOR, s5pxx18_led_MINOR);

	if( -EINVAL == (err = s5pxx18_led_register_cdrv( s5pxx18_led_device )) ){
		printk(KERN_WARNING s5pxx18_led_device_name "s5pxx18_led_register_cdrv register failed!\r\n");
		goto register_failed;
	}

	return err;

register_failed:
	;

failed:
	kfree(s5pxx18_led_device);
	return -EINVAL;

}


static int __init s5pxx18_led_drv_init(void)
{
	printk("led_drv_init\r\n");
	s5pxx18_led_cdrv_init();
	return 0;
}

static void __exit s5pxx18_led_drv_exit(void)
{
	printk("led_drv_exit\r\n");
    gpio_set_value(gpio_num, 0);
    gpio_free(gpio_num);
	s5pxx18_led_unregister_cdrv(s5pxx18_led_device);
    kfree(s5pxx18_led_device);
	return;
}

module_init(s5pxx18_led_drv_init);
module_exit(s5pxx18_led_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUANGZHUYU");	 				

