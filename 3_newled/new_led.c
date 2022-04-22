#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <dt-bindings/soc/s5p4418-base.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/drivers/pinctrl/nexell/s5pxx18-gpio.h>



struct new_led_dev
{
	struct cdev cdev;	   //字符设备
	struct class *class;   //类
	struct device *device; //设备
	dev_t devid;		   //设备号
	int major;			   //主设备号
	int minor;			   //次设备号
};

struct new_led_dev new_led;

#define GpioB_Out (PHYS_BASE_GPIOB)
#define GpioB_OutEnb (PHYS_BASE_GPIOB + 0x04)
#define GpioB_ALTFN0 (PHYS_BASE_GPIOB + 0X20)
#define GpioB_ALTFN1 (PHYS_BASE_GPIOB + 0X24)

//物理地址映射后得到的虚拟地址指针
static void __iomem *GPIOB_Out;
static void __iomem *GPIOB_OutEnb;
static void __iomem *GPIOB_ALTFN0;
static void __iomem *GPIOB_ALTFN1;

/* Opens the device. */
static int led_open(struct inode *inode, struct file *file)
{
	file->private_data = &new_led;
	printk("chrdevbase_open\r\n");
	return 0;
}

/* Closes the device. */
static int led_close(struct inode *inode, struct file *file)
{
	struct new_led_dev *devices = (struct new_led_dev *)file->private_data;

	printk("chrdevbase_release\r\n");
	/* do nothing for now */
	return 0;
}

/* Writes data to eeprom. */
static ssize_t led_write(struct file *file, __user char *buf, size_t count, loff_t *ppos)
{
	printk("chrdevbase_write\r\n");

	return 0;
}

/*
 * 字符设备操作集合
 */
static const struct file_operations new_led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
	.release = led_close};

/*
 * 入口
 */
static int __init newled_init(void)
{
	printk("led init\n");
	unsigned int val = 0;
	int ret = 0;
	/* GPIO初始化
	 * PHYS_BASE_GPIOB		(0xC001B000)
	 * #define  GpioB_Out		   (PHYS_BASE_GPIOB)
	 * #define  GpioB_OutEnb	   (PHYS_BASE_GPIOB+0x04))
	 * #define  GpioB_ALTFN0	   (PHYS_BASE_GPIOB+0X20))
	 * #define  GpioB_ALTFN1	   (PHYS_BASE_GPIOB+0X24))
	 */
	GPIOB_Out = ioremap(GpioB_Out, 4);
	GPIOB_OutEnb = ioremap(GpioB_OutEnb, 4);
	GPIOB_ALTFN0 = ioremap(GpioB_ALTFN0, 4);
	GPIOB_ALTFN1 = ioremap(GpioB_ALTFN1, 4);

	//初始化
	val = readl(GPIOB_ALTFN0); //读寄存器
	val &= ~(0X3 << 24);	   //清寄存器
	val |= (0x2 << 24);		   //设置寄存器值
	writel(val, GPIOB_ALTFN0); //写寄存器

	val = readl(GPIOB_OutEnb);
	val |= (1 << 12);
	writel(val, GPIOB_OutEnb);

	val = readl(GPIOB_Out);
	val &= ~(1 << 12);
	writel(val, GPIOB_Out);

	new_led.major = 200;
	//注册字符设备
	if (new_led.major)
	{
		new_led.devid = MKDEV(new_led.major, 0);
		ret = register_chrdev_region(new_led.devid, 1, "LED");
		if (ret < 0)
		{
			printk("register fail!\n");
			return -EIO;
		}
		printk("new_led major=%d, minor=%d\n", new_led.major, new_led.minor);
	}
	else
	{
		ret = alloc_chrdev_region(&new_led.devid, 0, 1, "LED");
		if (ret < 0)
		{
			printk("register fail!\n");
			return -EIO;
		}
		new_led.major = MAJOR(new_led.devid);
		new_led.minor = MINOR(new_led.devid);
		printk("new_led major=%d, minor=%d\n", new_led.major, new_led.minor);
	}

	//注册字符设备
	new_led.cdev.owner = THIS_MODULE;
	cdev_init(&new_led.cdev, &new_led_fops);
	ret = cdev_add(&new_led.cdev, new_led.devid, 1);

	//自动创建设备节点
	new_led.class = class_create(THIS_MODULE, "LED");
	if (IS_ERR(new_led.class))
		return PTR_ERR(new_led.class);

	new_led.device = device_create(new_led.class, NULL, new_led.devid, NULL, "LED");
	if (IS_ERR(new_led.device))
		return PTR_ERR(new_led.device);

	return 0;
}

/*
 * 出口
 */
static void __exit newled_exit(void)
{
	unsigned int val = 0;
	val = readl(GPIOB_Out);
	val |= (1 << 12);
	writel(val, GPIOB_Out);

	//取消地址映射
	iounmap(GPIOB_Out);
	iounmap(GPIOB_OutEnb);
	iounmap(GPIOB_ALTFN0);
	iounmap(GPIOB_ALTFN1);

	//删除字符设备
	cdev_del(&new_led.cdev);

	//销毁设备
	device_destroy(new_led.class, new_led.devid);

	//销毁类
	class_destroy(new_led.class);

	//注销设备号
	unregister_chrdev_region(new_led.devid, 1);

	printk("led exit\n");
	return;
}

/*
 * 注册驱动加载和卸载
 */

module_init(newled_init);
module_exit(newled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUANGZHUYU");
